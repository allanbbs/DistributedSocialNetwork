//
// Created by allanbs on 10/12/21.
//

#include "../../lib/Kademlia/Peer.h"

#include "../../lib/Kademlia/KBucket.h"
#include "../../lib/Message.h"
#include "../../lib/Kademlia/kademlia_serialize.h"
#include "../../lib/App/app_serialization.h"
#include "../../lib/Kademlia/p2p_layer.h"
#include "../../lib/p2p_layer_utils.h"
#include <iostream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <algorithm>
#include <utility>


/*
 * Prints peer table
 */
void print_peer_table(std::unordered_map<UUID,peer_address>& k){
    std::cout<<"------------------------------------"<<std::endl;
    for(auto &p : k)
        std::cout<<p.second<<std::endl;
    std::cout<<"------------------------------------"<<std::endl;
}

/*
 * Prints own kbuckets
 */
void Peer::print_kbuckets(){
    /*
    std::cout<<"------------------------------------"<<std::endl;
    for(auto &p : kbuckets){
        for(auto& el : p.nodes){
            std::cout<<el.second<<std::endl;
        }
    }
    std::cout<<"------------------------------------"<<std::endl;
     */
}



Peer::Peer(UUID id, const peer_address& address, peer_address boot_peer_address):
address(address),
conn_handler_ptr(std::make_shared<UDPConnectionHandler> (std::shared_ptr<Peer>(this), address))
{
    peer_id = id;
    //peer_table.insert(std::make_pair(id,address));
    std::thread ([&]{ conn_handler_ptr->run(); }).detach();
    //std::cout<<"Hash:"<<sha256(address)<<std::endl;
    //join(std::move(boot_peer_address));
}

Peer::Peer(const peer_address& address) :
        address(address)/*,
        conn_handler_ptr(std::make_shared<UDPConnectionHandler> (std::shared_ptr<Peer>(this), address))*/
{
    peer_id = sha256(address);
    //std::cout<<"Hash:"<<sha256(address)<<std::endl;
    for(size_t i = 0; i < 256;i++){
        kbuckets.emplace_back(peer_id,address,K,i);
    }
    /*
    std::thread ([&]{ conn_handler_ptr->run(); }).detach();
    std::thread ([&]{ this->conn_handler_ptr->update();}).detach();
     */
}

/*
 * "Runs" the peer
 */
void Peer::run(bool independent){
    conn_handler_ptr = std::make_shared<UDPConnectionHandler>(shared_from_this(),address);
    if(!independent) {
        std::thread([&] { conn_handler_ptr->run(); }).detach();
        std::thread([&] { this->conn_handler_ptr->update(); }).detach();
    }
    else{
        std::thread t1 ([&] { conn_handler_ptr->run(); });
        std::thread t2([&] { this->conn_handler_ptr->update(); });
        t1.join();
        t2.join();
    }
}

/*
 * Get reply for message type "type", while waiting for at most P2P_LAYER_TIMEOUT
 * This function is intended to make this layer(P2P) synchronous, even though the underlying network
 * layer is asynchronous. This is a decision to simplify the implementation and avoid rethinking the message class/struct to
 * better suit a completely asynchronous layer.
 */
Message<KademliaMessageType> Peer::get_reply(KademliaMessageType type){
    std::scoped_lock lock(mutex);
    tsqueue<Message<KademliaMessageType>> temp;
    Message<KademliaMessageType> k;
    k.get_header().id = NONE;
    std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
    while((std::chrono::steady_clock::now() - t1) < std::chrono::milliseconds(P2P_LAYER_TIMEOUT)){
        if(conn_handler_ptr->get_msg_replies().empty())
            conn_handler_ptr->get_msg_replies().front_to_back(temp);
        if(!conn_handler_ptr->get_msg_replies().empty())
            k = conn_handler_ptr->get_msg_replies().pop_front();
        if(k.get_id() == type)
            break;
        else if(k.get_id() != NONE)
            temp.push_back(k);
    }
    conn_handler_ptr->get_msg_replies().front_to_back(temp);
    if(k.get_id() != type){
        return {NONE,std::vector<byte>()};
    }
    return k;
}


/*
 * Joins the network by using peer at endpoint "endpoint" as bootstrapping peer and updates
 * own kbuckets with received routing table afterwards
 */
