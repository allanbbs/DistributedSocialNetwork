//
// Created by allanbs on 05/01/22.
//

#ifndef PROJ2_STORAGE_H
#define PROJ2_STORAGE_H
#include <string>
#include <fstream>
#include <unordered_map>
#include "../utils.h"


template<typename KEY_TYPE,typename VALUE_TYPE>
class Storage {
private:
    std::string message_filename;
    std::string subscriber_filename;
    std::fstream message_stream;
    std::fstream subscriber_stream;
    std::unordered_map<KEY_TYPE, VALUE_TYPE> messages;

    size_t operationsBeforeSave = 5;
    size_t operationsAfterLastSave = 0;


    void save();
    void load();

public:
    /*
    size_t get_message_counter(KEY_TYPE user_id) {
        if(messages.find(user_id) == messages.end()) return 0;
        return messages[user_id].back().counter;
    }
    */

    std::unordered_map<KEY_TYPE, VALUE_TYPE> get_messages(){ return messages; }

    std::vector<KEY_TYPE> own_subscribers;
    template<typename VALUE_TEMPLATE>
    void add(KEY_TYPE key, VALUE_TEMPLATE value) {
        auto it = messages.find(key);
        if(it != messages.end()){
            for(auto &message : it->second){
                if(value == message){
                    return;
                }
            }
            it->second.push_back(value);
            save();
        }
        else {
            VALUE_TYPE temp;
            temp.push_back(value);
            messages.insert({key,temp});
            save();
        }
    }

    VALUE_TYPE get(KEY_TYPE key) {
        auto it = messages.find(key);
        if (it != messages.end()) {
            return it->second;
        }
        return VALUE_TYPE{};
    }

    void remove(KEY_TYPE key){
        auto it = messages.find(key);
        if (it != messages.end()) {
            messages.erase(it);
        }
    }

    void add_subscriber(KEY_TYPE subscriber){
        if(find(own_subscribers.begin(),own_subscribers.end(),subscriber) == own_subscribers.end()) {
            own_subscribers.push_back(subscriber);
            save();
        }
    }

    void unsubscribe(KEY_TYPE subscriber){
        auto it = std::find(own_subscribers.begin(), own_subscribers.end(), subscriber);
        if(it != own_subscribers.end()) {
            own_subscribers.erase(it);
            save();
        }
    }


    Storage() = default;

    Storage(std::string message_filename,std::string subscriber_filename) : subscriber_filename(subscriber_filename), message_filename(message_filename), message_stream(message_filename, std::ios_base::out | std::ios_base::in |
                                                                              std::ios_base::binary),subscriber_stream(subscriber_filename, std::ios_base::out | std::ios_base::in |
                                                                                                                              std::ios_base::binary) {
        load();
    }

    void force_save();

    void save_to_disk();
};


#endif //PROJ2_STORAGE_H
