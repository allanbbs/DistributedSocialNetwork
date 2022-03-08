//
// Created by breno on 05/01/22.
//

#ifndef PROJ2_APP_H
#define PROJ2_APP_H

#include "../Kademlia/Peer.h"
#include "Storage.h"
#include <vector>

template <typename DATA_TYPE>
class App : public std::enable_shared_from_this<App<DATA_TYPE>> {
private:
    bool is_running;
    std::mutex mutex;
    size_t message_counter;
    std::condition_variable cvBlocking;
    std::mutex muxBlocking;
    std::vector<std::pair<UUID,peer_address >> own_subscribers;
    std::unordered_map<UUID,std::vector<std::pair<UUID,peer_address>>> subscribed;
    std::vector<UUID> pending_subscribes;

    // Interface
    std::shared_ptr<std::condition_variable> cv_interface;
    void launch_interface();

    // Handlers
    void handle_subscribe(std::pair<Message<DTMessageType>, ip_address> p);
    void handle_unsubscribe(std::pair<Message<DTMessageType>, ip_address> p);
    void handle_get(std::pair<Message<DTMessageType>, ip_address> p);

    // Operations
    void update();
    void event_handler();

    // Auxiliary
    size_t get_message_counter(UUID user_id);

public:
    Storage<UUID,std::vector<DTMessage<DATA_TYPE>>> storage;
    std::shared_ptr<Peer> peer_ref;
    App(setup_config& config) : is_running(false){
        message_counter = 1;
        storage = {(config.local_endpoint.ip + std::to_string(config.local_endpoint.port)) + ".1",(config.local_endpoint.ip + std::to_string(config.local_endpoint.port)) + ".2"};

        peer_ref = std::make_shared<Peer>(config.local_endpoint);
        peer_ref->run();
        if(!config.bootstrap_peer){
            for(auto& peers: config.bootstraping_peers)
                peer_ref->join(peers);
        }
        auto key_value_pair = peer_ref->find_value(peer_ref->get_peer_id());
        if(key_value_pair.found){
            DHT_VALUE data = key_value_pair.data;
            for(auto& pair : data.followers) {
                if(pair.first != peer_ref->get_peer_id()){
                    own_subscribers.push_back({pair.first, pair.second});
                    notify_subscriber(pair.first);
                }
            }
        }else{
            std::unordered_map<UUID,peer_address> temp;
            temp.insert({peer_ref->get_peer_id(),peer_ref->get_address()});
            peer_ref->store(peer_ref->get_peer_id(),{peer_ref->get_peer_id(),temp});
        }
        for(auto &p : storage.own_subscribers){
            subscribe(p);
        }

        message_counter = get_message_counter(peer_ref->get_peer_id())+1;
    }

    void run();

    void stop();

    void publish(DATA_TYPE data);

    void subscribe(UUID user_id);

    void unsubscribe(UUID user_id);

    void get(UUID source_id, size_t counter);

    void leave();

    Message<DTMessageType> get_reply(DTMessageType type);

    void notify_interface();

    void publish_push(DATA_TYPE data);

    void handle_push(std::pair<Message<DTMessageType>, ip_address> p);

    void notify_subscriber(UUID user_id);

    void handle_notify(std::pair<Message<DTMessageType>, ip_address> p);
};

#endif //PROJ2_APP_H
