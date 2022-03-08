//
// Created by allanbs on 05/01/22.
//

#include "../../lib/App/Storage.h"
#include "../../lib/utils.h"
#include "../../lib/App/application_layer.h"
#include "../../lib/App/app_serialization.h"
#include "boost/serialization/vector.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>



template class Storage<UUID,std::vector<DTMessage<std::string>>>;
template class Storage<int,std::vector<std::string>>;





template<typename KEY_TYPE,typename VALUE_TYPE>
void Storage<KEY_TYPE, VALUE_TYPE>::save() {
    operationsAfterLastSave++;
    if(operationsAfterLastSave == operationsBeforeSave){
        save_to_disk();
        operationsAfterLastSave = 0;
    }
}

template<typename KEY_TYPE,typename VALUE_TYPE>
void Storage<KEY_TYPE, VALUE_TYPE>::save_to_disk() {
    message_stream = std::fstream(message_filename,std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc);
    subscriber_stream = std::fstream(subscriber_filename,std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc);
    boost::archive::binary_oarchive oa(message_stream);
    oa & messages;
    boost::archive::binary_oarchive oa2(subscriber_stream);
    oa2 & own_subscribers;
    std::cout<<"SAVED DATA IN DISK"<<std::endl;
    message_stream.close();
    subscriber_stream.close();
}


template<typename KEY_TYPE,typename VALUE_TYPE>
void Storage<KEY_TYPE, VALUE_TYPE>::force_save() {
        save_to_disk();
        operationsAfterLastSave = 0;
}

template<typename KEY_TYPE,typename VALUE_TYPE>
void Storage<KEY_TYPE, VALUE_TYPE>::load() {
    //Loading messages
    //messages.clear();
    if(message_stream.peek() != std::ifstream::traits_type::eof()) {
        boost::archive::binary_iarchive ia(message_stream);
        ia >> messages;
    }
    message_stream.close();
    //Loading subscribers
    //own_subscribers.clear();
    if(subscriber_stream.peek() != std::ifstream::traits_type::eof()) {
        boost::archive::binary_iarchive ia(subscriber_stream);
        ia >> own_subscribers;
    }
    subscriber_stream.close();



}