/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition (Revised 2020)
 *
 * MP2 Starter template version
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	this->delimiter = "::";
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
}

/**
* FUNCTION NAME: updateRing
*
* DESCRIPTION: This function does the following:
*                 1) Gets the current membership list from the Membership Protocol (MP1Node)
*                    The membership list is returned as a vector of Nodes. See Node class in Node.h
*                 2) Constructs the ring based on the membership list
*                 3) Calls the Stabilization Protocol
*/
void MP2Node::updateRing() {
	/*
     * Implement this. Parts of it are already implemented
     */
    vector<Node> curMemList;
    bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	change = isMembershipStale(curMemList);
    if (ring.empty() || change){
        ring = curMemList;
	}

    if (hasMyReplicas.empty() || haveReplicasOf.empty()){
        assignReplicationNodes();
	}

	// If stablization is required run stablization protocol.
    if (change && !ht->isEmpty()){
        ring = curMemList;
		stabilizationProtocol();
	}
}

/**
* FUNCTION NAME: isStablizationRequired
*
* DESCRIPTION: Checks if stabilaization is required.
*/
bool MP2Node::isMembershipStale(vector<Node> currentMembershipList) {
	// If ring is empty then stabilization is not needed.
	if (ring.empty()){
		return false;
	}

	// If current membership list is not same as current ring, then we need stabilization.
	if (ring.size() != currentMembershipList.size()){
		return true;
	}

	// If any node in current membership list and ring are different, then we need stabilization.
	for (unsigned int i = 0; i < currentMembershipList.size(); i++) {
		if (!areNodesSame(currentMembershipList[i], ring[i])){
			return true;
		}
	}

    return false;
}

/**
 * FUNCTION NAME: areNodesSame
 *
 * DESCRIPTION: Checks if two nodes are same. This is done by comparing addresses.
 */
bool MP2Node::areNodesSame(Node node1, Node node2) {
    if (memcmp(node1.getAddress()->addr, node2.getAddress()->addr, sizeof(Address)) == 0){
        return true;
	}

    return false;
}

/**
* FUNCTION NAME: getMembershipList
*
* DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
*                 i) generates the hash code for each member
*                 ii) populates the ring member in MP2Node class
*                 It returns a vector of Nodes. Each element in the vector contain the following fields:
*                 a) Address of the node
*                 b) Hash code obtained by consistent hashing of the Address
*/
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
* FUNCTION NAME: hashFunction
*
* DESCRIPTION: This functions hashes the key and returns the position on the ring
*                 HASH FUNCTION USED FOR CONSISTENT HASHING
*
* RETURNS:
* size_t position on the ring
*/
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
* FUNCTION NAME: clientCreate
*
* DESCRIPTION: client side CREATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientCreate(string key, string value) {
	/*
	* Implement this
	*/

	propagateMessageFromClient(CREATE, key, value);
}

/**
* FUNCTION NAME: clientRead
*
* DESCRIPTION: client side READ API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientRead(string key){
	/*
	* Implement this
	*/
	propagateMessageFromClient(READ, key, "");
}

/**
* FUNCTION NAME: clientUpdate
*
* DESCRIPTION: client side UPDATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientUpdate(string key, string value){
	/*
    * Implement this
    */
	propagateMessageFromClient(UPDATE, key, value);
}

/**
* FUNCTION NAME: clientDelete
*
* DESCRIPTION: client side DELETE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientDelete(string key){
	/*
	* Implement this
	*/
	propagateMessageFromClient(DELETE, key, "");
}

