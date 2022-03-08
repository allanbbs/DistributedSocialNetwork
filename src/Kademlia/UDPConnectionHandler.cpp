//
// Created by allanbs on 04/01/22.
//

#include "../../lib/Kademlia/UDPConnectionHandler.h"
#include <boost/bind.hpp>
namespace ip = boost::asio::ip;


void UDPConnectionHandler::send(peer_address remote_endpoint, Message<KademliaMessageType> message){
    ip::basic_endpoint<ip::udp> endpoint(ip::make_address(remote_endpoint.ip), remote_endpoint.port);
    //std::cout << "Sending to: " << endpoint << std::endl;
    auto k = message.to_bytes();
    boost::asio::mutable_buffer buf(k.data(),message.total_size());
    byte proof[message.total_size()];
    memcpy(proof,buf.data(),buf.size());
    acceptor_.send_to(buf,endpoint);
    //std::cout<<"Sent "<<acceptor_.send_to(buf,endpoint)<<" bytes"<<std::endl;
}



void UDPConnectionHandler::async_read_message(const boost::system::error_code& error, unsigned int br){
    if(!error.failed()){
        size_t available = acceptor_.available();
        byte* buffer = new byte[available];
        //std::cout<<"Available: "<<available<<std::endl;
        boost::asio::ip::udp::endpoint senderEndpoint;
        boost::system::error_code ec;
        unsigned int packetSize = acceptor_.receive_from(boost::asio::buffer(buffer, available), senderEndpoint, 0, ec);
        if(!ec.failed()) {
            Message<KademliaMessageType> message;
            byte message_bytes[packetSize];
            memcpy(message_bytes, buffer, packetSize);
            message.from_bytes(buffer);
            incoming.push_back(std::make_pair(message,peer_address{senderEndpoint.address().to_string(),senderEndpoint.port()}));
            this->handle_receive();
        }else{
            std::cerr<<"[UDPHandler] error reading message:"<<std::endl;
            std::cerr<<error.message()<<std::endl;
        }
        delete[] buffer;

    }
    else{
        std::cerr<<"[UDPHandler] error in async_read_message:"<<std::endl;
        std::cerr<<error.message()<<std::endl;
    }
}

void UDPConnectionHandler::handle_receive(){
    acceptor_.async_receive(boost::asio::null_buffers(), boost::bind(&UDPConnectionHandler::async_read_message,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

void UDPConnectionHandler::run() {
    is_running = true;
    this->handle_receive();
    context_t = std::thread ([this]{io_context.run();});
    //std::thread check (&UDPConnectionHandler::check_connections, this);
    //check.join();
}

void UDPConnectionHandler::stop(){
    is_running = false;
    incoming.set_block(false);
    incoming.notify();
    io_context.stop();
    if(context_t.joinable())
        context_t.join();
}

void UDPConnectionHandler::update(){
    while(is_running){
        incoming.wait();
        while(!incoming.empty()){
            auto p = incoming.pop_front();
            handle_message(p.first,p.second );
        }
    }
}

void UDPConnectionHandler::handle_message(Message<KademliaMessageType> p, peer_address endpoint) {
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
    else if(p.get_id() == FIND_VALUE){
        peer_reference->handle_find_value(p,endpoint);
    }
    else if(p.get_id() == STORE){
        peer_reference->handle_store(p,endpoint);
    }
    else if(p.get_id() == APPLICATION_REQUEST || p.get_id() == APPLICATION_REPLY){
        peer_reference->handle_app_messages(p,endpoint);
    }
}
