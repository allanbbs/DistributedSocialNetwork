//
// Created by diogo on 30/12/21.
//

#ifndef PROJ2_KBUCKET_H
#define PROJ2_KBUCKET_H

#include "../utils.h"

class KBucket {
private:
    int k;
    UUID upper_limit, lower_limit;
    UUID peer_id;
    peer_address address;


public:
    std::unordered_map<UUID, peer_address> nodes;
    KBucket(UUID peer_id, peer_address address, int k, int j): peer_id(peer_id), address(address), k(k),j(j)
    {
        UUID aux(1);
        upper_limit = aux << (j + 1);
        lower_limit = aux << j;

    }
    int j;
    bool has_node(UUID node_id);
    bool add_or_update_node(UUID peer_id, peer_address address);
    bool remove_node(UUID peer_id);
    void move_node_to_end(std::unordered_map<UUID, peer_address>::iterator itr,
                          UUID node_id, peer_address node_address);

    int size() {
        return this->nodes.size();
    }

    std::pair<UUID, peer_address> get_oldest_node();
};


#endif //PROJ2_KBUCKET_H
