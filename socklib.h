#ifndef _NETWORK_LIB_H
#define _NETWORK_LIB_H

/*
* 
* 
*								MIT License
*
*	Copyright(c) 2022 Lloyd Jay Arpilleda Edradan
*
*	Permission is hereby granted, free of charge, to any person obtaining a copy
*	of this softwareand associated documentation files(the "Software"), to deal
*	in the Software without restriction, including without limitation the rights
*	to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
*	copies of the Software, and to permit persons to whom the Software is
*	furnished to do so, subject to the following conditions :
*
*	The above copyright noticeand this permission notice shall be included in all
*	copies or substantial portions of the Software.
*
*	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
*	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	SOFTWARE.
* 
* 
*/


/* --------------------------------------------------------------------	*/
/*								Version: 1.0							*/
/* --------------------------------------------------------------------	*/
/*
* ChangeLog: (July 8, 2022)
*	provides basic interface for sockets 
*/

/* --------------------------------------------------------------------	*/
/*								TODO									*/
/* --------------------------------------------------------------------	*/
/*
* Test Implemtation
* 
*/


#ifdef UNICODE		// no unicode for me
#undef UNICODE
#elif defined _UNICODE
#undef _UNICODE
#endif // UNICODE


#ifndef _WIN32		// code only works on windows
#error Windows only
#endif


#pragma comment(lib, "ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include <string>
#include <queue>
#include <vector>

constexpr unsigned int __INET_MSG_SIZE = 2048; // 2 4bytes

enum class INET_PROTO {
	TCP = SOCK_STREAM, UDP = SOCK_DGRAM
};

typedef struct INetClientInfo {
	SOCKET			clientSocket;
	sockaddr_in		clientAddrInfo;
};
// this struct will contain info about the message
typedef struct INetClientMessage {
	char			msg[__INET_MSG_SIZE];	// message
	unsigned int	msgSize;				// message size
	INetClientInfo	msgOwner;				// message owner
};

typedef struct INetServerReply {
	char			msg[__INET_MSG_SIZE];
	uint32_t		msgSize;
	INetClientInfo	msgReceiver;
};

typedef struct INetMsg {
	char			msg[__INET_MSG_SIZE];
	uint32_t		msgSize;
};



/* ---------------------------------------------------------------------------- */
/*						The server socket interface								*/
/* ---------------------------------------------------------------------------- */

class INetServer {

public:
	void		serverStartListening();									// starts the listening thread for an incoming connection;
	void		serverStartReceving();									// starts the receving thread for the messages received
	void		serverStartQueryMsg(void(*query)(INetClientMessage));	// start the thread for msg query
	uint32_t	getConnectedClientCount();								// returns the number of connected client
	void		sendMessage(INetServerReply);							// adds a message to the reply queue

	INetServer();
	INetServer(INET_PROTO, unsigned short PortNumber);
	~INetServer();
private:
	
	bool							m_isQuerying = false;	// flag to see if msg query is active
	bool							m_isReceiving = false;	// flag to see if thread is still receiving 
	bool							m_isListening = false;	// flag to see if the listening thread already started
	bool							m_isPacketsSend = false;// flag to see if packet sending thread is running
	SOCKET							m_serverSocket;			// the server socket id... of some sort
	sockaddr_in						m_serverSocketInfo;		// i really dunno what is this... transport addres of some sort
	std::vector<INetClientInfo>		m_establisedConn;		// currently established connections
	std::queue<INetClientMessage>	m_msgQueue;				// the received message queue.
	std::queue<INetServerReply>		m_msgToSendQueue;		// message queue to send to
	std::thread						m_listeningThread;		// thread use to listen for connections
	std::thread						m_getDataThread;		// thread use to get data from the establised connections
	std::thread						m_queryMsgThread;		// the thread to use to query msg from the queue
	std::thread						m_packetSenderThread;	// the thread that will run serverSendingPackets()
	WSADATA							m_WSAData;				// wsa
	unsigned short					m_portNumber;			// port number

	void		queryMessage(void(*query)(INetClientMessage));			// takes a function pointer that this function will for the user to query the message queue	
	void		serverListening();										// the function that the serverStartListening() will call
	void		serverReceiving();										// the function that the serverStarReceiving will call
	void		serverStartSending();									// this function will start a thread that will send the packets
	void		serverSendingPackets();									// this funtion will be called to another thread to start sending data from reply queue
};

/* ---------------------------------------------------------------------------- */
/*						The client socket interface								*/
/* ---------------------------------------------------------------------------- */

class INetClient {

public:
	INetClient();											// defaults the protocol to be TCP with port number 80
	INetClient(INET_PROTO protocol, unsigned short portNum);// construct with the assigned protocol and port number
	
	void connectToServer(std::string ip);					// connects to the server with ip
	void disconnectToServer();								// send server a signal to disconnect
	bool isConnected();										// query if we are connected to a server
	bool sendToServer(INetMsg msg);							// sends packet to server
	bool sendToServer(INetMsg * msgBlock, uint32_t count);	// sends multiple packets to server
	INetMsg getMessage(unsigned int* msgQueueSize);			// returns the message in front of the message queue
private:
	void acceptingMsg();

	WSADATA							m_WSAData;				// WSA
	SOCKET							m_Socket;				// socket
	sockaddr_in						m_serverAddr;			// the server the address 
	unsigned short					m_portNumber;			// port number
	bool							m_isConnected = false;	// flag if we are connected to a server
	std::thread						m_recvThread;			// the thread that will accumulate the packets received
	std::queue<INetMsg>				m_msgQueue;				// the packets received queue

};


#endif // _NETWORK_LIB_H