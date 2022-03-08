//
// Created by allanbs on 31/12/21.
//

#ifndef PROJ2_KADEMLIA_SERIALIZE_H
#define PROJ2_KADEMLIA_SERIALIZE_H
#include <boost/asio.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/bitset.hpp>
#include "../utils.h"

BOOST_SERIALIZATION_SPLIT_FREE(peer_address)
namespace boost{
    namespace serialization{
        template<class Archive>
        void save(Archive& ar, peer_address const& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.ip);
            ar & BOOST_SERIALIZATION_NVP(p.port);
        }

        template<class Archive>
        void load(Archive& ar, peer_address& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.ip);
            ar & BOOST_SERIALIZATION_NVP(p.port);
        }

    }
}


namespace boost{
    namespace serialization{
        template<class Archive,typename DATA_TYPE>
        void save(Archive& ar, find_answer<DATA_TYPE> const& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.found);
            ar & BOOST_SERIALIZATION_NVP(p.k_closest);
            ar & BOOST_SERIALIZATION_NVP(p.data);
        }

        template<class Archive,typename DATA_TYPE>
        void load(Archive& ar, find_answer<DATA_TYPE>& p, unsigned){
            ar & BOOST_SERIALIZATION_NVP(p.found);
            ar & BOOST_SERIALIZATION_NVP(p.k_closest);
            ar & BOOST_SERIALIZATION_NVP(p.data);
        }

        template<class Archive,typename DATA_TYPE>
        inline void serialize(Archive &ar, find_answer<DATA_TYPE>& p, const unsigned  int file_version){
            split_free(ar,p,file_version);
        }

    }
}
#endif //PROJ2_KADEMLIA_SERIALIZE_H
