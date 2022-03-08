//
// Created by allanbs on 21/12/21.
//
#include <boost/bind.hpp>
#include "../../lib/tsqueue.h"
#include "../../lib/Network/TCPConnection.h"
#include "../../lib/utils.h"
#include "../../lib/Message.h"
/*
TCPConnection::TCPConnection(peer_address local,peer_address remote)
        : Connection(std::move(local),std::move(remote)){}
*/

template class TCPConnection<KademliaMessageType>;

template <typename MSG_TYPE>

TCPConnection<MSG_TYPE>::TCPConnection(boost::asio::ip::tcp::socket socket)
        : Connection<MSG_TYPE>(std::move(peer_address {socket.local_endpoint().address().to_string(),
                                                       socket.local_endpoint().port()}),
                     std::move(peer_address {socket.remote_endpoint().address().to_string(),socket.remote_endpoint().port()})),
                     socket(std::move(socket))
                     {
                         this->start_time = std::chrono::steady_clock::now();
                         this->set_expiry_period(std::chrono::milliseconds (10000));
                         //this->read_header();
                     }



/*
template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::read_header() {
    boost::shared_ptr<TCPConnection<MSG_TYPE>> p = this->shared_from_this();
    boost::asio::async_read(socket,boost::asio::buffer(&temp_msg.get_header(),sizeof(header<MSG_TYPE>)),
                            [p](const boost::system::error_code& ec,const long unsigned int& length){
                                 if(!ec.failed()){
                                    if(p->temp_msg.get_size() > 0){
                                        p->temp_msg.get_body().resize(p->temp_msg.get_size());
                                        p->read_body();
                                    }
                                    else{
                                        //
                                        p->read_header();
                                        //
                                    }
                                 }else if(ec==boost::asio::error::eof){

                                 }
                                 else{
                                     std::cerr<<"[TCPConnection] ERROR in read_header:"<<std::endl;
                                     std::cerr<<ec.message()<<std::endl;
                                 }
    });
}
*/
template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::read_header() {
    boost::asio::async_read(socket,boost::asio::buffer(&temp_msg.get_header(),sizeof(HEADER<MSG_TYPE>)),
                            boost::bind(&TCPConnection<MSG_TYPE>::handle_read_header,this->shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}


/*
template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::read_body() {
    boost::shared_ptr<TCPConnection<MSG_TYPE>> p = this->shared_from_this();
    boost::asio::async_read(socket,boost::asio::buffer(temp_msg.get_body().data(),temp_msg.get_size()),
                            [p](const boost::system::error_code& ec,const long unsigned int& length){
                                if(!ec.failed()){
                                    p->incoming_queue.push_back(p->temp_msg);
                                    p->read_header();

                                }else if(ec==boost::asio::error::eof){

                                }
                                else{
                                    std::cerr<<"[TCPConnection] ERROR in read_body:"<<std::endl;
                                    std::cerr<<ec.message()<<std::endl;

                                }
                            });
}
*/

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::read_body() {
    boost::asio::async_read(socket,boost::asio::buffer(temp_msg.get_body().data(),temp_msg.get_size()),
                            boost::bind(&TCPConnection<MSG_TYPE>::handle_read_body,this->shared_from_this(),boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred));
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::send(Message<MSG_TYPE> &message){
    send_bytes(message.to_bytes().data(), message.total_size());
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::send_bytes(byte* buffer, size_t size) {
    boost::asio::mutable_buffer buf(buffer, size);
    this->socket.send(buf);
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::close() {
    try {
        //this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
        this->socket.close();
    }catch(std::exception& e){
        std::cerr<<"[TCPCONNECTION] error in close"<<std::endl;
        exit(-1);
    }
}

template <typename MSG_TYPE>
bool TCPConnection<MSG_TYPE>::is_open() {
    return socket.is_open();
}

template <typename MSG_TYPE>
bool TCPConnection<MSG_TYPE>::is_active() {
    return true;//socket.;
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::start_read() {
    this->read_header();
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::handle_read_header(const boost::system::error_code& ec,const long unsigned int& length) {
    if(!ec.failed()){
        if(this->temp_msg.get_size() > 0){
            this->temp_msg.get_body().resize(this->temp_msg.get_size());
            this->read_body();
        }
        else{
            this->incoming_queue.push_back(this->temp_msg);
            this->read_header();
        }
    }else if(ec==boost::asio::error::eof){
        //TODO
    }
    else if(ec==boost::asio::error::operation_aborted){
        //TODO
    }
    else{
        std::cerr<<"[TCPConnection] ERROR in read_header:"<<std::endl;
        std::cerr<<ec.message()<<std::endl;
    }
}

template <typename MSG_TYPE>
void TCPConnection<MSG_TYPE>::handle_read_body(const boost::system::error_code& ec,const long unsigned int& length) {
    if(!ec.failed()){
        this->incoming_queue.push_back(this->temp_msg);
        this->read_header();

    }else if(ec==boost::asio::error::eof){
        //TODO
    }
    else if(ec==boost::asio::error::operation_aborted){
        //TODO
    }
    else{
        std::cerr<<"[TCPConnection] ERROR in read_body:"<<std::endl;
        std::cerr<<ec.message()<<std::endl;

    }
}



