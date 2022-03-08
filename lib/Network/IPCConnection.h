//
// Created by breno on 08/01/22.
//

#ifndef PROJ2_IPCCONNECTION_H
#define PROJ2_IPCCONNECTION_H
#include "../utils.h"
#include <string>
#include <iostream>
#include <utility>
#include <boost/asio.hpp>

class IPCConnection {
public:
    IPCConnection(int port) : socket(io_context){
        boost::asio::ip::tcp::endpoint endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), port);
        //socket.open(boost::asio::ip::tcp::v4());
        socket.connect(endpoint);
    };
    void send(std::string message, boost::system::error_code &error){
        auto size_ = (short) size(message);
        // sends size
        boost::asio::write(socket, boost::asio::buffer(&size_, sizeof(size_)), error);
        if(error==boost::asio::error::broken_pipe)
            return;
        // sends message
        boost::asio::write(socket, boost::asio::buffer(message), error);
    };
    std::string read(boost::system::error_code &error){
        short size;
        std::string s;
        boost::asio::read(socket, boost::asio::buffer(&size, sizeof(size)), error);
        if(error==boost::asio::error::eof)
            return s;
        s.resize(size);
        boost::asio::read(socket, boost::asio::buffer(s.data(), size), error);
        return s;
    };
    void close(){
        socket.close();
    }
private:
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::socket socket;
};
#endif //PROJ2_IPCCONNECTION_H
