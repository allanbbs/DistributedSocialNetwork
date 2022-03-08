//
// Created by allanbs on 20/12/21.
//
#include "../lib/utils.h"
#include <boost/serialization/serialization.hpp>
#include <iostream>
#include <fstream>


std::ostream& operator<<(std::ostream& out,const peer_address& p){
    out<<p.ip<<":"<<p.port;
    return out;
}

bool operator==(const ip_address &lhs, const ip_address &rhs) {
    return (lhs.ip == rhs.ip) && (lhs.port == rhs.port);
}

void debugging_info(const char *file, const size_t line) {
    std::cerr << file << ":" << line << std::endl;
}

bool is_kademlia_reply(KademliaMessageType type){
    return (type % 2) == 0;
}

bool needs_reply(KademliaMessageType type){
    return (type!=LEAVE && type!=APPLICATION_REQUEST && type!=NONE && type!= APPLICATION_REPLY);
}


bool operator<(const UUID& x, const UUID& y)
{
    size_t N = 256;
    for (int i = N-1; i >= 0; i--) {
        if (x[i] ^ y[i]) return y[i];
    }
    return false;
}

bool operator<=(const UUID& x, const UUID& y) {
    return x == y || x < y;
}

void bytes_to_string(byte* bytes,size_t size){
    std::string r="";
    for (size_t i = 0;i<size;i++)
        r+= "-" + std::to_string((int)bytes[i]);
    std::cout<<r<<std::endl;
}

UUID distance_between(UUID id_1, UUID id_2) {
    return (id_1 ^ id_2);
}