void MP2Node::propagateMessageFromClient(MessageType msgType, string key, string value) {
	// Find replica nodes for they key.
    vector<Node> replicaNodes = findNodes(key);

	// TODO: Remove it when logging not needed.
	if (CUSTOMLOGENABLED == 1){
		string temp = "";
		for (auto n : replicaNodes)
		{
			temp += ", " + n.getAddress()->getAddress();
		}

		log->LOG(&getMemberNode()->addr, "CUSTOMLOG: Sending read to servers: %s", temp.c_str());
	}

	// Should transaction id be tracked at client level or it should be at server level.
	// if client level: then there will be 3 messages in network for each transaction id.
	int transId = ++g_transID;
	for (unsigned int i = 0; i < replicaNodes.size(); i++){
		Node replica = replicaNodes.at(i);
		Address currAddress = getMemberNode()->addr;

		ReplicaType replicaType = i == 0 ? PRIMARY : (i == 1 ? SECONDARY : TERTIARY);
		Message *msg;
        switch(msgType)
		{
            case CREATE:
			case UPDATE:
				msg = new Message(transId, currAddress, msgType, key, value, replicaType);
				break;
            case READ:
            case DELETE:
				msg = new Message(transId, currAddress, msgType, key);
				break;
            default:
				return;
        }

		if (CUSTOMLOGENABLED == 1){
			log->LOG(&getMemberNode()->addr, "CUSTOMLOG: Sent read to server: %s", replica.getAddress()->getAddress().c_str());
		}
        emulNet->ENsend(&currAddress, replica.getAddress(), msg->toString());

		// TODO: free mes. Why?
	}

	// Add details of this transaction to active transactionList.
	addActiveTransaction(transId, msgType, key, value, replicaNodes.size());
}

/**
* FUNCTION NAME: createKeyValue
*
* DESCRIPTION: Server side CREATE API
*                    The function does the following:
*                    1) Inserts key value into the local hash table
*                    2) Return true or false based on success or failure
*/
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table
	Entry e(value, par->getcurrtime(), replica);
    return ht->create(key, e.convertToString());
}

/**
* FUNCTION NAME: readKey
*
* DESCRIPTION: Server side READ API
*                 This function does the following:
*                 1) Read key from local hash table
*                 2) Return value
*/
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
	return ht->read(key);
}

/**
* FUNCTION NAME: updateKeyValue
*
* DESCRIPTION: Server side UPDATE API
*                 This function does the following:
*                 1) Update the key to the new value in the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
	// return ht->update(key, value);
	Entry e(value, par->getcurrtime(), replica);
	return ht->update(key, e.convertToString());
}

/**
* FUNCTION NAME: deleteKey
*
* DESCRIPTION: Server side DELETE API
*                 This function does the following:
*                 1) Delete the key from the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table
	return ht->deleteKey(key);
}

/**
* FUNCTION NAME: checkMessages
*
* DESCRIPTION: This function is the message handler of this node.
*                 This function does the following:
*                 1) Pops messages from the queue
*                 2) Handles the messages according to message types
*/
void MP2Node::checkMessages() {
	/*
	* Implement this. Parts of it are already implemented
	*/
	char * data;
	int size;

	/*
	* Declare your local variables here
	*/

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);

		/*
		 * Handle the message types here
		 */

		// Convert message string to Message type so that we can access properties.
		Message receivedMessage(message);
        switch(receivedMessage.type)
		{
            case CREATE:
				handleCreateMessage(receivedMessage);
				break;
            case READ:
				handleReadMessage(receivedMessage);
				break;
            case REPLY:
				handleReplyMessage(receivedMessage);
				break;
            case READREPLY:
				handleReadReplyMessage(receivedMessage);
				break;
            case DELETE:
				handleDeleteMessage(receivedMessage);
				break;
            case UPDATE:
				handleUpdateMessage(receivedMessage);
				break;
            default:
				break;
        }

		checkAwaitedTransactions();
	}

	/*
	* This function should also ensure all READ and UPDATE operation
	* get QUORUM replies
	*/
}

/**
* FUNCTION NAME: findNodes
*
* DESCRIPTION: Find the replicas of the given keyfunction
*                 This function is responsible for finding the replicas of a key
*/
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i < (int)ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
	if ( memberNode->bFailed ) {
		return false;
	}
	else {
		return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
	}
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: stabilizationProtocol
*
* DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
*                 It ensures that there always 3 copies of all keys in the DHT at all times
*                 The function does the following:
*                1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
*                Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
*/
void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */
	if (CUSTOMLOGENABLED == 1){
		log->LOG(&getMemberNode()->addr, "CUSTOMLOG: Performing stablization.");
		// cout << "Performing stablization " << endl;
	}

	Address myAddress = getMemberNode()->addr;
    vector<Node> new_replicas, new_bosses;
    Node this_node = Node(myAddress);

	// Find new successors and predecessors.
	// We are in this method, means that one or more of them will be different than current successors/ predecessors.
	// Store these in temporary variables for now.
	for (unsigned int i = 0; i < ring.size(); i++) {
		if (areNodesSame(ring[i], this_node)){
			new_bosses.push_back(ring[(i - 1 + ring.size()) % ring.size()]);
			new_bosses.push_back(ring[(i - 2 + ring.size()) % ring.size()]);

			new_replicas.push_back(ring[(i + 1) % ring.size()]);
			new_replicas.push_back(ring[(i + 2) % ring.size()]);

			break;
		}
	}

	manageFailedSuccessors(new_replicas, myAddress);
	manageFailedPredecessors(new_bosses, new_replicas, myAddress);

    // Now update the new replicas and bosses
    hasMyReplicas  = new_replicas;
    haveReplicasOf = new_bosses;

}

