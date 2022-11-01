/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions. (Revised 2020)
 *
 *  Starter code template
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node( Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = new Member;
    this->shouldDeleteMember = true;
	memberNode->inited = false;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member* member, Params *params, EmulNet *emul, Log *log, Address *address) {
    for( int i = 0; i < 6; i++ ) {
        NULLADDR[i] = 0;
    }
    this->memberNode = member;
    this->shouldDeleteMember = false;
    this->emulNet = emul;
    this->log = log;
    this->par = params;
    this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {
    if (shouldDeleteMember) {
        delete this->memberNode;
    }
}

/**
* FUNCTION NAME: recvLoop
*
* DESCRIPTION: This function receives message from the network and pushes into the queue
*                 This function is called by a node to receive messages currently waiting for it
*/
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: nodeStart
*
* DESCRIPTION: This function bootstraps the node
*                 All initializations routines for a member.
*                 Called by the application layer.
*/
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
    /*
    * This function is partially implemented and may require changes
    */

    // Ashutosh: Following two lines were commented to remove compile time warnings
	// int id = *(int*)(&memberNode->addr.addr);
	// int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	// node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
	initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == strcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;

        // Add initial node to it's own membership list.
        int id = *(int*)(&memberNode->addr.addr);
	    int port = *(short*)(&memberNode->addr.addr[4]);

        addNodeToMemberList(id, port, memberNode->heartbeat, memberNode->timeOutCounter);
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
* FUNCTION NAME: finishUpThisNode
*
* DESCRIPTION: Wind up this node and clean up state
*/
int MP1Node::finishUpThisNode(){
    /*
     * Your code goes here
     */

    // Returning 0 just to avoid compile time warnings.
    return 0;
}

/**
* FUNCTION NAME: nodeLoop
*
* DESCRIPTION: Executed periodically at each member
*                 Check your messages in queue and perform membership protocol duties
*/
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
    /*
    * Your code goes here
    */

    MP1Node::MsgTypes messageType = getMessageTypeFromMessage(data, size);

    if (messageType == JOINREQ) {
        NetworkMessage* networkMessage = parseNetworkMessage(data, size);

        // If incoming node is not currently a member of current node's membership list then add this node to membership list of current node.
        addNodeToMemberList(networkMessage->id, networkMessage->port, networkMessage->heartbeat, memberNode->timeOutCounter);

        // Send JOINREP message
        Address targetNodeAddress = getNodeAddress(networkMessage->id, networkMessage->port);
        sendJoinRepMessage(&targetNodeAddress);
    }
    else if (messageType == JOINREP) {

        // Node is added to the group now.
        memberNode->inGroup = true;

        // Update membership list of current node.
        updateMembershipList(data);
    }
    else if (messageType == HEARTBEAT) {
        NetworkMessage* networkMessage = parseNetworkMessage(data, size);

        if (this->findNodeInMembershipList(networkMessage->id) == NULL){
            // If not already present, add node that sent heartbeat to current node's membership list.
            addNodeToMemberList(networkMessage->id, networkMessage->port, networkMessage->heartbeat, memberNode->timeOutCounter);
        }
        else {
            // Update the membership entry with heartbeat and timestamp.
            MemberListEntry* node = findNodeInMembershipList(networkMessage->id);

            node->setheartbeat(networkMessage->heartbeat);
            node->settimestamp(memberNode->timeOutCounter);
        }
    }

    return true;
}

