#pragma once

class CMailAddress
{
	char m_szUser[MAX_USER_LENGTH+5];
	char m_szDomain[MAX_DOMAIN_LENGTH+5];

	char m_szAddress[MAX_ADDRESS_LENGTH+2];
	char m_szMBoxPath[300];
	char timesend[20];

public:
	CMailAddress(char szAddress[] = "") {		
		strcpy(m_szAddress, szAddress);
	}
	void SetMBoxPath(char *path) {
		strcpy(m_szMBoxPath, path);
	}

	bool SetAddress(char szAddress[]){
		char *domain = m_szDomain;
		if(!AddressValid(szAddress)) 
			return false;
		strcpy(m_szAddress, szAddress);
		domain = strchr(m_szAddress, '@');
		domain += 1;
		memset(m_szUser, 0, sizeof(m_szUser));
		strncpy(m_szUser, m_szAddress, strlen(m_szAddress) - strlen(domain) - 1);
		time_t timer;
		time(&timer);		
		strcpy(timesend, asctime(localtime(&timer)));
		return true;
	}
	
	char* GetMBoxPath()	{
		return m_szMBoxPath;
	}

	char* GetAddress() {
		return m_szAddress; 
	}

	char* GetDomain() {
		return m_szAddress;
	}

	char* GetUser() {
		return m_szUser;
	}

	char* GetTime() {
		return timesend;
	}

	static bool AddressValid(char *szAddress) {
		return (strlen(szAddress) > 2 && strchr(szAddress, '@'));
	}
};