// Manage failed successors.
void MP2Node::manageFailedSuccessors(vector<Node> new_replicas, Address myAddress) {
    vector<pair<string, string>> keys = findMyKeys(PRIMARY);
	
	string logMessage = "None";
	bool secondaryFailed = !nodeExistsInList(new_replicas, hasMyReplicas[0]);
	bool terFailed = !nodeExistsInList(new_replicas, hasMyReplicas[1]);

	if (secondaryFailed && terFailed){
		logMessage = "Both secondary and tertiary failed";
		MessageType messageType = CREATE;
		for (unsigned int i = 0; i < new_replicas.size(); i++) {
			ReplicaType replicaType = i == 0 ? SECONDARY : TERTIARY;
			for (unsigned int k = 0; k < keys.size(); k++) {
				Message *msg = new Message(-1, myAddress, messageType, keys[k].first, keys[k].second, replicaType);
				emulNet->ENsend(&myAddress, new_replicas[i].getAddress(), msg->toString());
				free(msg);
			}
		}
	}
	else if (secondaryFailed){
		logMessage = "Only secondary failed";
		for (unsigned int i = 0; i < new_replicas.size(); i++) {
			ReplicaType replicaType = i == 0 ? SECONDARY : TERTIARY;
			MessageType messageType = i == 0 ? UPDATE : CREATE;
			for (unsigned int k = 0; k < keys.size(); k++) {
				Message *msg = new Message(-1, myAddress, messageType, keys[k].first, keys[k].second, replicaType);
				emulNet->ENsend(&myAddress, new_replicas[i].getAddress(), msg->toString());
				free(msg);
			}
		}
	}
	else if (terFailed){
		logMessage = "Only tertiary failed";
		ReplicaType replicaType = TERTIARY;
		MessageType messageType = CREATE;
		for (unsigned int k = 0; k < keys.size(); k++) {
			Message *msg = new Message(-1, myAddress, messageType, keys[k].first, keys[k].second, replicaType);
			emulNet->ENsend(&myAddress, new_replicas[1].getAddress(), msg->toString());
			free(msg);
		}
	}
	
	if (CUSTOMLOGENABLED == 1){
		logMessage = "CUSTOMLOG: Inside manageFailedSuccessors: " + logMessage;
		log->LOG(&getMemberNode()->addr, logMessage.c_str());
	}
}

