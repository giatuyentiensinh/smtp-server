#include "stdafx.h"
#include <Winsock2.h>
#include "MailSession.h"

char g_szDomain[300]="localhost";
char g_szDirectoryPath[300];
sqlite3 *db;

DWORD WINAPI ConnectionThread(void *param) {
	int len;
	char buf[2050];
	CMailSession *pSession = (CMailSession *)param;
	pSession->SendResponse(220);
	
	while (len = recv(pSession->m_socConnection, buf, sizeof(buf), 0)) {
		switch (pSession->ProcessCMD(buf, len)) {
		case 221:			
			char *error;
			for (int i = 0; i < pSession->m_nRcptCount; i++) {
				char *sql = (char*)calloc(1024, 1);
				sprintf(sql, "%s%s%s%s%s%s%s%s%s%s%s",
					"INSERT INTO EMAIL VALUES(NULL, '",
					pSession->m_FromAddress.GetAddress(),
					"', '",
					pSession->m_ToAddress[i].GetAddress(),
					"','",
					pSession->m_ToAddress[i].GetTime(),
					"','",
					pSession->m_ToAddress[i].GetMBoxPath(),
					"','",
					pSession->message,
					"');"
					);
				if (sqlite3_exec(db, sql, NULL, NULL, &error) != SQLITE_OK) {
					printf("\n Error sql: %s", error);
					sqlite3_free(error);
				}
				free(sql);
			}
			printf("\n Thread client closing...\n");
			return 0;			
		}
	}	
	return -1;
}

void AcceptConnections(SOCKET server_soc) {
	CMailSession *pSession;
	SOCKET soc_client;

	printf("SMTP Server is ready and listening to TCP port %d\n",SMTP_PORT);
	printf("--------------------------------------------------------------");

	while(true) {
		SOCKADDR cAddr;
		int clen = sizeof(SOCKADDR);
		printf("\nWaiting for incoming connection...\n");
		if (INVALID_SOCKET == (soc_client = accept(server_soc, &cAddr, &clen)))
			printf("\nError: Invalid Soceket returned by accept(): %d", WSAGetLastError());
		else 
			printf("\nAccepted new connection. Now creating session thread...");
		pSession = new CMailSession(soc_client);
		DWORD dwThreadId; 
		HANDLE hThread; 

		hThread = CreateThread(NULL, 0, ConnectionThread, (void *)pSession, 0, &dwThreadId);
		if (hThread == NULL) 
			printf("\nCreateThread failed.");
	}
}

int _tmain(int argc, _TCHAR* argv[]) {

	if (sqlite3_open(DB_FILE, &db)) {
		printf("\n Error opening SQLite3 database, message: %s", sqlite3_errmsg(db));
		sqlite3_close(db);
		return -1;
	}
	const char *sqlCreate = "CREATE TABLE EMAIL(id INTEGER PRIMARY KEY, sender STRING, recver STRING, time STRING, mailbox STRING, message STRING);";
	if (sqlite3_exec(db, sqlCreate, NULL, NULL, NULL)) {
		/*printf("\n Not create table, message: %s\n", sqlite3_errmsg(db));*/
	}

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa)) {
		printf("Error in  initializing.\nQuiting with error code: %d\n", WSAGetLastError());
		Sleep(5000);
		exit(WSAGetLastError());
	}

	SOCKET soc = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (soc == INVALID_SOCKET) {
		printf("Error: Invalid socket\nQuiting with error code: %d\n", WSAGetLastError());
		Sleep(5000);
		exit(WSAGetLastError());
	}

	SOCKADDR_IN sAddr;
	sAddr.sin_family = AF_INET;
	sAddr.sin_port = htons(SMTP_PORT);
	LPHOSTENT lpHost = gethostbyname(g_szDomain);
	sAddr.sin_addr = *(LPIN_ADDR)(lpHost->h_addr_list[0]);
	//sAddr.sin_addr.S_un.S_addr = htonl(ADDR_ANY);

	if (bind(soc, (const struct sockaddr*)&sAddr, sizeof(sAddr))) {
		printf("Error: Can not bind socket. Another server running?\nQuiting with error code: %d\n",WSAGetLastError());
		Sleep(5000);
		exit(WSAGetLastError());
	}

	if (SOCKET_ERROR == listen(soc, 5))	{
		printf("Error: Can not listen to socket\nQuiting with error code: %d\n",WSAGetLastError());
		Sleep(5000);
		exit(WSAGetLastError());
	}

	char direct[300], f[300];
	GetFullPathName("./", 255, (LPTSTR)direct, (LPTSTR *)f);
	strcpy(g_szDirectoryPath, direct);
	printf("Active directory path %s\n", g_szDirectoryPath);
	AcceptConnections(soc);

	closesocket(soc);
	WSACleanup();
	sqlite3_close(db);
	printf("You should not see this message.\nIt is an abnormal condition.\nTerminating...");
	return 0;
}