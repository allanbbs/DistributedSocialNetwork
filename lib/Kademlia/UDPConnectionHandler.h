//
// Created by allanbs on 04/01/22.
//

#ifndef PROJ2_UDPCONNECTIONHANDLER_H
#define PROJ2_UDPCONNECTIONHANDLER_H


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

class UDPConnectionHandler {
public:
    explicit UDPConnectionHandler(std::shared_ptr<Peer> peer_ref, peer_address local_endpoint) :
            peer_reference(peer_ref),
            local_endpoint(local_endpoint),
            acceptor_(io_context, ip::basic_endpoint<ip::udp>(ip::make_address(local_endpoint.ip), local_endpoint.port))
    {
    }

    void run();
    void stop();
    void update();
    void send(peer_address endpoint, Message<KademliaMessageType> message);
    tsqueue<Message<KademliaMessageType>>& get_msg_replies(){return msg_replies;};
private:
    bool is_running = false;
    tsqueue<Message<KademliaMessageType>> msg_replies;
    std::shared_ptr<Peer> peer_reference;
    boost::asio::io_context io_context;
    std::thread context_t;
    peer_address local_endpoint;
    boost::asio::ip::udp::socket acceptor_;
    void handle_message(Message<KademliaMessageType> msg, peer_address endpoint);
    void handle_receive();
    tsqueue<std::pair<Message<KademliaMessageType>,peer_address >> incoming;
    void async_read_message(const boost::system::error_code &error, unsigned int br);
};


#endif //PROJ2_UDPCONNECTIONHANDLER_H