/**
* FUNCTION NAME: nodeLoopOps
*
* DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
*                 the nodes
*                 Propagate your membership list
*/
void MP1Node::nodeLoopOps() {
    
    /*
     * Your code goes here
     */

    // Remove all nodes from membership list that timeout.
    // Note that current node is part of it's own membership list, do not remove itself.
    for (auto iterator = memberNode->memberList.begin(); iterator != memberNode->memberList.end(); ++iterator) {
        Address memberNodeAddress = getNodeAddress(iterator->id, iterator->getport());

        if (!isCurrentNode(&memberNodeAddress)) {
            // If heartbeat was not detected within timeout period (setting as 5 here)
            // Then remove the mode from membership list.
            if (memberNode->timeOutCounter - iterator->timestamp > 5) {
                memberNode->memberList.erase(iterator);

#ifdef DEBUGLOG
                log->logNodeRemove(&memberNode->addr, &memberNodeAddress);
#endif
            }
        }
    }

    // Increment timeout counter for current node.
    memberNode->timeOutCounter++;

    // Send heartbeat messages to all nodes that are present in the membership list.
    // Note that current node is part of it's own membership list.
    // Do not send hearbeat to itself.
    for (auto iterator = memberNode->memberList.begin(); iterator != memberNode->memberList.end(); ++iterator) {
        Address memberNodeAddress = getNodeAddress(iterator->id, iterator->getport());

        if (!isCurrentNode(&memberNodeAddress)) {
            sendHeartbeat(&memberNodeAddress);
        }
    }

    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

/************** New functions ***************/

/**
 * FUNCTION NAME: addNodeToMemberList
 *
 * DESCRIPTION: Adds a node to current node's membership list.
 */
void MP1Node::addNodeToMemberList(int id, short port, long heartbeat, long timestamp) {
    // If incoming node is not currently a member of current node's membership list then add this node to membership list of current node.

    if (this->findNodeInMembershipList(id) == NULL){
        // The incoming node is not a member, add it to membership list.
        MemberListEntry* entry = new MemberListEntry(id, port, heartbeat, timestamp);
        memberNode->memberList.insert(memberNode->memberList.end(), *entry);

#ifdef DEBUGLOG
        Address incomingNodeAddress = getNodeAddress(id, port);
        log->logNodeAdd(&memberNode->addr, &incomingNodeAddress);
#endif

        free(entry);
    }
}

/**
 * FUNCTION NAME: getNodeAddress
 *
 * DESCRIPTION: Gets address of a node by using id and port.
 */
Address MP1Node::getNodeAddress(int id, short port) {
    Address* nodeaddress = new Address(to_string(id) + ":" + to_string(port));
    return *nodeaddress;
}

/**
 * FUNCTION NAME: findNodeInMembershipList
 *
 * DESCRIPTION: Finds given node in current node's membership list by using it's id.
 * If node is not found then NULL is returned, else MemberListEntry for found node is returned.
 */
MemberListEntry* MP1Node::findNodeInMembershipList(int id) {
    for (auto iterator = memberNode->memberList.begin(); iterator != memberNode->memberList.end(); ++iterator) {
        if (iterator->id == id) {
            return iterator.base();
        }
    }

    return NULL;
}

/**
 * FUNCTION NAME: sendJoinRepMessage
 *
 * DESCRIPTION: Send JOINREP message. This is done after a node has joined introducer node.
 * Introducer node (i.e. current node) will send it's membership list to target node.
 */
void MP1Node::sendJoinRepMessage(Address* targetAddress) {
    // 1. Create empty JOINREP message first.
    // Content of one entry: id, port, heartbeat, timestamp.
    size_t sizeOfOneMembershipEntry = sizeof(int) + sizeof(short) + sizeof(long) + sizeof(long);

    // Content of message: MessageType, number of items in membership list, All entries of membership list.
    size_t messageSize = sizeof(MessageHdr) + sizeof(int) + (memberNode->memberList.size() * sizeOfOneMembershipEntry);
    MessageHdr* joinrepMessage = (MessageHdr*)malloc(messageSize * sizeof(char));
    
    // 2. Start filling content in joinrepMessage
    joinrepMessage->msgType = JOINREP;
    int numberOfItems = memberNode->memberList.size();
    memcpy((char*)(joinrepMessage + 1), &numberOfItems, sizeof(int));

    // 3. Add member list of current node (introducer) to JOINREP message.
    int offset = sizeof(int);
    for (auto iterator = memberNode->memberList.begin(); iterator != memberNode->memberList.end(); ++iterator) {
        // Content of one entry: id - int, port - short, heartbeat - long, timestamp - long.
        memcpy((char*)(joinrepMessage + 1) + offset, &iterator->id, sizeof(int));
        offset += sizeof(int);

        memcpy((char*)(joinrepMessage + 1) + offset, &iterator->port, sizeof(short));
        offset += sizeof(short);

        memcpy((char*)(joinrepMessage + 1) + offset, &iterator->heartbeat, sizeof(long));
        offset += sizeof(long);

        memcpy((char*)(joinrepMessage + 1) + offset, &iterator->timestamp, sizeof(long));
        offset += sizeof(long);
    }

    // Send JOINREP message to the target node.
    emulNet->ENsend(&memberNode->addr, targetAddress, (char*)joinrepMessage, messageSize);

    free(joinrepMessage);
}

/**
 * FUNCTION NAME: updateMembershipList
 *
 * DESCRIPTION: Data represents content of JOINREP message.
 * This method extracts all membership list items from JOINREP message and adds them to current node's
 * membership list. (Add if they are not already present)
 */
void MP1Node::updateMembershipList(char* data) {
    // Find how many membership list entries are there in JOINREP message.
    int numberOfItems;
    memcpy(&numberOfItems, data + sizeof(MessageHdr), sizeof(int));

    int offset = sizeof(int);

    // Extract all entries and add them to current node's membership list (if they are not already present).
    for (int i = 0; i < numberOfItems; i++) {
        int id;
        short port;
        long heartbeat;
        long timestamp;

        memcpy(&id, data + sizeof(MessageHdr) + offset, sizeof(int));
        offset += sizeof(int);

        memcpy(&port, data + sizeof(MessageHdr) + offset, sizeof(short));
        offset += sizeof(short);

        memcpy(&heartbeat, data + sizeof(MessageHdr) + offset, sizeof(long));
        offset += sizeof(long);

        memcpy(&timestamp, data + sizeof(MessageHdr) + offset, sizeof(long));
        offset += sizeof(long);

        // Create and insert new entry
        addNodeToMemberList(id, port, heartbeat, timestamp);
    }
}

/**
 * FUNCTION NAME: isCurrentNode
 *
 * DESCRIPTION: Checks if given node is same as current node. This is done by comparing addresses.
 */
bool MP1Node::isCurrentNode(Address* address) {
    if ( 0 == strcmp((char *)&(memberNode->addr.addr), (char *)&(address))) {
        return true;
    }

    return false;
}

/**
 * FUNCTION NAME: sendHeartbeat
 *
 * DESCRIPTION: Sends hearbeat to one target node.
 */
void MP1Node::sendHeartbeat(Address* target) {
    size_t msgsize = sizeof(MessageHdr) + sizeof(target->addr) + sizeof(long) + 1;
    MessageHdr* msg = (MessageHdr*)malloc(msgsize * sizeof(char));

    // Create HEARTBEAT message
    msg->msgType = HEARTBEAT;
    memcpy((char*)(msg + 1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
    memcpy((char*)(msg + 1) + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

    // Send HEARTBEAT message to destination node
    emulNet->ENsend(&memberNode->addr, target, (char*)msg, msgsize);

    free(msg);
}

/**
 * FUNCTION NAME: getMessageTypeFromMessage
 *
 * DESCRIPTION: Extract message type from message.
 */
MP1Node::MsgTypes MP1Node::getMessageTypeFromMessage(char* data, int size) {
    MessageHdr receivedMessage;
    memcpy(&receivedMessage, data, sizeof(MessageHdr));
    return receivedMessage.msgType;
}

/**
 * FUNCTION NAME: getNodeIdFromMessage
 *
 * DESCRIPTION: Parses network messages JOINREQ and HEARTBEAT.
 */
NetworkMessage* MP1Node::parseNetworkMessage(char* data, int size) {
    int id;
    short port;
    long heartbeat;
    memcpy(&id, data + sizeof(MessageHdr), sizeof(int));
    memcpy(&port, data + sizeof(MessageHdr) + sizeof(int), sizeof(short));
    memcpy(&heartbeat, data + sizeof(MessageHdr) + sizeof(int) + sizeof(short), sizeof(long));
    NetworkMessage* nw = new NetworkMessage(id, port, heartbeat);
    return nw;
}