#define ASIO_STANDALONE
#include <iostream>
#include <string>
#include "../lib/utils.h"
#include "../lib/Kademlia/Peer.h"
#include "../lib/App/App.h"
#include "../lib/App/Storage.h"
#include "../lib/p2p_layer_utils.h"
#include <thread>
#include <boost/asio.hpp>

namespace ip = boost::asio::ip;

/*
 * Parse command line arguments into struct
 */
void parse_cli_args(int argc, char ** argv,setup_config& config){

    for(size_t i = 1;i < argc;){

        //Set current peer as a bootstrap peer
        if(!strcmp(argv[i],"-b") || !strcmp(argv[i],"--bootstrap")){
            config.bootstrap_peer = true;
            i++;
            continue;
        }
        //Define local endpoint
        if(!strcmp(argv[i],"-l") || !strcmp(argv[i],"--local_endpoint")){
            config.local_endpoint = {argv[i+1], static_cast<uint16_t>(std::stoi(argv[i+2]))};
            i+=3;
            continue;
        }

        //Define a single remote bootstrapping endpoint
        if(!strcmp(argv[i],"-be") || !strcmp(argv[i],"--bootstrap_endpoint")){
            config.bootstraping_peers.push_back({argv[i+1], static_cast<uint16_t>(std::stoi(argv[i+2]))});
            i+=3;
            continue;
        }
        else{
            i+=1;
        }
    }
}

/*
 * Display setup_config struc passed as parameter to stdout
 */
void print_setup_config(setup_config& p){
    std::cout<<"Bootstrap peer: "<<(p.bootstrap_peer? "Yes" : "No")<<std::endl;
    std::cout<<"Local endpoint: "<<p.local_endpoint<<std::endl;
    if(!p.bootstrap_peer) {
        std::cout << "Bootstraping peers: " << std::endl;
        for(auto& peer : p.bootstraping_peers){
            std::cout<<"Endpoint: "<<peer<<std::endl;
        }
    }
}

int main(int argc, char**argv) {

    setup_config config;
    parse_cli_args(argc,argv,config);
    print_setup_config(config);
    std::shared_ptr<App<std::string>> application = std::make_shared<App<std::string>>(config);
    std::thread t([&]{application->run();});
    if(t.joinable())
        t.join();
    return 0;
}