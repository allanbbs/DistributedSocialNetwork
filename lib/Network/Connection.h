//
// Created by allanbs on 10/12/21.
//

#ifndef PROJ2_CONNECTION_H
#define PROJ2_CONNECTION_H

#include <utility>
#include <chrono>
#include "../utils.h"
#include "../tsqueue.h"
#include "../Message.h"

template<typename MSG_TYPE>
class Connection {

    protected:
        std::chrono::steady_clock::time_point start_time;
        std::chrono::milliseconds expiry_time;
        peer_address local_endpoint;
        peer_address remote_endpoint;
    public:
        virtual void send_bytes(byte* buffer, size_t size) = 0;
        virtual void send(Message<MSG_TYPE> &message) = 0;
        virtual void close() = 0;
        virtual void start_read() = 0;
        void reset_start_time() { start_time = std::chrono::steady_clock::now(); };
        void set_expiry_period(std::chrono::milliseconds time) { expiry_time = time; };
        bool is_expired() { return (std::chrono::steady_clock::now() - start_time) > expiry_time; };
        std::chrono::steady_clock::time_point get_start_time() const{return start_time;};
        virtual bool is_open() = 0;
        virtual tsqueue<Message<MSG_TYPE>>& incoming() = 0;
        Connection(peer_address local,peer_address remote) : local_endpoint(std::move(local)),remote_endpoint(std::move(remote)) {};
        peer_address get_local_endpoint() const{return local_endpoint;};
        peer_address get_remote_endpoint() const{return remote_endpoint;};
};


#endif //PROJ2_CONNECTION_H
