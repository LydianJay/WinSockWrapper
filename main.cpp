#include "socklib.h"
#include <iostream>

void queryMsg(INetClientMessage msg) {
	DWORD d;
	WriteConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), msg.msg, msg.msgSize, { 0,1 }, &d);
	
}

int main() {

	
	INet server(INET_PROTO::TCP, 1322);
	server.serverStartListening();
	server.serverStartReceving();
	server.serverStartQueryMsg(queryMsg);
	DWORD d;
	while (true) {
		WriteConsoleOutputCharacter(GetStdHandle(STD_OUTPUT_HANDLE), "Querying...", 12, { 0,0 }, &d);
	}
	

	return 0;
}