#include "socklib.h"

void INet::ListenForConnection()
{
	bind(m_winSock, reinterpret_cast<const sockaddr*>(&m_sockInfo), sizeof(m_sockInfo));
	listen(m_winSock, SOMAXCONN);
	if (!m_isConnectionEstablised)
		m_comThread = std::thread(&INet::Listening, this);

}

bool INet::isEstablised()
{
	return m_isConnectionEstablised;
}

void INet::StartReceivingData(void(*CallBackFunc)(char* data, unsigned int nDataSize))
{
	if (!m_isConnectionEstablised)return;

	// the thread migth have not yet joined
	if (m_comThread.joinable())m_comThread.join();

	m_comThread = std::thread(&INet::Receiving, this, CallBackFunc);

}

void INet::ConnectToServer(std::string IPAdd)
{
	if (!m_isConnectionEstablised)
	{
		m_serverAdd.sin_port = htons(m_portNumber);
		m_serverAdd.sin_family = AF_INET;
		inet_pton(AF_INET, IPAdd.c_str(), &m_serverAdd.sin_addr);

		if (m_comThread.joinable())m_comThread.join();

		m_comThread = std::thread(&INet::Connecting, this);

	}


}

void INet::CloseConnection()
{
	if (m_isConnectionEstablised)
	{
		m_isConnectionEstablised = false;
		if (m_comThread.joinable())m_comThread.join();

		switch (m_netType)
		{
		case NET_TYPE::SERVER_TO_CLIENT:
			shutdown(m_clientSocket, SD_SEND);
			break;
		case NET_TYPE::CLIENT_TO_SERVER:

			closesocket(m_winSock);
			break;
		default:
			break;
		}


	}


}


//-----------------------------------------------------------

void INet::serverStartListening()
{
	bind(m_serverSocket, reinterpret_cast<const sockaddr*>(&m_serverSocketInfo), sizeof(m_serverSocketInfo));
	listen(m_serverSocket, SOMAXCONN);
	

	if (!m_isListening) {// check if thread already been running
		m_isListening = true;
		m_listeningThread = std::thread(&INet::serverListening, this);	// start a thread
	}								

}

void INet::serverListening()
{
	if (!m_isListening)return;
	INetClient client;
	
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
	std::cout << "fuck";
}

void INet::serverStartReceving()
{
	if (m_isReceiving)return;

	m_getDataThread = std::thread(&INet::serverReceiving, this);
	m_isReceiving = true;
}



void INet::serverReceiving() {
	while (m_isReceiving) {
		
		
		int length = m_establisedConn.size();

		for (size_t i = 0; i < length; i++) {
			INetClient& client = m_establisedConn[i];

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




uint32_t INet::getConnectedClientCount(){
	return m_establisedConn.size();
}
void INet::serverStartQueryMsg(void(*query)(INetClientMessage)){
	if (m_isQuerying)return;
	m_isQuerying = true;

	m_queryMsgThread = std::thread(&INet::queryMessage, this, query);
}

void INet::queryMessage(void(*query)(INetClientMessage)){
	
	while (m_isQuerying){
		
		if (!m_msgQueue.empty()) {
			INetClientMessage clientMsg = std::move(m_msgQueue.front());
			m_msgQueue.pop();
			query(std::move(clientMsg));
		}
	}
}



void INet::sendMessage(INetServerReply serverReply)	{
	m_msgToSendQueue.push(std::move(serverReply));
}





// -------------------------------------------------------------------




void INet::Receiving(void(*CallBackFunc)(char* data, unsigned int nDataSize))
{

	while (m_isConnectionEstablised)
	{
		char buffer[256];
		ZeroMemory(buffer, 256);
		int dataSize = 0;
		switch (m_netType)
		{
		case NET_TYPE::SERVER_TO_CLIENT:
			dataSize = recv(m_clientSocket, (char*)buffer, 256, 0);
			break;
		case NET_TYPE::CLIENT_TO_SERVER:
			dataSize = recv(m_winSock, (char*)buffer, 256, 0);
			break;
		}
		if (dataSize == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
		{
			CloseConnection();
		}
		else if (dataSize > 0 && CallBackFunc != nullptr)
			CallBackFunc(buffer, dataSize);
	}

}

void INet::Listening()
{
	int size = sizeof(m_clientAdd);
	m_clientSocket = accept(m_winSock, reinterpret_cast<sockaddr*>(&m_clientAdd), &size);

	if (m_clientSocket != INVALID_SOCKET)
	{
		m_isConnectionEstablised = true;
		return;
	}
}

void INet::Connecting()
{

	int result = connect(m_winSock, reinterpret_cast<const sockaddr*>(&m_serverAdd), sizeof(m_serverAdd));
	while (result != 0)
	{
		result = connect(m_winSock, reinterpret_cast<const sockaddr*>(&m_serverAdd), sizeof(m_serverAdd));
	}
	m_isConnectionEstablised = true;
}

INet::INet()
{
	m_netType = NET_TYPE::SERVER_TO_CLIENT;
	m_portNumber = 1322;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {

		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}

	m_winSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_winSock == INVALID_SOCKET)
	{
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}

	m_sockInfo.sin_family = AF_INET;
	m_sockInfo.sin_port = htons(m_portNumber);
	m_sockInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

}

INet::INet(NET_TYPE type)
{
	m_portNumber = 1322;
	if (WSAStartup(MAKEWORD(2, 2), &m_WSAData) != 0) {

		//std::cout << "Startup fail! ERROR " << WSAGetLastError() << '\n';
	}

	m_winSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_winSock == INVALID_SOCKET)
	{
		//std::cout << "Socket error ERROR " << WSAGetLastError() << '\n';
		WSACleanup();
	}

	m_sockInfo.sin_family = AF_INET;
	m_sockInfo.sin_port = htons(m_portNumber);
	m_sockInfo.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	m_netType = type;
}

INet::INet(INET_PROTO protocol, unsigned short portNum)
{
	m_portNumber = portNum;
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


unsigned int INet::sendData(char* data, unsigned int nDataSize)
{


	if (!m_isConnectionEstablised)return 0;
	unsigned int totalBytes = 0;
	switch (m_netType)
	{
	case NET_TYPE::CLIENT_TO_SERVER:
		totalBytes = send(m_winSock, (const char*)(data), nDataSize, 0);
		break;
	case NET_TYPE::SERVER_TO_CLIENT:
		totalBytes = send(m_clientSocket, (const char*)(data), nDataSize, 0);
		break;
	default:
		break;
	}

	if (totalBytes == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET)
	{

		m_isConnectionEstablised = false;
		if (m_comThread.joinable())m_comThread.join();
	}

	return totalBytes;

}


INet::~INet()
{
	WSACleanup();
	if (m_winSock != INVALID_SOCKET)
		closesocket(m_winSock);

	if (m_comThread.joinable())m_comThread.join();
}