void Peer::join(peer_address endpoint) {
    Message<KademliaMessageType> msg(JOIN, entry_as_bytes(peer_id,address));
    auto reply = send_and_wait(endpoint,msg);
    if(reply.second && reply.first.get_id() != NONE){
        std::cout << "Joined network" << std::endl;
        std::stringstream ss;
        ss << bytes_as_string(reply.first.get_body());
        boost::archive::binary_iarchive ia(ss);
        std::unordered_map<UUID,peer_address> temp;
        ia >> temp;
        for(auto x: temp){
            update_kbucket_with_entry(x.first,x.second);
        }
    }
    else{
        std::cout << "Unable to join the network, please try again later!" << std::endl;
    }
}

/*
 * Join handler : adds joining peer to kbucket and answer with own routing table(kbuckets)
 */
void Peer::handle_join(Message<KademliaMessageType> msg, peer_address endpoint) {
    // Handles incoming join, reply with routing table
    std::pair<UUID ,peer_address> joining_peer = bytes_as_entry(msg.get_body());
    std::cout <<"Remote peer id:"<< joining_peer.first << std::endl;
    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    std::unordered_map<UUID,peer_address> temp_map;
    temp_map.insert(std::make_pair(peer_id,address));
    for(auto& kbucket: kbuckets){
        for(auto& peer : kbucket.nodes)
            temp_map.insert(std::make_pair(peer.first,peer.second));
    }
    oa & temp_map;
    auto serialized_table = ss.str();
    //peer_table.insert(std::make_pair ( joining_peer.first, joining_peer.second));
    Message<KademliaMessageType> reply(JOIN_ANSWER, string_as_bytes(serialized_table));
    conn_handler_ptr->send(endpoint, reply);
    update_kbucket_with_entry(joining_peer.first,joining_peer.second);
}

/*
 * Leaves netowrk: Sent a leave message to all peers in the kbuckets.
 */

void Peer::leave() {
    std::cout << "Leaving network" << std::endl;
    // Send leave to all peers in the table
    Message<KademliaMessageType> leave_msg (LEAVE, uuid_as_bytes(peer_id));
    send_to_all(leave_msg);
    conn_handler_ptr->stop();
}

/*
 * LEAVE message handler: removes peer from kbucket
 */
void Peer::handle_leave(Message<KademliaMessageType> msg) {
    std::bitset<256> remote_peer_id (0);
    size_t k = 0;
    for(byte b: msg.get_body()) {
        remote_peer_id.set(k, b);
        k++;
    }
    // Remove leaving peer from peer_table
    remove_from_kbucket(remote_peer_id);
    std::cout << "Removed Peer: " << remote_peer_id << std::endl;
}


/*
 * Searches the network for node identified by id "target_id"
 */

find_answer<peer_address> Peer::lookup(UUID target_id){
    int k = K;
    // get own closest peers
    std::unordered_map<UUID,peer_address> closest_peers = get_k_closest_kbucket(target_id, k);

    std::unordered_map<UUID,peer_address> previous_closest;
    auto it = closest_peers.find(target_id);
    if ( it != closest_peers.end())
        return {true, closest_peers, it->second};

    // While the results are not improved (no closest peers found than the previous search)

    while(closest_peers!=previous_closest){
        previous_closest = closest_peers;
        // send lookup request to k-closest peers and receive the closest peers from the previous peers
        for (const auto& closest_peer: closest_peers)
            closest_peers.merge(findClosest(closest_peer, target_id));
        closest_peers.erase(peer_id); // we don't need our id

        closest_peers = get_k_closest(closest_peers, target_id, k); // from the answered peers, keep the k-closest
        /*for(auto& p : closest_peers){
            std::cout<<"C2: "<<p.second<<std::endl;
        }*/
        if (closest_peers.find(target_id) != closest_peers.end())  // check if it is found
            return {true, closest_peers, closest_peers.find(target_id)->second};
    }

    return {false, closest_peers, {}};
}

/*
 *  Searches the network for the value identified by key "key_id"
 */
