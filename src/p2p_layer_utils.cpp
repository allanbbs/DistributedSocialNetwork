//
// Created by allanbs on 05/01/22.
//

#include "../lib/utils.h"
#include <openssl/sha.h>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <iomanip>
#include "../lib/Kademlia/kademlia_serialize.h"
const char* hex_char_to_bin(char c)
{
    switch(toupper(c))
    {
        case '0': return "0000";
        case '1': return "0001";
        case '2': return "0010";
        case '3': return "0011";
        case '4': return "0100";
        case '5': return "0101";
        case '6': return "0110";
        case '7': return "0111";
        case '8': return "1000";
        case '9': return "1001";
        case 'a': return "1010";
        case 'b': return "1011";
        case 'c': return "1100";
        case 'd': return "1101";
        case 'e': return "1110";
        case 'f': return "1111";
        default : return "0000";
    }
}

std::string hex_str_to_bin_str(const std::string& hex)
{
    std::string bin;
    for(unsigned i = 0; i != hex.length(); ++i)
        bin += hex_char_to_bin(hex[i]);
    return bin;
}

UUID sha256(peer_address p_address) {
    std::string line = p_address.ip + ":" + std::to_string(p_address.port);
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, line.c_str(), line.length());
    SHA256_Final(hash, &sha256);
    std::string output;
    std::stringstream  ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
    }
    return UUID(hex_str_to_bin_str(ss.str()));
}

UUID hex_to_uuid(std::string hex){
    if(hex.length()>(sizeof(UUID)*2)) return {0};
    return UUID(hex_str_to_bin_str(hex));
};

std::string uuid_to_hex(UUID id){
    std::stringstream ss;
    for(size_t i=0; i<sizeof(UUID)*8; i=i+4)
        ss << std::hex << ((char)id[i] << (char)id[i+1] << (char)id[i+2] << (char)id[i+3]);
    return ss.str();
};

std::vector<byte> string_as_bytes(std::string str){
    std::vector<byte> result(str.size());
    memcpy(result.data(),str.data(),str.size());
    return result;
}

std::string bytes_as_string(std::vector<byte>& bytes){
    std::string result;
    result.resize(bytes.size());
    memcpy(result.data(),bytes.data(),bytes.size());
    return result;
}

std::vector<byte> uuid_as_bytes(UUID id){
    byte id_bytes[id.size()];
    for(size_t i=0; i<id.size(); i++){
        id_bytes[i] = id.test(i);
    }
    std::vector<byte> bytes(id.size());
    memcpy(bytes.data(), id_bytes, bytes.size());
    return bytes;
}

UUID bytes_as_uuid(std::vector<byte> bytes){
    UUID result;
    for(size_t i=0; i<bytes.size(); i++){
        result.set(i,bytes.at(i));
    }
    return result;
}

std::vector<byte> entry_as_bytes(UUID id,peer_address address){
    std::unordered_map<UUID,peer_address> temp;
    temp.insert(std::make_pair(id,address));
    std::stringstream ss;
    boost::archive::binary_oarchive oa(ss);
    oa & temp;
    auto serialized_table = ss.str();
    return  string_as_bytes(serialized_table);
}

std::pair<UUID,peer_address> bytes_as_entry(std::vector<byte> bytes){
    std::unordered_map<UUID,peer_address> temp;
    std::stringstream ss;
    ss << bytes_as_string(bytes);
    boost::archive::binary_iarchive ia(ss);
    ia >> temp;
    return *(temp.begin());
}