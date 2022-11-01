/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file (Revised 2020)
 *
 * MP2 Starter template version
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "Message.h"
#include "Queue.h"

#define REPLYTIMEOUT 4

/**
 * CLASS NAME: TransactionDetail
 *
 * DESCRIPTION: This class encapsulates details of a transaction.
 */
struct TransactionDetail {
	// Transaction id.
	int transId;

	// Count of nodes where this transaction was sent.
	int sentNodeCount;

	// Count of replies received so far.
	int replyCount;

	// Count of success replies so far.
	int successCount;

	// Count of failed replies so far.
	int failureCount;

	// Time at which message was sent.
	int timeSent;

	// Type of message related to this transaction.
	MessageType messageType;

	// Key.
	string key;

	// value.
	string value;
};

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
	// string delimiter
	string delimiter;
	// Transaction map
	map<int, Message> transMap;

	// Active transactions at this coordinator.
	map<int, TransactionDetail> activeTransactions;

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	bool compareNode(const Node& first, const Node& second) {
		return first.nodeHashCode < second.nodeHashCode;
	}

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();
	
	// Methods to handle different type of messages.
    void handleCreateMessage(Message message);
    void handleReadMessage(Message message);
    void handleReplyMessage(Message message);
    void handleReadReplyMessage(Message message);
    void handleDeleteMessage(Message message);
    void handleUpdateMessage(Message message);
	void addActiveTransaction(int transId, MessageType messageType, string key, string value, int sentToCount);
	void checkAwaitedTransactions();
	void propagateMessageFromClient(MessageType msgType, string key, string value);
	bool isMembershipStale(vector<Node> currentMembershipList);
	bool areNodesSame(Node node1, Node node2);
	vector<pair<string, string>> findMyKeys(ReplicaType rep_type);
	int ifExistNode(vector<Node> v, Node n1);
	void assignReplicationNodes();
	bool nodeExistsInList(vector<Node> list, Node node);

	~MP2Node();
};

#endif /* MP2NODE_H_ */
