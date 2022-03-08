//
// Created by allanbs on 08/01/22.
//

#ifndef PROJ2_INTERFACE_H
#define PROJ2_INTERFACE_H
#include <string>
#include "../Network/IPCConnection.h"
#include "App.h"
#include "../json.h"


bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

template <typename T>
class Interface {
public:
    Interface(std::shared_ptr<App<T>> ref) : app_ref(ref) {
        cv = std::make_shared<std::condition_variable>();
    }

    void run(){
        auto conn = IPCConnection(7777);
        //sleep(1);
        std::thread refresh([&]{this->timeline_refresh();});
        sleep(1);
        cv->notify_one();
        while(is_running){
            boost::system::error_code error;
            auto command = read_message(conn,error);
            if(error == boost::asio::error::eof) {
                is_running = false;
                cv->notify_one();
                app_ref->stop();
                break;
            }
            nlohmann::json json = nlohmann::json::parse(command);
            command_handler(conn, json);
        }
        if(refresh.joinable()) refresh.join();
    }

    std::shared_ptr<std::condition_variable> get_shared_cv(){
        return cv;
    }

private:
    bool is_running = true;
    std::mutex mutex;
    std::shared_ptr<std::condition_variable> cv;
    std::shared_ptr<App<T>> app_ref;

    void send_message(IPCConnection &conn,std::string message, boost::system::error_code &error) {
        conn.send(message, error);
    }

    std::string read_message(IPCConnection &conn, boost::system::error_code &error) {
        return conn.read(error);
    }

    nlohmann::json create_timeline(){
        nlohmann::json timeline;


        for(auto e: app_ref->storage.get_messages()){
            std::vector<nlohmann::json> temp;
            for(auto m: e.second){
                nlohmann::json message_json;
                if(!is_number(m.timestamp) || e.first.to_string() == "")
                    continue;
                message_json["sender"] = e.first.to_string();
                message_json["timestamp"] = m.timestamp;
                message_json["body"] = m.body;
                temp.push_back(message_json);
            }
            std::stringstream ss;
            ss << e.first;
            std::string uuid = ss.str();
            timeline[uuid] = temp;
        }
        return timeline;
    }

    void timeline_refresh(){
        IPCConnection conn (7777);
        while (is_running){
            std::unique_lock<std::mutex> ul(mutex);
            cv->wait(ul);
            boost::system::error_code error;
            send_message(conn, create_timeline().dump(), error);
            if(error==boost::asio::error::broken_pipe) {
                is_running = false;
                return;
            }
        }
    }

    void command_handler(IPCConnection &conn, nlohmann::json j){
        nlohmann::json response;

        if(j["command"]=="SUBSCRIBE") {
            std::string s = j["value"];
            app_ref->subscribe(UUID(s));
        }
        else if(j["command"]=="UNSUBSCRIBE") {
            std::string s = j["value"];
            app_ref->unsubscribe(UUID(s));
        }
        else if(j["command"]=="PUBLISH")
            app_ref->publish_push(j["value"]);
        else if(j["command"]=="GET_ID") {
            std::stringstream ss;
            ss << app_ref->peer_ref->get_peer_id();
            std::string uuid = ss.str();
            response["value"] = uuid;
            boost::system::error_code error;
            send_message(conn, response.dump(), error);
        }
        else if(j["command"]=="GET_SUBSCRIBERS") {
            std::vector<std::string> subs;
            for (auto& s: app_ref->storage.get_messages()) {
                if(s.first==app_ref->peer_ref->get_peer_id()) continue;
                std::stringstream ss;
                ss << s.first;
                subs.push_back(ss.str());
            }
            response["subscribers"] = subs;
            boost::system::error_code error;
            send_message(conn, response.dump(), error);
        }
    }
};


#endif //PROJ2_INTERFACE_H
