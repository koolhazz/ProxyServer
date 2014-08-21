#ifndef __REDIS_SERVER_H
#define __REDIS_SERVER_H
#include <string>
#include <string.h>
using std::string;
using namespace std;
#include "hiredis.h"

enum
{
	KEYFLAG_ILLEGAL = 0,
	KEYFLAG_LEVEL ,
	KEYFLAG_SCORE ,
	KEYFLAG_LEFTDAYS ,
	KEYFLAG_OUTOFFORCESTAMP,
	KEYFLAG_CURRENTSTARTSTAMP,
	KEYFLAG_HISTORYSTARTSTAMP,
	KEYFLAG_HISTORYDAYS,
	KEYFLAG_PLUSSCORE,
};

enum
{
	REPLY_OK = 0,
	REPLY_NULL,
	REPLY_ELEMENT_NULL,
	REPLY_ELEMENTCOUNT_ERROR,
	REPLY_KEY_ERROR,
	REPLY_KEY_TYPEERROR,
	REPLY_COMMAND_ERROR,
	REPLY_TYPE_ERROR,
};

#define REDIS_CONNECT_TIMER 1

#define RANK_REDIS_EXPIRE  2592000        //����redis 30�����
#define SCORE_REDIS_EXPIRE 10800          //��Ҹ��˻���redis��¼3Сʱ����

class CRedisServer
{
public:
	CRedisServer();
	~CRedisServer();
	int Connect(string& ip, int port);
	int Reconnect();
	int SetExpire(char* key, int expire);
	redisReply * RedisCommand(const char * command);
	int SetTimeout(struct timeval tv);
	int SetInfo(string ip, int port, int sid);
public:
	string m_strIp;
	int m_nPort;
	redisContext *m_pRedisConn;		//����hiredis
	redisReply *m_pRedisReply;		//�ظ�hiredis 

	string HallServerInfo;
};
#endif

