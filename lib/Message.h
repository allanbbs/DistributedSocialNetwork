#include <utility>

//
// Created by allanbs on 10/12/21.
//

#ifndef PROJ2_MESSAGE_H
#define PROJ2_MESSAGE_H
#include "utils.h"
#include <vector>
#include <cstring>


template<typename T>
struct HEADER{
    T id;
    size_t size;
};

template<typename TYPE>
class Message {
public:

    std::ostream& operator<<(std::ostream &os) {
        return os << this->to_string();
    }

    Message() = default;

    Message(TYPE id,std::vector<byte> vec) :body(vec){

        header = ::HEADER<TYPE>{id,vec.size()};
    }

    std::vector<byte> to_bytes(){
        std::vector<byte> result(sizeof(header) + body.size());
        auto bytes_size = sizeof(header);
        byte size_bytes[sizeof(header)];
        memcpy(size_bytes,&header,sizeof(header));
        for(size_t i = 0;i<bytes_size;i++){
            result.at(i) = size_bytes[i];
        }

        for(size_t i = bytes_size,k=0;i<body.size() + bytes_size;i++,k++)
            result.at(i) = body.at(k);
        return result;
    };
    void from_bytes(byte* bytes){
        memcpy(&header,bytes,sizeof(header));
        size_t offset =  sizeof(header);
        body.resize(header.size);
        size_t k = 0;
        for(size_t i = offset;i<header.size+offset;i++,k++)
            body.at(k) = bytes[i];
    };

    size_t& get_size() {
        return header.size;
    };

    size_t total_size(){
        return sizeof(header) + body.size();
    }

    std::vector<byte>& get_body() { return body;}

    TYPE& get_id(){
        return header.id;
    }

    ::HEADER<TYPE>& get_header(){
        return header;
    }


    std::string to_string(){
        std::string s = "";
        s += "Type:\t" + std::to_string(header.id);
        s += "\nSize:\t"+ std::to_string(body.size());
        char bodyp[header.size+1];
        memcpy(bodyp,body.data(),header.size);
        bodyp[header.size] = 0;
        std::string t(bodyp);
        s+="\nBody:\t"+t+"\n";
        return s;
    };
    void setId(TYPE id) {
        header.id = id;
    }

    void setBody(const std::vector<byte> &body) {
        Message::body = body;
    }

    void setSize(size_t size) {
        header.size = size;
    }
private:

    ::HEADER<TYPE> header;
    std::vector<byte> body;

};


#endif //PROJ2_MESSAGE_H