find_answer<DHT_VALUE> Peer::find_value(UUID key_id){
    int k = K;
    // get own closest peers
    std::unordered_map<UUID,peer_address> closest_peers = get_k_closest_kbucket( key_id, k);
    std::unordered_map<UUID,peer_address> previous_closest;

    if (has_key(key_id)) {
        return {true, own_entry(), pairs.find(key_id)->second};
    }

    // While the results are not improved (no closest peers found than the previous search)
    while(closest_peers!=previous_closest){
        previous_closest = closest_peers;
        // send lookup request to k-closest peers and receive the closest peers from the previous peers
        for (const auto& closest_peer: closest_peers) {
            auto answer = find_key(closest_peer, key_id);
            if(answer.found) {
                //This block issues a STORE message for the found pair to the closes known peer that did NOT returned the value.
                auto cp = closest_peers;
                cp.merge(answer.k_closest);
                std::vector<std::tuple<UUID,peer_address,UUID>> distances;
                for(auto& p : cp){
                    if(p.first != answer.k_closest.begin()->first){
                        distances.push_back({p.first,p.second, distance_between(p.first,key_id)});
                    }
                    std::sort(distances.begin(), distances.end(),
                              [](const std::tuple<UUID,peer_address ,UUID> & a, const std::tuple<UUID,peer_address ,UUID> & b) -> bool
                              {
                                  return std::get<2>(a) < std::get<2>(b);
                              });
                    if(distances.size() > 0) {
                        std::pair<UUID, DHT_VALUE> pair(key_id, answer.data);
                        Message<KademliaMessageType> message = {STORE, generic_as_bytes(pair)};
                        store_directly(std::get<1>(distances.at(0)), message);
                    }else{
                        add_pair(key_id,answer.data);
                    }
                }
                return answer;
            }
            closest_peers.merge(answer.k_closest);
        }
        closest_peers.erase(peer_id); // we don't need our id
        closest_peers = get_k_closest(closest_peers, key_id, k); // from the answered peers, keep the k-closest
    }
    return {false, closest_peers, DHT_VALUE{}};
}


/*
 * LOOKUP message handler : Return k_closest know peers to the ky provided in the message, and these peers are sent to the
 * sender as a reply
 */
void Peer::handle_lookup(Message<KademliaMessageType> message, peer_address endpoint) {

    int k = K;
    std::pair<UUID,UUID> p = bytes_as_generic<std::pair<UUID,UUID>>(message.get_body());
    std::unordered_map<UUID,peer_address> own_closest
        = get_k_closest_kbucket(p.second, k);

    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    oa & own_closest;
    auto serialized_table = ss.str();
    Message<KademliaMessageType> answer(LOOKUP_ANSWER, string_as_bytes(serialized_table));
    conn_handler_ptr->send(endpoint,answer);
    update_kbucket_with_entry(p.first,endpoint);

}

/*
 * Asks peer "closest" for the closest peers to "target" it knows.
 */
std::unordered_map<UUID,peer_address> Peer::findClosest(std::pair<UUID,peer_address> closest, UUID target) {
    std::unordered_map<UUID,peer_address> temp;
    std::pair<UUID,UUID> p = {peer_id,target};
    Message<KademliaMessageType> message(LOOKUP, generic_as_bytes(p));

    auto reply = send_and_wait(closest.second,message);
    if(reply.second && reply.first.get_id() != NONE){
        std::stringstream ss2;
        ss2 << bytes_as_string(reply.first.get_body());
        boost::archive::binary_iarchive ia(ss2);
        ia >> temp;
        /*std::cout<<"Lookup answer:"<<std::endl;
        std::cout<<reply.to_string()<<std::endl;
        */
        return temp;
    }
    else{
        remove_from_kbucket(closest.first);
        std::cout<<"Error when looking up at "<<closest.second<<std::endl;
        return {};
    }
}

/*
 * Send find_value message to peer "closest" and waits for the reply.
 */
find_answer<DHT_VALUE> Peer::find_key(std::pair<UUID,peer_address> closest, UUID target){
    find_answer<DHT_VALUE> temp;
    std::pair<UUID ,UUID > p = {peer_id,target};
    Message<KademliaMessageType> message(FIND_VALUE, generic_as_bytes(p));
    auto reply = send_and_wait(closest.second,message);

    if(reply.second && reply.first.get_id() != NONE){
        /*std::cout<<"Find answer:"<<std::endl;
        std::cout<<reply.to_string()<<std::endl;*/
        std::stringstream ss2;
        ss2 << bytes_as_string(reply.first.get_body());
        boost::archive::binary_iarchive ia(ss2);
        ia >> temp;
        return temp;
    }
    else{
        remove_from_kbucket(closest.first);
        std::cout<<"Error looking for key at "<<closest.second<<std::endl;
        return {};
    }
}

/*
 * FIND_VALUE message handler
 */
