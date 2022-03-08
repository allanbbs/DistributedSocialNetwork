//
// Created by allanbs on 05/01/22.
//

#ifndef PROJ2_P2P_LAYER_UTILS_H
#define PROJ2_P2P_LAYER_UTILS_H
#include "../lib/Message.h"
#include "Kademlia/kademlia_serialize.h"
#include "Kademlia/p2p_layer.h"
#include <iostream>
#include <iomanip>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <algorithm>
#include <utility>

const char* hex_char_to_bin(char c);

std::string hex_str_to_bin_str(const std::string& hex);

UUID sha256(peer_address p_address);

UUID hex_to_uuid(std::string hex);

std::string uuid_to_hex(UUID id);

std::vector<byte> string_as_bytes(std::string str);

std::string bytes_as_string(std::vector<byte>& bytes);

std::vector<byte> uuid_as_bytes(UUID id);

UUID bytes_as_uuid(std::vector<byte> bytes);

std::vector<byte> entry_as_bytes(UUID id,peer_address address);

std::pair<UUID,peer_address> bytes_as_entry(std::vector<byte> bytes);


template<typename first,typename second>
std::vector<byte> map_as_bytes(std::unordered_map<first,second> map){
    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    oa & map;
    return string_as_bytes(ss.str());
}

template<typename first,typename second>
std::unordered_map<first,second> bytes_as_map(std::vector<byte> bytes ){
    std::unordered_map<first,second> temp;
    std::stringstream ss;
    ss << bytes_as_string(bytes);
    boost::archive::binary_iarchive ia(ss);
    ia >> temp;
    return temp;
}


template<typename T>
std::vector<byte> generic_as_bytes(T data){
    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    oa & data;
    return string_as_bytes(ss.str());
}

template<typename T>
T bytes_as_generic(std::vector<byte> bytes ){
    T temp;
    std::stringstream ss;
    ss << bytes_as_string(bytes);
    boost::archive::binary_iarchive ia(ss);
    ia >> temp;
    return temp;
}

#endif //PROJ2_P2P_LAYER_UTILS_H
