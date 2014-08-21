#ifndef __CLIENT_SYNC_H__
#define __CLIENT_SYNC_H__

#include <poller.h>
#include <timerlist.h>
#include <global.h>
#include <noncopyable.h>
#include <cache.h>
#include <memcheck.h>
#include "ICHAT_PacketBase.h"
#include "EncryptDecrypt.h"

using namespace comm::sockcommu;

HTTP_SVR_NS_BEGIN
#pragma pack(1)
struct TPkgHeader
{	
	 short length;
	 char  flag[2];	 	
	 char  cVersion;	
	 char  cSubVersion;
	 short cmd;
	 unsigned char  code;
};
#pragma pack()


class CDecoderUnit;
class CHelperUnit;
class CIncoming;
class CGameUnit;

class CClientUnit : public CPollerObject, private CTimerObject, private noncopyable
{
public:
	CClientUnit (CDecoderUnit*, int);
	virtual ~CClientUnit ();

    int Attach (void);

    inline void set_state (CConnState stage) 
	{ 
		_stage = stage;
	}
	inline CConnState get_state (void) 
	{ 
		return _stage;
	}

    inline void set_netfd (int fd)
	{ 
		netfd = fd;
	}
	
	inline int get_netfd (void) 
	{ 
		return netfd; 
	}

	inline void set_uid(int id)
	{
		_uid = id;
	}

	inline int get_uid()
	{
		return _uid;
	}

	inline void set_login_flag(int flag)
	{
		_login_flag = flag;
	}

	inline int get_login_flag()
	{
		return _login_flag;
	}

	int ResetIpMap();
	int ResetHelperUnit();
	int recv (void);
	int send (void);
	void add_rsp_buf(const char* data, unsigned int len);
	int HandleInput(const char* data,  int len);
	int HandleInputBuf(const char* data, int len);
	bool CheckCmd(int cmd);

private:
	virtual int InputNotify (void);
	virtual int OutputNotify (void);
    virtual int HangupNotify (void);
    virtual void TimerNotify(void);
    void update_timer(void);
	TDecodeStatus proc_pkg();

	int ProcessGetLevelCount(NETInputPacket *pPacket);

	int ProcUserGetNewRoom(NETInputPacket *pPacket);
	int ProcUserGetNewRoom1(CGameUnit *pGameUnit);
	int ProcUserGetNewRoom2(NETInputPacket *pPacket);
	int ProcGetUserCount(NETInputPacket* pPacket);

	CHelperUnit* GetRandomHelper(short level);

	int SendPacketToHelperUnit(CHelperUnit *pHelperUnit, char *pData, int nSize);

	void SendIPSetPacket(CGameUnit* pGameUnit,NETInputPacket &reqPacket, int cmd);	
	int ProcessOpenDebug(NETInputPacket *pPacket);

public:
	short           _api;
	short           _send_error_times;
private:
    CConnState      _stage;
	TDecodeStatus   _decodeStatus;
    CDecoderUnit*   _decoderunit;
	int			    _uid;
	int             _login_flag;
	CRawCache       _r;
	CRawCache       _w;
};
HTTP_SVR_NS_END
#endif

