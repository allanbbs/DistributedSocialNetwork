//
// Created by allanbs on 26/12/21.
//

#include "../../lib/Kademlia/ConnectionHandler.h"
#include <boost/bind.hpp>
namespace ip = boost::asio::ip;


void ConnectionHandler::send(peer_address endpoint, byte* bytes, size_t size){
    std::bitset<sizeof(size_t)*8> message_size(size);
    byte size_bytes[sizeof(size_t)*8];
    for(size_t i = 0;i<message_size.size();i++){
        size_bytes[i] = message_size.test(i);
    }
    connections[endpoint]->send_bytes(size_bytes, message_size.size());
    connections[endpoint]->send_bytes(bytes,size);
}

void ConnectionHandler::send(peer_address endpoint, Message<KademliaMessageType> message){
    std::cout << "Sending to: " << endpoint << std::endl;
    std::cout << "Connections available: " << std::endl;
    for(auto& e: connections)
        std::cout << e.first << std::endl;
    if(connections.count(endpoint)) // check if connection exists before sending
        connections[endpoint]->send(message);
    else
        std::cerr<<"[ConnectionHandler] Error in send - Connection does not exist!"<<std::endl;
}

void ConnectionHandler::handle_n_messages(peer_address endpoint,size_t max_messages){
    auto temp = connections[endpoint].get();
    size_t count = 0;
    while(temp && !temp->incoming().empty() && count<max_messages){
        auto t = temp->incoming().pop_front();
        std::cout<<"Answer received!"<<std::endl;
        std::cout<<t.to_string()<<std::endl;
        count++;
    }
}

bool ConnectionHandler::connect(peer_address remote_endpoint) {
    ip::basic_endpoint<ip::tcp> endpoint(ip::make_address(remote_endpoint.ip), remote_endpoint.port);
    auto  socket_ = ip::tcp::socket (io_context);
    socket_.connect(endpoint); // TODO add try catch here

    std::shared_ptr<Connection<KademliaMessageType>> conn =
            std::make_shared<TCPConnection<KademliaMessageType>> (std::move(socket_));

    auto temp = conn->get_remote_endpoint();
    auto temp2 = conn->get_local_endpoint();
    std::cout<<"Connect! "<<temp<<" to "<<temp2<<std::endl;
    auto b = connections.insert(std::make_pair(temp, std::move(conn))).second;
    if(b)
        connections[temp]->start_read();
    return b;
}

void ConnectionHandler::disconnect(peer_address endpoint) {
    mutex.lock();
    if(connections.find(endpoint) != connections.end()) { // check if connection exists before sending
        //while(connections[endpoint])
        connections[endpoint]->close();
        connections[endpoint].reset();
        connections.erase(endpoint);
    }
    mutex.unlock();
}

/**
 * Periodically checks for expired connections
 */
[[noreturn]] void ConnectionHandler::check_connections(){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(2));
        for(auto it = connections.begin(); it!=connections.end();) {
            // close if it is expired and don't have any messages to be processed
            if(it->second->is_expired() && it->second->incoming().empty()){
                it->second->close();
                it->second.reset();
                it = connections.erase(it);
            }else ++it;
        }
    }
}

bool ConnectionHandler::connection_exists(peer_address endpoint){
    return connections.find(endpoint) != connections.end();
}

void ConnectionHandler::print_connection(){
    for(auto &connection : connections) {
        std::cout << "=======================" << std::endl;
        std::cout << connection.second->get_local_endpoint() << " " <<  connection.second->get_remote_endpoint() << std::endl;
        std::cout << "=======================" << std::endl;
    }
}


void ConnectionHandler::handle_accept(){

    acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    acceptor_.async_accept([&](const boost::system::error_code ec, ip::tcp::socket socket_){
        // check if conn already exists
        if(!connections.count(peer_address{
            socket_.remote_endpoint().address().to_string(), socket_.remote_endpoint().port()})) {
            // creates a new connection
            std::shared_ptr<Connection<KademliaMessageType>> conn =
                    std::make_shared<TCPConnection<KademliaMessageType>>(std::move(socket_));
            // store it in our map
            auto temp = conn->get_remote_endpoint();
            auto temp2 = conn->get_local_endpoint();
            auto b= this->connections.insert(std::make_pair(temp, std::move(conn)));
            if(b.second)
                connections[temp]->start_read();
            std::cout <<"Accept! "<< temp2 << " to "<<temp << std::endl;
        }
        this->handle_accept();
    });
}

void ConnectionHandler::run() {
    is_running = true;
    this->handle_accept();
    context_t = std::thread ([this]{io_context.run();});
    //std::thread check (&ConnectionHandler::check_connections, this);
    //check.join();
}

void ConnectionHandler::stop(){
    is_running = false;
    io_context.stop();
    if(context_t.joinable())
        context_t.join();
}

void ConnectionHandler::update(){
    //TODO thread safe map pq pode dar erase no meio do loop da connection
    while(is_running){
        for(auto &connection : connections) {
            mutex.lock();
            while (connection.second && !connection.second->incoming().empty()) {
                auto p = connection.second->incoming().pop_front();
                handle_message(p, connection.first);
            }
            mutex.unlock();
        }
        sleep(1);
    }
}

void ConnectionHandler::handle_message(Message<KademliaMessageType> p, peer_address endpoint) {
    std::cerr <<  "Handling Message!" << std::endl;
    std::cerr <<  p.to_string() << std::endl;
    if(is_kademlia_reply(p.get_id())){
        msg_replies.push_back(p);
        return;
    }
    if(p.get_id() == JOIN){
        peer_reference->handle_join(p, endpoint);
    }
    else if(p.get_id() == LEAVE){
        peer_reference->handle_leave(p);
    }
    else if(p.get_id() == LOOKUP){
        peer_reference->handle_lookup(p,endpoint);
    }
    else if(p.get_id() == PING){
        peer_reference->handle_ping(p,endpoint);
    }
}
