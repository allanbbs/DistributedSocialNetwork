//
// Created by allanbs on 10/12/21.
//

#ifndef PROJ2_PEER_H
#define PROJ2_PEER_H

#include <sys/types.h>
#include <map>
#include <string>
#include <utility>
#include <cmath>
#include "../utils.h"
#include "ConnectionHandler.h"
#include "UDPConnectionHandler.h"
#include "../App/application_layer.h"
#include <unordered_set>
#include "KBucket.h"

class ConnectionHandler;
class UDPConnectionHandler;

class Peer : public std::enable_shared_from_this<Peer> {
private:
    UUID peer_id;
    std::string user_id;
    std::mutex mutex;
    peer_address address;
    std::unordered_map<UUID,DHT_VALUE> pairs;
    find_answer<DHT_VALUE> find_key(std::pair<UUID,peer_address> closest, UUID target);
    void send_to_all(Message<KademliaMessageType> message);


public:

    std::vector<KBucket> kbuckets;
    std::unordered_map<UUID, peer_address> peer_table;
    std::shared_ptr<UDPConnectionHandler> conn_handler_ptr;
    tsqueue<std::pair<Message<DTMessageType>,peer_address>> application_requests;
    tsqueue<std::pair<Message<DTMessageType>,peer_address>> application_replies;
    std::unordered_map<UUID, peer_address> own_entry(){
        return {{peer_id,address}};
    };
    // Constructors

    Peer(UUID id, const peer_address& address, peer_address boot_peer_address);

    Peer(const peer_address& address);

    Peer(UUID peer_id, std::string user_id, std::unordered_map<UUID, peer_address> map) :
            peer_id(peer_id), user_id(std::move(user_id)), peer_table(std::move(map)) {};

    std::unordered_map<UUID, peer_address> get_k_closest(std::unordered_map<std::bitset<256>, ip_address> src, UUID id,size_t k);

    // Getters

    UUID get_peer_id() const { return peer_id; };

    std::string get_user_id() const { return user_id; };

    const std::unordered_map<UUID, peer_address> &get_peer_table() const { return peer_table; };

    peer_address get_address() const { return address; }

    // Key handlers

    void add_pair(UUID key,DHT_VALUE data) { pairs.insert_or_assign(key,data); };

    void remove_key(UUID key) { pairs.erase(key); };

    bool has_key(UUID key) { return pairs.find(key) != pairs.end(); };


    // Send and receive

    void join(peer_address endpoint);

    void handle_join(Message<KademliaMessageType> type, peer_address endpoint);

    void handle_ping(Message<KademliaMessageType> message,peer_address endpoint);

    void ping(UUID remote_peer_id);

    void leave();

    void handle_leave(Message<KademliaMessageType> msg);

    //find_answer<DATA_TYPE> lookup(UUID id);

    void handle_lookup(Message<KademliaMessageType> message, peer_address endpoint);

    std::unordered_map<UUID,peer_address> findClosest(std::pair<UUID,peer_address> closest, UUID target);

    Message<KademliaMessageType> get_reply(KademliaMessageType type);

    find_answer<peer_address> lookup(UUID target_id);

    void handle_find_value(Message<KademliaMessageType> message, peer_address endpoint);

    find_answer<DHT_VALUE> find_value(UUID key_id);

    void store(UUID key, DHT_VALUE data);

    void handle_store(Message<KademliaMessageType> message, peer_address endpoint);

    void handle_app_messages(Message<KademliaMessageType> message, peer_address endpoint);

    void send(peer_address, Message<KademliaMessageType> message);

    void run(bool independent = false);

    std::unordered_map<UUID, peer_address> get_k_closest_kbucket(UUID id, size_t k);

    void print_kbuckets();

    bool update_kbucket_with_entry(UUID id, peer_address endpoint);

    std::pair<Message<KademliaMessageType>,bool> send_and_wait(peer_address endpoint, Message<KademliaMessageType> message);

    bool remove_from_kbucket(UUID id);

    void store_directly(peer_address endpoint, Message<KademliaMessageType> message,UUID id = UUID(0));
};
/**
 *
 func (list,id,mensagem) -> networking
 *
 */

#endif //PROJ2_PEER_H