void Peer::handle_find_value(Message<KademliaMessageType> message, peer_address endpoint) {
    int k = K;
    //Parses message body
    std::pair<UUID ,UUID > p = bytes_as_generic<std::pair<UUID ,UUID >>(message.get_body());

    //Calculate closest peers to key to be sent as reply if key is not found locally
    std::unordered_map<UUID,peer_address> own_closest
            = get_k_closest_kbucket(p.second, k);

    bool found=false;
    find_answer<DHT_VALUE> answer_;
    //If peer has key-value pair locally, return the pair and set found to true, else return k_closest and set found to false
    if(has_key(p.second)){
        found=true;
        answer_ = {found, own_entry(), pairs.find(p.second)->second};
    }else{
        answer_ = {found, own_closest, DHT_VALUE{}};
    }

    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    oa & answer_;
    auto serialized_answer = ss.str();
    Message<KademliaMessageType> answer(FIND_VALUE_ANSWER, string_as_bytes(serialized_answer));
    conn_handler_ptr->send(endpoint,answer);
    update_kbucket_with_entry(p.first,endpoint);

}

/*
 * Gets the k closest peers to id in "src"
 */
std::unordered_map<UUID,peer_address> Peer::get_k_closest(std::unordered_map<std::bitset<256>, ip_address> src,
                                                          UUID id, size_t k) {
    std::unordered_map<UUID,peer_address> map;
    std::vector<std::pair<UUID,UUID>> distances;
    for(auto& pair : src)
        distances.push_back(std::make_pair(pair.first, distance_between(pair.first,id)));

    std::sort(distances.begin(), distances.end(),
              [](const std::pair<UUID,UUID> & a, const std::pair<UUID,UUID> & b) -> bool
            {
                return a.second < b.second;
            });
    for(size_t i = 0;i < std::min(k, distances.size()) ;i++){
        UUID temp = distances.at(i).first;
        map.insert(std::make_pair(UUID (temp),(src.find(temp)->second)));
    }
    return map;
}

/*
 * Get the k closest peers to "id" in the kbuckets
 */
std::unordered_map<UUID,peer_address> Peer::get_k_closest_kbucket(UUID id, size_t k) {

    std::unordered_map<UUID,peer_address> map;
    std::vector<std::pair<std::pair<UUID,peer_address >,UUID>> distances;

    for(auto& kbucket : kbuckets){
        for(auto node : kbucket.nodes){
            distances.push_back(std::make_pair(std::make_pair(node.first,node.second), distance_between(node.first,id)));
        }
    }
    std::sort(distances.begin(), distances.end(),
              [](const std::pair<std::pair<UUID,peer_address >,UUID> & a, const std::pair<std::pair<UUID,peer_address >,UUID> & b) -> bool
              {
                  return a.second < b.second;
              });
    for(size_t i = 0;i < std::min(k, distances.size()) ;i++){
        UUID temp = distances.at(i).first.first;
        map.insert(std::make_pair(UUID (temp),(distances.at(i).first.second)));
    }
    return map;
}

//PING

/*
 * Pings a peer and waits for a response
 */
void Peer::ping(UUID remote_peer_id){
    auto msg = Message<KademliaMessageType>(PING,std::vector<byte>());
    conn_handler_ptr->send(peer_table[remote_peer_id],msg);
    auto reply = get_reply(PONG);
    if(reply.get_id() != NONE){
        std::cout<<"Peer is online!"<<std::endl;
    }else{
        std::cout<<"Peer is offline!"<<std::endl;
    }
}

/*
 * PING message handler
 */
void Peer::handle_ping(Message<KademliaMessageType> message,peer_address endpoint){
    auto answer = Message<KademliaMessageType>(PONG,std::vector<byte>());
    conn_handler_ptr->send(endpoint,answer);
}

// MISC

/*
 * Send "message" to all peers that are present in the kbuckets
 */
void Peer::send_to_all(Message<KademliaMessageType> message) {
    for(auto& kbucket : kbuckets){
        for(auto pair : kbucket.nodes)
            conn_handler_ptr->send(pair.second, message);
    }
}


//STORE  [DHT]
/*
 * Execute the STORE procedure defined in the Kademlia specification : we search for the closest peers to the key, and send store messages with the key-value pair to the peers
 */
// ID: ip_address,followers,
void Peer::store(UUID key,DHT_VALUE data){
    auto closest = lookup(key);
    for(auto peer : closest.k_closest){
        std::cout<<"STORE:::"<<peer.second<<std::endl;
    }
    add_pair(key,data);
    print_kbuckets();
    if(!closest.found){
        std::pair<UUID,DHT_VALUE> pair(key,data);
        Message<KademliaMessageType> message = {STORE, generic_as_bytes(pair)};
        for(auto peer : closest.k_closest){
            if(data.original_publisher != peer.first) {
                store_directly(peer.second,message,peer.first);
            }
        }
    }
    else{
        std::cout<<"Found node with same ID as message.Something went wrong!"<<std::endl;
    }
}

