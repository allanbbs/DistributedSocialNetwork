//
// Created by allanbs on 19/12/21.
//

#ifndef PROJ2_UTILS_H
#define PROJ2_UTILS_H
#include <bitset>
#include <string>
#include <iostream>
#include <unordered_map>
#include <memory>
#include <boost/asio.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/vector.hpp>

typedef std::bitset<256> UUID;
typedef unsigned char byte;
typedef unsigned long long ull;

enum KademliaMessageType{
    NONE = 0,
    JOIN   = 1,
    JOIN_ANSWER = 2,
    LEAVE  = 3,
    LOOKUP = 5,
    LOOKUP_ANSWER = 6,
    PING = 7,
    PONG = 8,
    FIND_VALUE = 9,
    FIND_VALUE_ANSWER = 10,
    STORE = 11,
    STORE_ANSWER = 12,
    APPLICATION_REQUEST = 13,
    APPLICATION_REPLY = 15
};

bool is_kademlia_reply(KademliaMessageType type);

bool needs_reply(KademliaMessageType type);

typedef struct ip_address{
    std::string ip;
    uint16_t port;
}peer_address;

template<typename DATA_TYPE>
struct find_answer{
    bool found = false;
    std::unordered_map<UUID,peer_address> k_closest;
    DATA_TYPE data;
};

struct setup_config{
    bool bootstrap_peer = false;
    std::vector<peer_address> bootstraping_peers;
    peer_address local_endpoint;
};

struct ip_address_hash
{
    auto operator()( const ip_address& x ) const
    { return std::hash< std::string >()( x.ip+std::to_string(x.port)); }
};

bool operator==(const ip_address& lhs,const ip_address& rhs);
std::ostream& operator<<(std::ostream& out,const peer_address& p);
void debugging_info(const char * file,const size_t line);


void bitsetAdd(std::bitset<256> x, const std::bitset<256> y);
bool fullAdder(bool b1, bool b2, bool& carry);

bool operator<(const UUID& x, const UUID& y);

bool operator<=(const UUID& x, const UUID& y);

void bytes_to_string(byte* bytes,size_t size);

// Distance between peers (id_1 XOR id_2)
UUID distance_between(UUID id_1, UUID id_2);
#endif //PROJ2_UTILS_H
