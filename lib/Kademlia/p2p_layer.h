//
// Created by allanbs on 04/01/22.
//

#ifndef PROJ2_P2P_LAYER_H
#define PROJ2_P2P_LAYER_H
/*
 *  Definition of useful constants related to the peer to peer layer
 *  Constants marked with [KADEMLIA] are parameters defined in the KADEMLIA specification
 */

/*
 *  TIMEOUT BETWEEN REQUEST SENT TO NETWORK AND REPLY
 */
#define P2P_LAYER_TIMEOUT 3000

/*
 *  NUMBER OF MAXIMUM RETRANSMISSION OF A MESSAGE
 */
#define TRIES 3

/*  [KADEMLIA] suggested = 20
 *  NUMBER OF CLOSEST PEERS TO BE RETURNED IN LOOKUP OPERATIONS
 */
#define K 3

/*  [KADEMLIA] suggested = 3
 *  NUMBER OF PARALLEL FIND_NODE REQUESTS TO BE SENT
 */
#define ALPHA 3


#endif //PROJ2_P2P_LAYER_H