// Manage failed predecessors.
void MP2Node::manageFailedPredecessors(vector<Node> new_bosses, vector<Node> new_replicas, Address myAddress) {
	// check for my bosses if they failed
    for (unsigned int i = 0; i < haveReplicasOf.size(); i++) {
        if (areNodesSame(haveReplicasOf[i], new_bosses[i])) {
            break; // if the boss is same and at same location, then break because boss will take care of its boss and will also correct this nodes replica type if necessary as is shown below.
        }
        else {
            if (ifExistNode(new_bosses, haveReplicasOf[i]) != -1) {
                break; // if there is some boss present which was also present earlier then don't do anything as it will take care of things and nodes in this system only leave, no join is there'
            } else {
                // else find keys at this node which are secondary or tertiary depending on how this node is located according to the new boss.
                vector<pair<string, string>> keys = findMyKeys(static_cast<ReplicaType>(i + 1));
                Entry *e; // Update keys at this local server. No need to send message since all the operations are local.
                for (unsigned int k = 0; k < keys.size(); k++){
                    e = new Entry(keys[k].second, par->getcurrtime(), PRIMARY);
                    ht->update(keys[k].first, e->convertToString());
                    free(e);
                }
                if (i == 0){ // If this was the secondary replica of the failed boss, then make your first replica secondary from tertiary by updating it.
                    // Also, add new tertiary replica by sending create messages for all the keys of the bosses which have now become primary at this server.
                    Message *msg;
                    for (unsigned int k = 0; k < keys.size(); k++) {
                        if (areNodesSame(new_replicas[0], hasMyReplicas[0])){
                            msg = new Message(-1, getMemberNode()->addr, UPDATE, keys[k].first, keys[k].second, SECONDARY);
                            emulNet->ENsend(&getMemberNode()->addr, new_replicas[0].getAddress(), msg->toString());
                        } else {
                            msg = new Message(-1, getMemberNode()->addr, CREATE, keys[k].first, keys[k].second, SECONDARY);
                            emulNet->ENsend(&getMemberNode()->addr, new_replicas[0].getAddress(), msg->toString());
                        }
                        free(msg);

                        msg = new Message(-1, getMemberNode()->addr, CREATE, keys[k].first, keys[k].second, TERTIARY);
                        emulNet->ENsend(&getMemberNode()->addr, new_replicas[1].getAddress(), msg->toString());
                        free(msg);
                    }
                } else if (i == 1){
                    // If this was the tertiary replica of the failed boss, then add both of your replicas
                    // by sending create messages for all the keys of the bosses which have now become primary at this server.
                    Message *msg;
                    for (unsigned int k = 0; k < keys.size(); k++) {
                        msg = new Message(-1, getMemberNode()->addr, CREATE, keys[k].first, keys[k].second, SECONDARY);
                        emulNet->ENsend(&getMemberNode()->addr, new_replicas[0].getAddress(), msg->toString());
                        free(msg);
                        msg = new Message(-1, getMemberNode()->addr, CREATE, keys[k].first, keys[k].second, TERTIARY);
                        emulNet->ENsend(&getMemberNode()->addr, new_replicas[1].getAddress(), msg->toString());
                        free(msg);
                    }
                }
            }
        }
    }
}

/*
* CPF: Check if some vector of nodes contain a given node.
* */
int MP2Node::ifExistNode(vector<Node> v, Node n1){
    vector<Node>::iterator iterator1 = v.begin();
    int i = 0;
    while(iterator1 != v.end()){ // iterate over each node in the vector
        if (areNodesSame(n1, *iterator1)) // if the nodes are same return the location otherwise return -1
            return i;
        iterator1++;
        i++;
    }
    return -1;
}

// Check if node exists in vector of nodes.
bool MP2Node::nodeExistsInList(vector<Node> list, Node node){
	for (auto i : list){
		if (areNodesSame(node, i))
			return true;
	}

    return false;
}

// CPF: Find Keys at this server that are residing here as of specific replica type.
vector<pair<string, string>> MP2Node::findMyKeys(ReplicaType rep_type) {
    map<string, string>::iterator iterator1 = ht->hashTable.begin();
    vector<pair<string, string>> keys;
    Entry *temp_e;
    while (iterator1 != ht->hashTable.end()){ // Loop over all the keys in the hashtable and find the key that belong to the passed replica type
        temp_e = new Entry(iterator1->second);
        if (temp_e->replica == rep_type){
            keys.push_back(pair<string, string>(iterator1->first, temp_e->value));
        }
        iterator1++;
        free(temp_e); // free the entry variable
    }
    return keys; // return vector of pair of keys and values.
}

void MP2Node::handleCreateMessage(Message message)
{
	// TODO: Should we update transMap here? what is need of this map anyways?
	// Create key value locally.
	bool isSuccess = createKeyValue(message.key, message.value, message.replica);

	// Send reply message to coordinator.
    int transId = message.transID;
	Address myAddress = getMemberNode()->addr;
    Address coordinatorAddress(message.fromAddr);

    // TODO: Check for stablilization protocol.
    // if (transId < 0){
	//	return;
	// }

    Message *msg = new Message(transId, myAddress, REPLY, isSuccess);
    emulNet->ENsend(&myAddress, &coordinatorAddress, msg->toString());

    // Log result - note that these logs are replica level CRUD messages.
    if (isSuccess)
	{
        log->logCreateSuccess(&myAddress, false, transId, message.key, message.value);
	}
    else
	{
        log->logCreateFail(&myAddress, false, transId, message.key, message.value);
	}
}

