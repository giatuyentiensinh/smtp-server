#include "stdafx.h"
#include "MailSession.h"
#include "shlwapi.h"

#pragma comment(lib,"shlwapi.lib")

extern char g_szDomain[300];

#define TRACE printf

int CMailSession::ProcessCMD(char *buf, int len) {
	//printf("\n------------------- CMD -------------------\n");
	if (m_nStatus == SMTP_STATUS_DATA) {
		return ProcessDATA(buf, len);
	}
	else if (_strnicmp(buf, "HELO", 4) == 0) {
		return ProcessHELO(buf, len);
	}
	else if (_strnicmp(buf, "EHLO", 4) == 0) {
		return ProcessHELO(buf, len);
	}
	else if (_strnicmp(buf, "MAIL", 4) == 0) {
		return ProcessMAIL(buf, len);
	}
	else if (_strnicmp(buf, "RCPT", 4) == 0) {
		return ProcessRCPT(buf, len);
	}
	else if (_strnicmp(buf, "DATA", 4) == 0) {
		return ProcessDATA(buf,len);
	}
	else if (_strnicmp(buf, "QUIT", 4) == 0) {
		return ProcessQUIT(buf,len);
	}
	else 
		return ProcessNotImplemented(false);
}
int CMailSession::ProcessHELO(char *buf, int len) {
	buf+=5;
	m_nStatus=SMTP_STATUS_HELO;
	m_FromAddress.SetAddress("");
	m_nRcptCount=0;

	CreateNewMessage();

	return SendResponse(250);
}

/*
MAIL
S: 250
F: 552, 451, 452
E: 500, 501, 421
*/
int CMailSession::ProcessMAIL(char *buf, int len) {
	char address[MAX_ADDRESS_LENGTH+5];
	char *st,*en;
	__w64 int alen;

	if(m_nStatus!=SMTP_STATUS_HELO) {
		return SendResponse(503);
	}

	memset(address,0,sizeof(address));

	st=strchr(buf,'<');
	en=strchr(buf,'>');
	st++;

	alen=en-st;
	strncpy(address,st,alen);


	printf("FROM [%s]",address);

	if(!CMailAddress::AddressValid(address)) {
		return SendResponse(501);
	}
	m_FromAddress.SetAddress(address);
	return SendResponse(250);
}

/*
RCPT
S: 250, 251
F: 550, 551, 552, 553, 450, 451, 452
E: 500, 501, 503, 421
*/
int CMailSession::ProcessRCPT(char *buf, int len) {
	char address[MAX_ADDRESS_LENGTH + 5];
	char user[MAX_USER_LENGTH + 5];
	char tdom[MAX_DOMAIN_LENGTH + 5];
	char szUserPath[MAX_PATH + 1];
	char *st, *en, *domain = tdom;
	__w64 int alen;

	if (m_nStatus != SMTP_STATUS_HELO) {
		//503 Bad Command
		return SendResponse(503);
	}

	if (m_nRcptCount >= MAX_RCPT_ALLOWED) {
		//552 Requested mail action aborted: exceeded storage allocation
		return SendResponse(552);
	}

	memset(address, 0, sizeof(address));

	st = strchr(buf, '<');
	en = strchr(buf, '>');
	st++;

	alen = en - st;
	strncpy(address, st, alen);

	domain = strchr(address, '@');
	domain += 1;

	memset(user, 0, sizeof(user));
	strncpy(user, address, strlen(address) - strlen(domain) - 1);

	printf("RCPT [%s] User [%s] Domain [%s]\n", address, user, domain);

	char domain_path[300];
	sprintf(domain_path, "%s%s", DIRECTORY_ROOT, domain);

	if (PathFileExists(domain_path)) {
		sprintf(szUserPath, "%s\\%s", domain_path, user);
		if (!PathFileExists(szUserPath)) {
			TRACE("PathFileExists(%s) FALSE\n", szUserPath);
			printf("User not found on this domain\n");
			return SendResponse(550);
		}
	}
	else {
		TRACE("PathFileExists(%s) FALSE\n", domain_path);
		return SendResponse(551);
	}

	m_ToAddress[m_nRcptCount].SetMBoxPath(szUserPath);
	m_ToAddress[m_nRcptCount].SetAddress(address);
	m_nRcptCount++;
	printf("%s %s %s\n", m_ToAddress[m_nRcptCount].GetAddress(), m_ToAddress[m_nRcptCount].GetDomain(), m_ToAddress[m_nRcptCount].GetUser());
	return SendResponse(250);
}

