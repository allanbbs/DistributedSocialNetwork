//
// Created by allanbs on 20/12/21.
//

#ifndef PROJ2_TCPCONNECTION_H
#define PROJ2_TCPCONNECTION_H
#include "Connection.h"
#include "../utils.h"
#include "../tsqueue.h"
#include <string>
#include <iostream>
#include <utility>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

template<typename MSG_TYPE>
class TCPConnection : public Connection<MSG_TYPE>,public std::enable_shared_from_this<TCPConnection<MSG_TYPE>> {
    public:
        //TCPConnection(peer_address local, peer_address remote);
        TCPConnection(boost::asio::ip::tcp::socket socket);
        void send_bytes(byte* buffer, size_t size) override;
        void send(Message<MSG_TYPE> &message) override;
        void close() override;
        bool is_open() override;
        tsqueue<Message<MSG_TYPE>>& incoming() override{ return incoming_queue;}
        void start_read() override;
    private:
        tsqueue<Message<MSG_TYPE>> incoming_queue;
        boost::asio::ip::tcp::socket socket;
        Message<MSG_TYPE> temp_msg;
        void read_header();
        void read_body();

    bool is_active();


    void handle_read_header(const boost::system::error_code &ec, const unsigned long &length);

    void handle_read_body(const boost::system::error_code &ec, const unsigned long &length);
};
#endif //PROJ2_TCPCONNECTION_H
