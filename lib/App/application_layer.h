//
// Created by allanbs on 05/01/22.
//

#ifndef PROJ2_APPLICATION_LAYER_H
#define PROJ2_APPLICATION_LAYER_H

/*
 * ENUMERATOR OF MESSAGE TYPES AVAILABLE IN THE APPLICATION LAYER
 */
enum DTMessageType{
    DT_NONE = 0,
    STORE_MESSAGE = 1,
    STORE_MESSAGE_ANSWER = 2,
    SUBSCRIBE = 3,
    SUBSCRIBE_ANSWER = 4,
    UNSUBSCRIBE = 5,
    GET = 7,
    GET_ANSWER = 8,
    PUSH = 9,
    NOTIFY_SUBSCRIBER = 11
};

/*
 *  DEFINES MAXIMUM NUMBER OF MESSAGES TO BE STORED PER SUBSCRIBED PEER
 */
#define MAX_STORED_MSG 50

/*
 *  DEFINES THE INTERVAL OF STATE UPDATES
 */
#define UPDATE_INTERVAL 10000

/*
 *  DEFINES THE MAXIMUM NUMBER OF MESSAGES RETURNED AS REPLY FOR A GET MESSAGE
 */
#define MAX_GET_REP (size_t) 10

/*
 *  DEFINES THE TIMEOUT FOR WAITING REPLIES IN THE APPLICATION LAYER
 */
#define APP_LAYER_TIMEOUT 3000


/*
 *  TEMPLATE DTMessage struct (application level message) to be exchanged between users
 */
template<typename DATA_TYPE>
struct DTMessage{
    size_t counter;
    std::string timestamp;
    DATA_TYPE body;
};

template<typename DATA_TYPE>
bool operator==(DTMessage<DATA_TYPE> &d1,DTMessage<DATA_TYPE> &d2){
    return (d1.counter == d2.counter) && (d1.body == d2.body);
}


struct DHT_VALUE{
    UUID original_publisher;
    std::unordered_map<UUID,peer_address> followers;
};


#endif //PROJ2_APPLICATION_LAYER_H
