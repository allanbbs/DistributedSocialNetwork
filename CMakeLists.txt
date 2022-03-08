cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 17)
project(proj2)

add_executable(proj2 src/main.cpp src/Network/Connection.cpp src/Kademlia/Peer.cpp lib/Kademlia/Peer.h lib/Message.h lib/utils.h lib/Network/Connection.h lib/Network/TCPConnection.h src/utils.cpp src/Network/TCPConnection.cpp
        src/Kademlia/ConnectionHandler.cpp lib/Kademlia/ConnectionHandler.h lib/tsqueue.h lib/Kademlia/kademlia_serialize.h src/Kademlia/UDPConnectionHandler.cpp lib/Kademlia/UDPConnectionHandler.h lib/Kademlia/p2p_layer.h lib/Kademlia/KBucket.h src/Kademlia/KBucket.cpp src/App/App.cpp lib/App/App.h lib/App/application_layer.h lib/p2p_layer_utils.h src/p2p_layer_utils.cpp src/App/Storage.cpp lib/App/Storage.h lib/App/app_serialization.h)

TARGET_LINK_LIBRARIES( proj2 pthread boost_serialization boost_system ssl crypto)