void MP2Node::handleReadMessage(Message message)
{
	// TODO: Should we update transMap here? what is need of this map anyways?
	// read key value locally.
	string value = readKey(message.key);

	// Send reply message to coordinator.
    int transId = message.transID;
	Address myAddress = getMemberNode()->addr;
    Address coordinatorAddress(message.fromAddr);

    // TODO: Check for stablilization protocol.
    if (transId < 0){
		return;
	}

	// Send READREPLY message.
    Message *msg = new Message(transId, myAddress, value);
    emulNet->ENsend(&myAddress, &coordinatorAddress, msg->toString());

    // Log result - note that these logs are replica level CRUD messages.
    if (!value.empty())
	{
        log->logReadSuccess(&myAddress, false, transId, message.key, value);
	}
    else
	{
        log->logReadFail(&myAddress, false, transId, message.key);
	}
}

void MP2Node::handleReadReplyMessage(Message message)
{
	int transId = message.transID;
	if (this->activeTransactions.find(transId) == this->activeTransactions.end()) {
		// We are here when transId is not present in activeTransactions.
		// This happens only when we explicitly remove transId in checkAwaitedTransactions method.
		// We have received a reply for which we already passed/ failed transaction.
		// See logic in checkAwaitedTransactions for details.
		return;
	}

	// Address myAddress = getMemberNode()->addr;
    Address senderAddress(message.fromAddr);

	// If message's value is not empty then message was successful.
	bool isSuccess = !message.value.empty();

	this->activeTransactions[transId].replyCount++;

	if (isSuccess){
		this->activeTransactions[transId].value = message.value;
		this->activeTransactions[transId].successCount++;
	}
	else{
		this->activeTransactions[transId].failureCount++;
	}
}

void MP2Node::handleDeleteMessage(Message message)
{
	// TODO: Should we update transMap here? what is need of this map anyways?
	// Delete key value locally.
	bool isSuccess = deletekey(message.key);

	// Send reply message to coordinator.
    int transId = message.transID;
	Address myAddress = getMemberNode()->addr;
    Address coordinatorAddress(message.fromAddr);

    // TODO: Check for stablilization protocol.
    if (transId < 0){
		return;
	}

    Message *msg = new Message(transId, myAddress, REPLY, isSuccess);
    emulNet->ENsend(&myAddress, &coordinatorAddress, msg->toString());

    // Log result - note that these logs are replica level CRUD messages.
    if (isSuccess)
	{
        log->logDeleteSuccess(&myAddress, false, transId, message.key);
	}
    else
	{
        log->logDeleteFail(&myAddress, false, transId, message.key);
	}
}

void MP2Node::handleUpdateMessage(Message message)
{
	// TODO: Should we update transMap here? what is need of this map anyways?
	// Update key value locally.
	bool isSuccess = updateKeyValue(message.key, message.value, message.replica);

	// Send reply message to coordinator.
    int transId = message.transID;
	Address myAddress = getMemberNode()->addr;
    Address coordinatorAddress(message.fromAddr);

    // TODO: Check for stablilization protocol.
    // if (_trans_id < 0)
    //    return;

    Message *msg = new Message(transId, myAddress, REPLY, isSuccess);
    emulNet->ENsend(&myAddress, &coordinatorAddress, msg->toString());

    // Log result - note that these logs are replica level CRUD messages.
    if (isSuccess)
	{
        log->logUpdateSuccess(&myAddress, false, transId, message.key, message.value);
	}
    else
	{
        log->logUpdateFail(&myAddress, false, transId, message.key, message.value);
	}
}

// REPLY message is sent by replicas for create/ update or delete operation.
// This message is handled only at coordinator.
void MP2Node::handleReplyMessage(Message message)
{
	int transId = message.transID;
	if (this->activeTransactions.find(transId) == this->activeTransactions.end()) {
		// We are here when transId is not present in activeTransactions.
		// This happens only when we explicitly remove transId in checkAwaitedTransactions method.
		// We have received a reply for which we already passed/ failed transaction.
		// See logic in checkAwaitedTransactions for details.
		return;
	}

	// Address myAddress = getMemberNode()->addr;
    Address senderAddress(message.fromAddr);
	bool isSuccess = message.success;

	this->activeTransactions[transId].replyCount++;

	if (isSuccess){
		this->activeTransactions[transId].successCount++;
	}
	else{
		this->activeTransactions[transId].failureCount++;
	}
}

