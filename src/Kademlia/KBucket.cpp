//
// Created by diogo on 30/12/21.
//

#include "../../lib/Kademlia/KBucket.h"

/*
 * Checks if node can belong to the bucket
 *
 * Returns true if in range [2 XOR j; 2 XOR J+1[ and false otherwise
 */
bool KBucket::has_node(UUID node_id) {
    UUID distance = distance_between(node_id, peer_id);
    return (lower_limit <= distance) && (distance < upper_limit);
}
/*
 * Add a new node to the bucket or update the bucket by putting the
 * referenced node at the end of the bucket (used when communications are made with nodes)
 *
 * Returns true if successful
 * Returns false otherwise (bucket full and with closer nodes)
 */
bool KBucket::add_or_update_node(UUID peer_id, peer_address address) {
    auto itr = nodes.find(peer_id);
    if(size() < k) {
        if (itr == nodes.end())
            nodes.insert(std::make_pair(peer_id, address));
        else
            move_node_to_end(itr, peer_id, address);
        return true;
    }
    else {
        UUID max_distance(0);
        std::unordered_map<UUID, peer_address>::iterator to_replace;

        for(auto i = nodes.begin(); i != nodes.end(); i++) {
            UUID distance = distance_between(i->first, this->peer_id);
            if(max_distance < distance) {
                max_distance = distance;
                to_replace = i;
            }
        }
        if(distance_between(peer_id, this->peer_id) < max_distance) {
            nodes.erase(to_replace);
            nodes.insert(std::make_pair(peer_id, address));
            return true;
        }
    }
    return false;
}

/*
 * Removes node from the bucket
 *
 * Returns true if node removed successfully
 * Returns false if node doesn't exist in the kbucket
 */
bool KBucket::remove_node(UUID peer_id) {
    auto el = nodes.find(peer_id);
    if (el != nodes.end()) {
        nodes.erase(el);
        return true;
    }
    return false;
}

void KBucket::move_node_to_end(std::unordered_map<UUID, peer_address>::iterator itr,
                               UUID node_id, peer_address node_address) {
    nodes.erase(itr);
    nodes[node_id] = node_address;
}

/*
 * Gets the node that hasn't communicated for the longest time
 */
std::pair<UUID, peer_address> KBucket::get_oldest_node() {
    std::unordered_map<UUID, peer_address>::iterator last_element;
    for (auto i = nodes.begin(); i != nodes.end(); i++) {
        last_element = i;
    }
    return std::make_pair(last_element->first, last_element->second);
}
