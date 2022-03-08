//
// Created by allanbs on 05/01/22.
//

#ifndef PROJ2_APP_SERIALIZATION_H
#define PROJ2_APP_SERIALIZATION_H


#include <boost/asio.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/bitset.hpp>
#include "../utils.h"
#include "../Kademlia/kademlia_serialize.h"
#include "application_layer.h"


namespace boost{
    namespace serialization{
        template<class Archive,typename DATA_TYPE>
        void save(Archive& ar, DTMessage<DATA_TYPE> const& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.counter);
            ar & BOOST_SERIALIZATION_NVP(p.timestamp);
            ar & BOOST_SERIALIZATION_NVP(p.body);
        }

        template<class Archive,typename DATA_TYPE>
        void load(Archive& ar, DTMessage<DATA_TYPE>& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.counter);
            ar & BOOST_SERIALIZATION_NVP(p.timestamp);
            ar & BOOST_SERIALIZATION_NVP(p.body);
        }

        template<class Archive,typename DATA_TYPE>
        inline void serialize(Archive &ar, DTMessage<DATA_TYPE>& p, const unsigned  int file_version){
            split_free(ar,p,file_version);
        }

    }
}

namespace boost{
    namespace serialization{
        template<class Archive>
        void save(Archive& ar, DHT_VALUE const& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.original_publisher);
            ar & BOOST_SERIALIZATION_NVP(p.followers);
        }

        template<class Archive>
        void load(Archive& ar, DHT_VALUE& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.original_publisher);
            ar & BOOST_SERIALIZATION_NVP(p.followers);
        }

        template<class Archive>
        inline void serialize(Archive &ar, DHT_VALUE& p, const unsigned  int file_version){
            split_free(ar,p,file_version);
        }

    }
}
#endif //PROJ2_APP_SERIALIZATION_H
