#include "socklib.h"



//-----------------------------------------------------------
/*
* Initialize a thread that starts listening for connection and adds them to connection array
* 
* @param none
* @return none
*/
void INetServer::serverStartListening()
{
	bind(m_serverSocket, reinterpret_cast<const sockaddr*>(&m_serverSocketInfo), sizeof(m_serverSocketInfo));
	listen(m_serverSocket, SOMAXCONN);
	

	if (!m_isListening) {// check if thread already been running
		m_isListening = true;
		m_listeningThread = std::thread(&INetServer::serverListening, this);	// start a thread
	}								

}
/*
* This called from the serverStartListening and this called from a new thread
* 
* @param none
* @return none
*/
void INetServer::serverListening()
{
	if (!m_isListening)return;

	while (m_isListening) {

		INetClientInfo client;

		int errorcode = 0;
		int size = sizeof(client.clientAddrInfo);
		client.clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&client.clientAddrInfo), &size);

		if (client.clientSocket != INVALID_SOCKET)
		{
			m_establisedConn.push_back(client);
		}
		else {
			errorcode = WSAGetLastError();
		}

	}
	
}
/*
* Initialize/Start a thread that will call serverReceiving that will
* add message/packets to the message queue
* @param none
* @return none
*/
void INetServer::serverStartReceving()
{
	if (m_isReceiving)return;

	m_getDataThread = std::thread(&INetServer::serverReceiving, this);
	m_isReceiving = true;
}


/*
* This function will be called by serverStartReceving with a new thread
* @param none
* @return none
*/
void INetServer::serverReceiving() {
	while (m_isReceiving) {
		
		
		int length = m_establisedConn.size();

		for (size_t i = 0; i < length; i++) {
			INetClientInfo& client = m_establisedConn[i];

			INetClientMessage clientMsg;

			ZeroMemory(clientMsg.msg, __INET_MSG_SIZE);
			int dataSize = 0;
			dataSize = recv(client.clientSocket, clientMsg.msg, __INET_MSG_SIZE, 0);
			clientMsg.msgSize = dataSize;
			if (dataSize == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET) {
				shutdown(client.clientSocket, SD_BOTH);
				closesocket(client.clientSocket);
				m_establisedConn.erase(m_establisedConn.begin() + i);
			}
			else {
				m_msgQueue.push(std::move(clientMsg));
			}

		}

		

	}
}


/*
* 
* Initialize a thread that will start sending message from the msgtosend queue
* @param none
* @return none
*/
void INetServer::serverStartSending() {
	if (m_isPacketsSend)return;

	m_isPacketsSend = true;
	m_packetSenderThread = std::thread(&INetServer::serverSendingPackets, this);

}
/*
* This function will be called from serverStartSending with a thread
* @param none
* @return none
*/
void INetServer::serverSendingPackets() {

	while (m_isPacketsSend){

		if (!m_msgToSendQueue.empty()) {
			const INetServerReply& reply = std::move(m_msgToSendQueue.front());
			
			unsigned int totalBytes = send(reply.msgReceiver.clientSocket, (const char*)(reply.msg), reply.msgSize, 0);
			m_msgToSendQueue.pop();

		}
	}

}


/*
* Returns how many connections is active
* @param none
* @return number of connections active
*/
uint32_t INetServer::getConnectedClientCount(){
	return m_establisedConn.size();
}
/*
* This function initialize a thread that will call a function and 
* that function will call the function pointer when a message/packet is available
* @param query - the function pointer that will received the message/packets when there is available
* @return none
*/
void INetServer::serverStartQueryMsg(void(*query)(INetClientMessage)){
	if (m_isQuerying)return;
	m_isQuerying = true;

	m_queryMsgThread = std::thread(&INetServer::queryMessage, this, query);
}

/*
* This function will be called from serverStartQueryMsg
* @param query - the function pointer that will be called when message/packet is available
* @return none
*/
void INetServer::queryMessage(void(*query)(INetClientMessage)){
	
	while (m_isQuerying){
		
		if (!m_msgQueue.empty()) {
			INetClientMessage clientMsg = std::move(m_msgQueue.front());
			m_msgQueue.pop();
			query(std::move(clientMsg));
		}
	}
}

/*
* Sends message/packet
* @param serverReply - this structure will contain the message, message size, and the receiver
* @return none
*/
void INetServer::sendMessage(INetServerReply serverReply)	{
	m_msgToSendQueue.push(std::move(serverReply));
}

/*
* This constructor defaults the value of the port number to 80
* with a TCP protocol
*/
INetServer::INetServer()
{
	m_portNumber = 80;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {
		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}
	
	m_serverSocket = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	

	if (m_serverSocket == INVALID_SOCKET)
	{
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}

	m_serverSocketInfo.sin_family = AF_INET;
	m_serverSocketInfo.sin_port = htons(m_portNumber);
	m_serverSocketInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}
/*
* @param protocol - the desired protocol TCP or UDP. INET_PROTO::TCP/UDP
* @param portNumber - the desired port number
*/
INetServer::INetServer(INET_PROTO protocol, unsigned short PortNumber)
{
	m_portNumber = PortNumber;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {
		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}



	if (protocol == INET_PROTO::TCP)
		m_serverSocket = socket(AF_INET, (int)protocol, IPPROTO_TCP);
	else
		m_serverSocket = socket(AF_INET, (int)protocol, IPPROTO_UDP);

	if (m_serverSocket == INVALID_SOCKET)
	{
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}

	m_serverSocketInfo.sin_family = AF_INET;
	m_serverSocketInfo.sin_port = htons(m_portNumber);
	m_serverSocketInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
}

