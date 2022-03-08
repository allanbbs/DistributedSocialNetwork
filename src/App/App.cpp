//
// Created by breno on 05/01/22.
//

#include "../../lib/App/App.h"
#include "../../lib/p2p_layer_utils.h"
#include "../../lib/Network/IPCConnection.h"
#include "../../lib/App/application_layer.h"
#include "../../lib/App/app_serialization.h"
#include "../../lib/App/Interface.h"

template class App<std::string>;

//--------- AUXILIARY

/**
 * Get the most recent message counter of a peer in own storage
 * @tparam DATA_TYPE
 * @param user_id
 * @return The counter of the last message from user_id
 */
template<typename DATA_TYPE>
size_t App<DATA_TYPE>::get_message_counter(UUID user_id) {
    auto k = storage.get(user_id);
    if(k.size() == 0) return 0;
    return k.back().counter;
}

//--------- INTERFACE

/**
 * Launches the interface
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::launch_interface() {
    system("python3 ../src/interface/main.py");
}

/**
 * Notifies the interface of changes in the storage
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::notify_interface() {
    if(cv_interface.get() != nullptr)
        cv_interface->notify_one();
}

//--------- HANDLERS

/**
 * Handles different types of messages from the application requests queue
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::event_handler() {
    while(is_running){
        peer_ref->application_requests.wait();
        while(!peer_ref->application_requests.empty()){
            auto p = peer_ref->application_requests.pop_front();
            if(p.first.get_id() != DT_NONE){
                if      (p.first.get_id() == SUBSCRIBE)
                    handle_subscribe(p);
                else if (p.first.get_id() == UNSUBSCRIBE)
                    handle_unsubscribe(p);
                else if (p.first.get_id() == GET)
                    handle_get(p);
                else if (p.first.get_id() == PUSH)
                    handle_push(p);
                else if (p.first.get_id() == NOTIFY_SUBSCRIBER)
                    handle_notify(p);
            }
        }
    }
}

/**
 * Handles a subscribe request
 * @tparam DATA_TYPE
 * @param p Message and endpoint pair
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::handle_subscribe(std::pair<Message<DTMessageType>, ip_address> p){
    // Gets the subscribing peer from the message body
    auto subscribing_peer = bytes_as_entry(p.first.get_body());
    std::cout<<"Subscribe received:"<<subscribing_peer.second<<std::endl;
    auto it=std::find(own_subscribers.begin(), own_subscribers.end(), subscribing_peer);
    // Checks if it is already subscribed
    if(it==own_subscribers.end()) {
        own_subscribers.emplace_back(subscribing_peer.first, subscribing_peer.second);
        std::unordered_map<UUID,peer_address> temp;
        temp.insert({peer_ref->get_peer_id(),peer_ref->get_address()});
        for(auto& el : own_subscribers)
            temp.insert({el.first,el.second});
        // Updates the subscribers list in the DHT
        peer_ref->store(peer_ref->get_peer_id(),{peer_ref->get_peer_id(),temp});
    }
    // Sends the response
    Message<DTMessageType> reply = {SUBSCRIBE_ANSWER, std::vector<byte>()};
    peer_ref->conn_handler_ptr->send(p.second,{APPLICATION_REPLY,reply.to_bytes()});
}

/**
 * Handles a get request
 * @tparam DATA_TYPE
 * @param p Message and endpoint pair
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::handle_get(std::pair<Message<DTMessageType>, ip_address> p){
    // Gets the requesting peer from the request body
    auto requesting_peer = bytes_as_map<UUID,size_t>(p.first.get_body());
    std::cout<<"Get received! Counter: "<<requesting_peer.begin()->second<<" from "<< p.second<<std::endl;
    std::vector<DTMessage<DATA_TYPE>> messages;
    UUID temp_id = requesting_peer.begin()->first;
    size_t temp_counter = requesting_peer.begin()->second;

    std::vector<DTMessage<DATA_TYPE>> this_messages = storage.get(temp_id);
    if(!this_messages.empty()){
        // Get the messages that have a higher counter than the one specified in the request
        for(size_t i = 0; i < std::min(MAX_GET_REP,this_messages.size()); i++){
            if(this_messages.at(i).counter > temp_counter)
                messages.push_back(this_messages.at(i));
        }
        // Sends the response
        Message<DTMessageType> reply = {GET_ANSWER, generic_as_bytes(messages)};
        peer_ref->conn_handler_ptr->send(p.second,{APPLICATION_REPLY,reply.to_bytes()});
    }
}

/**
 * Handles a unsubscribe request
 * @tparam DATA_TYPE
 * @param p Message and endpoint pair
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::handle_unsubscribe(std::pair<Message<DTMessageType>, ip_address> p){
    auto unsubscribing_peer = bytes_as_entry(p.first.get_body());
    auto it = std::find(own_subscribers.begin(), own_subscribers.end(), std::pair<UUID, peer_address>{unsubscribing_peer.first, unsubscribing_peer.second});
    // Check if unsubscribing peer is indeed subscribed in this peer
    if(it!=own_subscribers.end()) {
        std::cout<<"Unsubscribe received:"<<unsubscribing_peer.second<<std::endl;
        // Remove from subscribers list
        own_subscribers.erase(it);
        std::unordered_map<UUID, peer_address> temp;
        temp.insert({peer_ref->get_peer_id(), peer_ref->get_address()});
        for (auto &el: own_subscribers)
            temp.insert({el.first, el.second});
        // Updates the subscribers list in the DHT
        peer_ref->store(peer_ref->get_peer_id(), {peer_ref->get_peer_id(), temp});
    }
}

/**
 * Handles a push from a subscribed peer
 * @tparam DATA_TYPE
 * @param p Message and endpoint pair
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::handle_push(std::pair<Message<DTMessageType>, ip_address> p){
    // Adds the new entry in the storage
    std::pair<UUID,DTMessage<std::string>> message = bytes_as_generic<std::pair<UUID,DTMessage<std::string>>>(p.first.get_body());
    storage.add(message.first,message.second);
    notify_interface();
    std::cout<<"Message received: Counter:"<<message.second.counter<<" Body: "<<message.second.body<<"" <<p.second<<std::endl;
}

/**
 * Handles a notify message from a subscribed peer and sends a get request
 * @tparam DATA_TYPE
 * @param p Message and endpoint pair
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::handle_notify(std::pair<Message<DTMessageType>, ip_address> p){
    std::pair<UUID,peer_address> message = bytes_as_entry(p.first.get_body());
    if(subscribed.find(message.first) != subscribed.end()){
        // Get pending messages
        std::cout<<"Peer at "<<message.second<<" is online!"<<std::endl;
        std::cout<<"Getting messages from "<< get_message_counter(message.first)<<std::endl;
        get(message.first,get_message_counter(message.first) );
    }
}

//--------- INTERNAL OPERATIONS

/**
 * Publishes a message and pushes it to own subscribers
 * @tparam DATA_TYPE
 * @param data
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::publish_push(DATA_TYPE data){
    auto now = std::chrono::system_clock::now();
    std::string epoch = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
    DTMessage<DATA_TYPE> message = {message_counter++, epoch, data};
    Message<DTMessageType> m = {PUSH, generic_as_bytes(std::make_pair(peer_ref->get_peer_id(),message))};
    storage.add(peer_ref->get_peer_id(),message);
    for(auto& el : own_subscribers){
        peer_ref->send(el.second,{APPLICATION_REQUEST,m.to_bytes()});
    }
    notify_interface();
}

/**
 * Sends a Get request to peers in subscribers list of source_id
 * @tparam DATA_TYPE
 * @param source_id
 * @param counter
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::get(UUID source_id, size_t counter){
    std::unordered_map<UUID,size_t> get_data;
    // Prepares the Get request
    get_data.insert({source_id,counter});
    Message<DTMessageType> message = {GET,map_as_bytes(get_data)};
    Message<KademliaMessageType> peer_message = {APPLICATION_REQUEST,message.to_bytes()};
    if(subscribed.find(source_id) == subscribed.end()) return;
    for(auto& peer : subscribed.at(source_id)){
        if(peer.first == peer_ref->get_peer_id()) continue; // Does not send to itself
        peer_ref->send(peer.second,peer_message);
        auto reply = get_reply(GET_ANSWER);
        // Checks if peer is responsive
        if(reply.get_id() != DT_NONE){
            std::vector<DTMessage<DATA_TYPE>> messages = bytes_as_generic<std::vector<DTMessage<DATA_TYPE>>>(reply.get_body());
            if(messages.empty()){
                std::cout << "No new messages " << std::endl;
            }else{
                for(auto& m : messages){
                    storage.add(source_id, m);
                    notify_interface();
                }
                break;
            }
        }else{
            // Retransmit the request
            for(size_t i=0; i<3;i++){
                peer_ref->send(peer.second, peer_message);
                auto reply = get_reply(GET_ANSWER);
                if(reply.get_id() != DT_NONE){
                    std::vector<DTMessage<DATA_TYPE>> messages = bytes_as_generic<std::vector<DTMessage<DATA_TYPE>>>(reply.get_body());
                    if(messages.empty()) {
                        std::cout << "No new messages" << std::endl;
                    }else{
                        for(auto& m : messages){
                            storage.add(source_id, m);
                            notify_interface();
                        }
                        break;
                    }
                }
            }
        }
    }
}

/**
 * Sends a subscribe request to user_id
 * @tparam DATA_TYPE
 * @param user_id
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::subscribe(UUID user_id){
    // Perform a lookup and check if it is active
    auto is_online = peer_ref->lookup(user_id);
    if(is_online.found){
        Message<DTMessageType> msg(SUBSCRIBE, entry_as_bytes(peer_ref->get_peer_id(),peer_ref->get_address()));
        peer_ref->send(is_online.data,{APPLICATION_REQUEST,msg.to_bytes()});
    }else{
        // If peer is not online, add it as a pending subscriber
        std::cout<<"Added pending subscribe!"<<std::endl;
        pending_subscribes.push_back(user_id);
    }
    // Gets the user information using the DHT records
    auto info = peer_ref->find_value(user_id);
    if(info.found){
        DHT_VALUE value = info.data;
        std::vector<std::pair<UUID,peer_address>> vec;
        for(auto& p : value.followers)
            vec.emplace_back(p.first,p.second);
        subscribed.insert_or_assign(user_id,vec);
        storage.add_subscriber(user_id);
        get(user_id, get_message_counter(user_id));
    }
    else{
        std::cout<<"Information about requested peer not found in the network"<<std::endl;
    }
}

/**
 * Sends an unsubscribe request for user_id and removes it from own storage
 * @tparam DATA_TYPE
 * @param user_id
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::unsubscribe(UUID user_id){
    if(subscribed.find(user_id)==subscribed.end()) return;
    Message<DTMessageType> msg(UNSUBSCRIBE, entry_as_bytes(peer_ref->get_peer_id(), peer_ref->get_address()));
    peer_ref->send(subscribed.find(user_id)->second.at(0).second,{APPLICATION_REQUEST,msg.to_bytes()});
    storage.unsubscribe(user_id);
    storage.remove(user_id);
    subscribed.erase(user_id);
    notify_interface();
}

/**
 * Notifies own subscribers for our presence in the network
 * @tparam DATA_TYPE
 * @param user_id
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::notify_subscriber(UUID user_id){
    Message<DTMessageType> msg(NOTIFY_SUBSCRIBER, entry_as_bytes(peer_ref->get_peer_id(),peer_ref->get_address()));
    for(auto& p : own_subscribers){
        if(p.first == user_id){
            peer_ref->send(p.second,{APPLICATION_REQUEST,msg.to_bytes()});
            break;
        }
    }
}

/**
 * Leave the network
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::leave() {
    peer_ref->leave();
}

/**
 * Waits for a reply of type
 * @tparam DATA_TYPE
 * @param type
 * @return message
 */