int CMailSession::ProcessDATA(char *buf, int len) {
	DWORD dwIn = len, dwOut;

	if (m_nStatus != SMTP_STATUS_DATA) {
		m_nStatus = SMTP_STATUS_DATA;
		return SendResponse(354);
	}

	if (strstr(buf, SMTP_DATA_TERMINATOR)) {		
		m_nStatus = SMTP_STATUS_DATA_END;
		return ProcessDATAEnd();
	}

	WriteFile(m_hMsgFile, buf, dwIn, &dwOut, NULL);

	return 220;
}

int CMailSession::ProcessQUIT(char *buf, int len) {
	printf("Quit session\n");
	return SendResponse(221);
}

int CMailSession::ProcessNotImplemented(bool bParam) {
	if (bParam) {
		return SendResponse(504);
	}
	else return SendResponse(502);
}

int CMailSession::SendResponse(int nResponseType) {
	char buf[100];
	int len;
	if (nResponseType == 220)
		sprintf(buf, "220 %s Welcome to SMTP server \r\n", DOMAIN_NAME);
	else if (nResponseType == 221)
		strcpy(buf, "221 Service closing transmission channel\r\n");
	else if (nResponseType == 250)
		strcpy(buf, "250 OK\r\n");
	else if (nResponseType == 354)
		strcpy(buf, "354 Start mail input; end with .<CRLF>\r\n");
	else if (nResponseType == 501)
		strcpy(buf, "501 Syntax error in parameters or arguments\r\n");
	else if (nResponseType == 502)
		strcpy(buf, "502 Command not implemented\r\n");
	else if (nResponseType == 503)
		strcpy(buf, "503 Bad sequence of commands\r\n");
	else if (nResponseType == 550)
		strcpy(buf, "550 No such user\r\n");
	else if (nResponseType == 551)
		strcpy(buf, "551 User not local. Can not forward the mail\r\n");
	else
		sprintf(buf, "%d No description\r\n", nResponseType);
	
	len = (int)strlen(buf);
	printf("Sending: %s", buf);
	send(m_socConnection, buf, len, 0);
	return nResponseType;
}

int CMailSession::ProcessDATAEnd(void) {
	m_nStatus = SMTP_STATUS_HELO;
	CloseHandle(m_hMsgFile);	
	char msg_file_name[300];

	for (int i = 0; i < m_nRcptCount; i++)	{
		for (int j = 0;; j++) {
			sprintf(msg_file_name, "%s\\mbox\\%s%d%s", m_ToAddress[i].GetMBoxPath(), NAME_EMAIL, j, MSG_SEARCH_WILDCARD);
			if (!PathFileExists(msg_file_name)) 
				break;
		}
		if (!CopyFile(m_szFileName, msg_file_name, TRUE))
			TRACE("Can not copy to %s\n", msg_file_name);
	}

	FILE *f;
	if ((f = fopen(msg_file_name, "r")) == NULL) {
		printf("\n No message not send, not open file");		
	}
	char ch;
	int k = 0;
	while ((ch = getc(f)) != EOF) {
		switch (ch) {	
		case '\'':
			strcat(message, "&apos;");
			k += 6;
			break;
		case '\"':
			strcat(message, "&quot;");
			k += 6;
			break;
		case '\>':
			strcat(message, "&gt;");
			k += 4;
			break;
		case '\<':
			strcat(message, "&lt;");
			k += 4;
			break;
		case '\&':
			strcat(message, "&amp;");
			k += 4;
			break;
		default:
			message[k] = ch;
			k++;
			break;
		}
	}
	message[k] = '\0';
	fclose(f);

	DeleteFile(m_szFileName);
	strcpy(m_szFileName,"");
	return SendResponse(250);
}

bool CMailSession::CreateNewMessage(void) {
	for (int i = 0;; i++) {
		sprintf(m_szFileName, "%s%d%s", NAME_EMAIL, i, MSG_SEARCH_WILDCARD);
		if (!PathFileExists(m_szFileName))
			break;
	}

	m_hMsgFile = CreateFile(m_szFileName,
		GENERIC_WRITE, FILE_SHARE_READ, 
		NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	return (m_hMsgFile != NULL);
}