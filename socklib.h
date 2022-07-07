#ifndef _NETWORK_LIB_H
#define _NETWORK_LIB_H

#ifdef UNICODE
#undef UNICODE
#elif defined _UNICODE
#undef _UNICODE
#endif // UNICODE


#ifndef _WIN32
#warning Windows only
#endif // MSC_VER


#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <vector>

#define __INET_MSG_SIZE 2048 // 2 bytes

enum class INET_PROTO {
	TCP = SOCK_STREAM, UDP = SOCK_DGRAM
};

enum class NET_TYPE
{
	SERVER_TO_CLIENT,
	CLIENT_TO_SERVER
};

typedef struct INetClient {
	SOCKET			clientSocket;
	sockaddr_in		clientAddrInfo;
};
// this struct will contain info about the message
typedef struct INetClientMessage {
	char			msg[__INET_MSG_SIZE];	// message
	unsigned int	msgSize;				// message size
	INetClient*		msgOwner;				// message owner
};

typedef struct INetServerReply {
	char			msg[__INET_MSG_SIZE];
	uint32_t		msgSize;
	INetClient*		msgReceiver;
};

class INet {

private:

	// -------------------------- ----------------------------------------------
	// this will probably get deprecated
	WSADATA				m_WSAData;
	sockaddr_in			m_clientAdd;
	sockaddr_in			m_serverAdd; // the server address to be connected  to
	std::thread			m_comThread;
	bool				m_isConnectionEstablised = false;
	SOCKET				m_clientSocket; // the client that is connecting to the server
	SOCKET				m_winSock;		// server socket
	unsigned short		m_portNumber;
	sockaddr_in			m_sockInfo;		// incoming connection socket info
	NET_TYPE			m_netType;

	// -------------------------- New Version ---------------------------- //

	// Server --
	bool							m_isQuerying = false;	// flag to see if msg query is active
	bool							m_isReceiving = false;	// flag to see if thread is still receiving 
	bool							m_isListening = false;	// flag to see if the listening thread already started
	SOCKET							m_serverSocket;			// the server socket id... of some sort
	sockaddr_in						m_serverSocketInfo;		// i really dunno what is this... transport addres of some sort
	std::vector<INetClient>			m_establisedConn;		// currently established connections
	std::queue<INetClientMessage>	m_msgQueue;				// the received message queue.
	std::queue<INetServerReply>		m_msgToSendQueue;		// message to send to
	std::thread						m_listeningThread;		// thread use to listen for connections
	std::thread						m_getDataThread;		// thread use to get data from the establised connections
	std::thread						m_queryMsgThread;		// the thread to use to query msg from the queue
	
	

public:
	void		serverStartListening();									// starts the listening thread for an incoming connection;
	void		serverStartReceving();									// starts the receving thread for the messages received
	void		serverStartQueryMsg(void(*query)(INetClientMessage));	// start the thread for msg query
	uint32_t	getConnectedClientCount();								// returns the number of connected client
	void		sendMessage(INetServerReply);							// adds a message to the reply queue
private:
	void		queryMessage(void(*query)(INetClientMessage));			// takes a function pointer that this function will for the user to query the message queue	
	void		serverListening();										// the function that the serverStartListening() will call
	void		serverReceiving();										// the function that the serverStarReceiving will call

	// ---------------------------------------------------------------------- //

private:
	void Receiving(void(*Receiver)(char* data, unsigned int nDataSize));
	void Listening();
	void Connecting();
public:


	/*
	---------------------------------------------------------------------------------------------
		This method start a thread that waits for an
		incomming connection to established
		should only be called once
	---------------------------------------------------------------------------------------------
	*/
	void ListenForConnection();

	/*
	---------------------------------------------------------------------------------------------
		This method returns true if a connection is establised
		ListforConnection() should be called first before querying

		-----------------------------------------------------------------------------------------
		@return - returns true if connection is established
		-----------------------------------------------------------------------------------------

	---------------------------------------------------------------------------------------------
	*/
	bool isEstablised();

	/*
	---------------------------------------------------------------------------------------------
		This method sends data the in the buffer to a established connection
		isEstablished() must be called first to check if connection exist

		--- this method may block the program ---

		-----------------------------------------------------------------------------------------
		@param data - a pointer to buffer that contains the data to be sent
		-----------------------------------------------------------------------------------------

		-----------------------------------------------------------------------------------------
		@param nDataSize - the size of the data in bytes
		-----------------------------------------------------------------------------------------

		-----------------------------------------------------------------------------------------
		@return - returns the number of bytes that was sent
		-----------------------------------------------------------------------------------------

	---------------------------------------------------------------------------------------------
	*/
	unsigned int sendData(char* data, unsigned int nDataSize);

	/*
	---------------------------------------------------------------------------------------------
		This method stops the thread from the ListenForConnection() method
		and uses the existing thread to try to catch any data that was received
		isEstablished() must be called first to check if connection exist

		-----------------------------------------------------------------------------------------
		@param void(*CallBackFunc)(char *, unsigned int) - this function pointer will be called if
		a data was received. the function will have a parameter that contains a pointer to buffer
		data and the size of the data in bytes
		-----------------------------------------------------------------------------------------

	---------------------------------------------------------------------------------------------

	*/
	void StartReceivingData(void (*CallBackFunc)(char* data, unsigned int nDataSize));

	/*
	---------------------------------------------------------------------------------------------
		This method will start a thread then tries to connect to a host
		it should ebe only called once. To check if connected call the isEstablished() method

		-----------------------------------------------------------------------------------------
		@param IPAdd - is a std::string that will contain the ip address of the host
		-----------------------------------------------------------------------------------------

	---------------------------------------------------------------------------------------------
	*/
	void ConnectToServer(std::string IPAdd);


	/*
	---------------------------------------------------------------------------------------------
		Closes the connection if there is a connection established
	---------------------------------------------------------------------------------------------
	*/
	void CloseConnection();

	INet();
	INet(NET_TYPE);
	INet(INET_PROTO, unsigned short PortNumber);


	~INet();
};





#endif // _NETWORK_LIB_H