INetServer::~INetServer() {
	m_isListening = false;
	m_isPacketsSend = false;
	m_isQuerying = false;
	m_isReceiving = false;
	
	m_getDataThread.join();
	m_listeningThread.join();
	m_packetSenderThread.join();
	m_queryMsgThread.join();

	for (const INetClientInfo& client : m_establisedConn) {
		shutdown(client.clientSocket, SD_BOTH);
		closesocket(client.clientSocket);
	}

	m_establisedConn.clear();
	
	int length = m_msgQueue.size();

	for (size_t i = 0; i < length; i++) {
		m_msgQueue.pop();
	}

	length = m_msgToSendQueue.size();
	for (size_t i = 0; i < length; i++) {
		m_msgToSendQueue.pop();
	}

	closesocket(m_serverSocket);
}



// -------------------------------------------------------------------






/*
* Set port number to 80 and protocol to TCP
* 
*/
INetClient::INetClient() {
	m_portNumber = 80;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {
		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}



	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_Socket == INVALID_SOCKET) {
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}
}
/*
* @param protocol - the desired protocol TCP or UDP. INET_PROTO::TCP/UDP
* @param portNumber - the desired port number
*/
INetClient::INetClient(INET_PROTO protocol, unsigned short portNum) {


	m_portNumber = portNum;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {
		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}



	if (protocol == INET_PROTO::TCP)
		m_Socket = socket(AF_INET, (int)protocol, IPPROTO_TCP);
	else
		m_Socket = socket(AF_INET, (int)protocol, IPPROTO_UDP);

	if (m_Socket == INVALID_SOCKET) {
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}

}
/*
* Initializae a thread that will constantly tries to connect to the server
* 
* @param ip - the ip address in string
* @return none
*/
void INetClient::connectToServer(std::string ip) {

	if (!m_isConnected)
	{
		m_serverAddr.sin_port = htons(m_portNumber);
		m_serverAddr.sin_family = AF_INET;
		inet_pton(AF_INET, ip.c_str(), &m_serverAddr.sin_addr);

		int result = connect(m_Socket, reinterpret_cast<const sockaddr*>(&m_serverAddr), sizeof(m_serverAddr));
		
		if (result != SOCKET_ERROR) {
			m_isConnected = true;
			
			if (m_recvThread.joinable()) m_recvThread.join();
			
			m_recvThread = std::thread(&INetClient::acceptingMsg, this);

		}
	}

}
/*
* Disconnects from the server
* @param none
* @retun none
*/
void INetClient::disconnectToServer(){

	m_isConnected = false;
	shutdown(m_Socket, SD_BOTH);
	closesocket(m_Socket);
	
}
/*
* returns if there is a connection establised
* @param none
* @return returns true if connection is establised otherwise return false
*/
bool INetClient::isConnected() {	return m_isConnected; }


/*
* 
* sends message/packet to the server
* @param msg - the msg that will be send
* @return returns true if msg was send successfuly
*/
bool INetClient::sendToServer(INetMsg msg) {
	
	int bytesSend = send(m_Socket, msg.msg, msg.msgSize, 0);

	if (bytesSend == SOCKET_ERROR)return false;
	return true;
}
/*
* sends message/packet to the server
* @param msg - the msg array that will be send
* @param count - the count of msg array
* @return returns true if all msg was send successfuly
*/
bool INetClient::sendToServer(INetMsg* msgBlock, uint32_t count) {
	bool allWasSendSuccesfully = true;
	for (int i = 0; i < count; i++) {
		int bytesSend = send(m_Socket, msgBlock[i].msg, msgBlock[i].msgSize, 0);

		if (bytesSend == SOCKET_ERROR) {
			allWasSendSuccesfully = false;
		}
	}
	return allWasSendSuccesfully;
}

/*
* gets the top message/packet in the message queue
* @param[out] msgQueueSize - will be set to the number of remaining msg/packets in the queue
* @return returns a INetMsg structure that will contain the message/packet and the size of msg in bytes
*/

INetMsg INetClient::getMessage(unsigned int * msgQueueSize){
	if (m_msgQueue.empty()) {
		if (msgQueueSize != nullptr) *msgQueueSize = 0;
		INetMsg msg;
		msg.msgSize = 0;
		return std::move(msg);
	}


	INetMsg msg = std::move(m_msgQueue.front());
	m_msgQueue.pop();
	*msgQueueSize = m_msgQueue.size();
	return std::move(msg);
}
/*
* This function will be called from with a thread and this will accepts and store msg/packets
* in the message queue
* @param none
* @return none
*/
void INetClient::acceptingMsg() {
	
	while (m_isConnected) {

		INetMsg msg;

		int bytesReceived = recv(m_Socket, msg.msg, __INET_MSG_SIZE, 0);

		if (bytesReceived != SOCKET_ERROR) {
			msg.msgSize = bytesReceived;
			m_msgQueue.push(std::move(msg));
		}
	}

}

