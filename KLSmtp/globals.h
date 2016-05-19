#pragma once

#define DOMAIN_NAME g_szDomain
#define DOMAIN_ROOT_PATH g_szDomainPath
#define USER_MAIL_DIRECTORY_NAME "mbox"

#define NAME_EMAIL "email"
#define MSG_SEARCH_WILDCARD ".txt"

//SMTP Defines
#define SMTP_PORT 25

//#define SMTP_DATA_TERMINATOR "\r\n.\r\n"
 #define SMTP_DATA_TERMINATOR ".\n"

#define SMTP_STATUS_INIT 1
#define SMTP_STATUS_HELO 2
#define SMTP_STATUS_DATA 16
#define SMTP_STATUS_DATA_END 32

#define SMTP_CMD_HELO "HELO"
#define SMTP_CMD_EHLO "EHLO"
#define SMTP_CMD_MAIL "MAIL"
#define SMTP_CMD_RCPT "RCPT"
#define SMTP_CMD_DATA "DATA"
#define SMTP_CMD_QUIT "QUIT"

// DATABASE
#define DB_FILE "SMTP.db"

//SMTP (RFC 821) DEFINED VALUES
#define MAX_USER_LENGTH 64 
#define MAX_DOMAIN_LENGTH 64
#define MAX_ADDRESS_LENGTH 256
#define MAX_CMD_LENGTH 512
#define MAX_RCPT_ALLOWED 100