/*
 * Handler to be called after receiving a STORE message.
 */
void Peer::handle_store(Message<KademliaMessageType> message, peer_address endpoint){
    std::pair<UUID,DHT_VALUE> to_store = bytes_as_generic<std::pair<UUID,DHT_VALUE>>(message.get_body());
    //Add key-value pair to local piece of the DHT
    add_pair(to_store.first,to_store.second);
    Message<KademliaMessageType> reply;
    if(true){ //For our purposes, this will never fail
        //std::cout<<"Peer "<<peer_id<<" storing data"<<std::endl;
        reply = {STORE_ANSWER, string_as_bytes("TRUE")};
    }else{
        reply = {STORE_ANSWER, string_as_bytes("FALSE")};
    }
    conn_handler_ptr->send(endpoint,reply);
}

/*
 * Send the STORE "message" directly to the peer at endpoint "endpoint"
 */
void Peer::store_directly(peer_address endpoint, Message<KademliaMessageType> message,UUID id){
    auto reply = send_and_wait(endpoint, message);
    if (reply.second && reply.first.get_id() != NONE) {
        if (bytes_as_string(reply.first.get_body()) == "TRUE") {
            std::cout << "STORE SUCCESSFUL" << std::endl;
        } else {
            std::cout << "STORE UNSUCCESSFUL" << std::endl;
        }
    } else {
        if(id != UUID(0)){
            remove_from_kbucket(id);
        }
        std::cout << "STORE REQUEST FOR PEER AT " << endpoint << " FAILED!" << std::endl;
    }
}

// SENDING OF MESSAGES

/*
 * Send a message and wait for reply, if a reply is needed at all. Returns a pair {reply(message),got_reply(bool)}
 * got_reply is true if the message needed a reply and a valid reply was returned or if the message does not need a reply at all.
 * got_reply is false if the message needs a reply but no reply was received after TRIES retransmits of the message.
 */
std::pair<Message<KademliaMessageType>,bool> Peer::send_and_wait(peer_address endpoint,Message<KademliaMessageType> message){
    Message<KademliaMessageType> reply;

    //If the message type needs a reply, send message and wait P2P_LAYER_TIMEOUT for reply. If reply is not received,
    //retransmit for up to TRIES times.
    if(needs_reply(message.get_header().id)) {
        for (size_t try_no = 0; try_no < TRIES; try_no++) {
            this->send(endpoint, message);
            reply = get_reply((KademliaMessageType) (message.get_header().id + 1));
            if(reply.get_id() != NONE){
                return {reply,true};
            }
        }

        return {reply,false};
    }
    //If message type does not need a reply, just send the message
    else{
        this->send(endpoint, message);
    }
    return {reply,true};
}



/*
 * Sends a message to the peer at endpoint "endpoint"
 */
void Peer::send(peer_address endpoint, Message<KademliaMessageType> message) {
    conn_handler_ptr->send(endpoint, message);
}


// APP MESSAGES
/*
 * Forwards APPLICATION messages to the application queues, to be accessed on-demand by the application layer.
 */
void Peer::handle_app_messages(Message<KademliaMessageType> message,peer_address endpoint){
    Message<DTMessageType> msg;
    msg.from_bytes(message.get_body().data()); //AN APPLICATION message is expected to contain a Message< <GENERIC_MESSAGE_TYPE> > encoded as bytes in its body
    if(message.get_id() == APPLICATION_REQUEST)
        application_requests.push_back(std::make_pair(msg,endpoint));
    else
        application_replies.push_back(std::make_pair(msg,endpoint));
}




//KBUCKETS FUNCTIONALITY


/*
 * Updates relevant kbucket with given entry : Adds if space is available or if the entry is closer to
 * current peer than an entry already in the kbucket. If the entry already exists in the kbucket, the entry is simply moved to the
 * end of that kbucket.
 */
bool Peer::update_kbucket_with_entry(UUID id,peer_address endpoint){
    for(auto& kbucket : kbuckets){
        if(kbucket.has_node(id)) {
            return kbucket.add_or_update_node(id,endpoint);
        }
    }
    return false;
}

/*
 * Remove a peer identified by "id" from the respective kbucket, if such a peer exists in the kbucket.
 */
bool Peer::remove_from_kbucket(UUID id){
    for(auto& kbucket : kbuckets){
        if(kbucket.has_node(id)) {
            return kbucket.remove_node(id);
        }
    }
    return false;
}