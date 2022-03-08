//
// Created by allanbs on 26/12/21.
//

#ifndef PROJ2_CONNECTIONHANDLER_H
#define PROJ2_CONNECTIONHANDLER_H

#include <boost/asio.hpp>
#include <thread>
#include <unordered_map>
#include <utility>

#include "../utils.h"
#include "../Network/Connection.h"
#include "../Network/TCPConnection.h"
#include "Peer.h"

namespace ip = boost::asio::ip;
typedef std::unordered_map<peer_address, std::shared_ptr<Connection<KademliaMessageType>>, ip_address_hash> connections_map;

class Peer;

class ConnectionHandler {
public:
    explicit ConnectionHandler(std::shared_ptr<Peer> peer_ref, peer_address local_endpoint) :
            peer_reference(peer_ref),
            local_endpoint(local_endpoint),
            acceptor_(io_context, ip::basic_endpoint<ip::tcp>(ip::make_address(local_endpoint.ip), local_endpoint.port))
    {}

    void run();
    void stop();
    void update();
    bool connect(peer_address endpoint);
    void disconnect(peer_address endpoint);
    void send(peer_address endpoint, byte *bytes, size_t size);
    void send(peer_address endpoint, Message<KademliaMessageType> message);
    void handle_n_messages(peer_address endpoint, size_t max_messages = -1);
    void print_connection();
    bool connection_exists(peer_address endpoint);
    tsqueue<Message<KademliaMessageType>>& get_msg_replies(){return msg_replies;};
private:
    bool is_running = false;
    std::mutex mutex;
    tsqueue<Message<KademliaMessageType>> msg_replies;

    std::shared_ptr<Peer> peer_reference;
    connections_map connections;
    boost::asio::io_context io_context;
    std::thread context_t;
    peer_address local_endpoint;

    boost::asio::ip::tcp::acceptor acceptor_;
    [[noreturn]] void check_connections();
    void handle_accept();
    void handle_message(Message<KademliaMessageType> msg, peer_address endpoint);

};


#endif //PROJ2_CONNECTIONHANDLER_H