void MP2Node::addActiveTransaction(int transId, MessageType messageType, string key, string value, int sentToCount)
{
	TransactionDetail transactionDetail;
	transactionDetail.messageType = messageType;
	transactionDetail.transId = transId;
	transactionDetail.sentNodeCount = sentToCount;
	transactionDetail.replyCount = 0;
	transactionDetail.successCount = 0;
	transactionDetail.failureCount = 0;
	transactionDetail.timeSent = par->getcurrtime();
	transactionDetail.key = key;
	transactionDetail.value = value;
	this->activeTransactions.insert(pair<int, TransactionDetail>(transId, transactionDetail));
}

void MP2Node::checkAwaitedTransactions()
{
	Address myAddress = getMemberNode()->addr;

	vector<int> deleteTransIds;

	for (pair<int, TransactionDetail> i: this->activeTransactions){
		int currTime = par->getcurrtime();
		int transId = i.first;
		TransactionDetail transDetail = i.second;

		// -1: no action, 0: fail, 1: success.
		int result = -1;
		if (transDetail.successCount >= 2){
			result = 1;
		}
		else if (currTime - transDetail.timeSent > REPLYTIMEOUT || transDetail.failureCount >= 2){
		// else if (transDetail.failureCount >= 2){
			if (currTime - transDetail.timeSent > REPLYTIMEOUT && CUSTOMLOGENABLED == 1)
			{
				log->LOG(&myAddress, "CUSTOMLOG: Timeout.");				
			}
			result = 0;
		}

		// We need to log success or failure.
		if (result >= 0){
			switch(transDetail.messageType){
				case CREATE: 
					if (result == 1){
						log->logCreateSuccess(&myAddress, true, transId, transDetail.key, transDetail.value);
					}
					else{
						log->logCreateFail(&myAddress, true, transId, transDetail.key, transDetail.value);
					}
					break;
				case DELETE: 
					if (result == 1){
						log->logDeleteSuccess(&myAddress, true, transId, transDetail.key);
					}
					else{
						log->logDeleteFail(&myAddress, true, transId, transDetail.key);
					}
					break;
				case READ: 
					if (result == 1){
						log->logReadSuccess(&myAddress, true, transId, transDetail.key, transDetail.value);
					}
					else{
						log->logReadFail(&myAddress, true, transId, transDetail.key);
					}
					break;
				case UPDATE: 
					if (result == 1){
						log->logUpdateSuccess(&myAddress, true, transId, transDetail.key, transDetail.value);
					}
					else{
						log->logUpdateFail(&myAddress, true, transId, transDetail.key, transDetail.value);
					}
					break;
        		default:
					break;
    		}

		    // cout << "Remove transaction: " << transId << " of type: " << transDetail.messageType << endl;
			deleteTransIds.push_back(transId);
		}
	}

	for (int tId: deleteTransIds){
		this->activeTransactions.erase(tId);	
	}
}

/**
*
* CPF: Assign successors and predecessors to this node i.e. change hasMyReplicas and haveReplicasOf
*
*/
void MP2Node::assignReplicationNodes() {
    Node currentNode = Node(getMemberNode()->addr);
    if (hasMyReplicas.empty() || haveReplicasOf.empty()) {
        for (unsigned int i = 0; i < ring.size(); i++) {
            if (areNodesSame(ring[i], currentNode)){
				haveReplicasOf.push_back(ring[(i - 1 + ring.size()) % ring.size()]);
                haveReplicasOf.push_back(ring[(i - 2 + ring.size()) % ring.size()]);

                hasMyReplicas.push_back(ring[(i + 1) % ring.size()]);
                hasMyReplicas.push_back(ring[(i + 2) % ring.size()]);

				if (CUSTOMLOGENABLED == 1){
					log->LOG(&getMemberNode()->addr, "CUSTOMLOG: Initialized hasMyReplicas.");
				}
				return;
            }
        }
    }

	if (CUSTOMLOGENABLED == 1){
		log->LOG(&getMemberNode()->addr, "CUSTOMLOG: Could not initialize hasMyReplicas.");
	}
}