template<typename DATA_TYPE>
Message<DTMessageType> App<DATA_TYPE>::get_reply(DTMessageType type){
    std::scoped_lock lock(mutex);
    tsqueue<std::pair<Message<DTMessageType>,peer_address>> temp;
    std::pair<Message<DTMessageType>,peer_address> k;
    k.first.get_id() = DT_NONE;
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    // Tries for a short period to receive the message
    while((std::chrono::steady_clock::now() - t1) < std::chrono::milliseconds(APP_LAYER_TIMEOUT)){
        if(peer_ref->application_replies.empty())
            peer_ref->application_replies.front_to_back(temp);
        if(!peer_ref->application_replies.empty())
            k = peer_ref->application_replies.pop_front();
        if(k.first.get_id() == type)
            break;
        else if(k.first.get_id() != DT_NONE)
            temp.push_back(k);
    }
    peer_ref->application_replies.front_to_back(temp);
    // If reply not received, returns with NONE
    if(k.first.get_id() != type){
        return {DT_NONE,std::vector<byte>()};
    }
    return k.first;
}

/**
 * Periodically resends pending subscribe requests
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::update(){
    while(is_running){
        auto cp = subscribed;
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.wait_for(ul,std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::milliseconds (UPDATE_INTERVAL)));
        if(!is_running) return;
        for(auto it = pending_subscribes.begin(); it != pending_subscribes.end() ; it++){
            auto is_online = peer_ref->lookup(*it);
            if(is_online.found){
                Message<DTMessageType> msg(SUBSCRIBE, entry_as_bytes(peer_ref->get_peer_id(),peer_ref->get_address()));
                peer_ref->send(is_online.data,{APPLICATION_REQUEST,msg.to_bytes()});
                auto rep = get_reply(SUBSCRIBE_ANSWER);
                if(rep.get_id()!=DT_NONE) {
                    std::cout<<"Pending subscribe for "<<is_online.data<<" resolved!"<<std::endl;
                    pending_subscribes.erase(it);
                    auto info = peer_ref->find_value(*it);
                    if(info.found){
                        DHT_VALUE value = info.data;
                        std::vector<std::pair<UUID,peer_address>> vec;
                        for(auto& p : value.followers)
                            vec.emplace_back(p.first,p.second);
                        subscribed.insert_or_assign(*it,vec);
                        storage.add_subscriber(*it);
                        get(*it, get_message_counter(*it));
                    }
                    else{
                        std::cout<<"Information about requested peer not found in the network"<<std::endl;
                    }
                    break;
                }
            }
        }
    }
}

/**
 * Run the application, starting the interface and handlers
 * @tparam DATA_TYPE
 */
template <typename DATA_TYPE>
void App<DATA_TYPE>::run() {
    is_running = true;
    std::thread i([&]{this->launch_interface();});
    std::thread inter_thread([&]{
        sleep(1);
        Interface<DATA_TYPE> interface(std::shared_ptr<App<DATA_TYPE>> (this->shared_from_this()));
        cv_interface = interface.get_shared_cv();
        interface.run();});
    std::thread t([&]{this->event_handler();});
    std::thread t2([&]{this->update();});
    if(t.joinable())
        t.join();
    if(t2.joinable())
        t2.join();
    if(i.joinable())
        i.join();
    if(inter_thread.joinable())
        inter_thread.join();

}

/**
 * Stops the application cleanly
 * @tparam DATA_TYPE
 */
template<typename DATA_TYPE>
void App<DATA_TYPE>::stop() {
    // Saves the structures in disk
    storage.save_to_disk();
    is_running = false;
    cvBlocking.notify_one();
    leave(); // leaves the network
    peer_ref->application_requests.set_block(false);
    peer_ref->application_requests.notify();
    std::unique_lock<std::mutex> ul(muxBlocking);
}
