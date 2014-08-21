

#include <stdio.h>
#include <sys/un.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <client_unit.h>
#include <decode_unit.h>
#include <memcheck.h>
#include <log.h>
#include <sys/types.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <cache.h>
#include "mempool.h"
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <helper_unit.h>
#include <memcheck.h>
#include <CHelper_pool.h>
#include <sstream>
#include <defs.h>
#include <MarkupSTL.h>
#include "RealTimer.h"
#include "clib_log.h"
extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;
#include "CHelper_pool.h"
#include "watchdog.h"
extern Watchdog *LogFile; 

HTTP_SVR_NS_BEGIN

extern CHelperPool*	_helperpool;

void CRealTimer::TimerNotify(void)
{
	if(TGlobal::_udpSwitch)
		LogFile->writeHallUserCount((int)_helperpool->m_objmap.size(), TGlobal::_ip, TGlobal::_port);
	AttachTimer(realTimerlist);
}

static int RandomUid()
{
	srand((unsigned)time(NULL));
	int value = rand()%10;
	return value;

}


//��User���ͻ�ȡ������ݵ�Э��
void CLevelCountTimer::TimerNotify()
{
	//�����һ��UserId
	g_pDebugLog->logMsg("%s||GetUserLevelCount", __FUNCTION__);
	SendGetLevelCount();
	StartTimer();
}

void CLevelCountTimer::SendGetLevelCount()
{
	int nUid = RandomUid();												//ģ��һ��uid
	int nSvid = TGlobal::RandomSvid(_helperpool->m_svidlist);			//���һ��AllocId

	NETOutputPacket	SubPacket;
	SubPacket.Begin(CMD_GET_ROOM_LEVER_NUM);
	SubPacket.WriteShort(_helperpool->m_levelmap.size());

	map<int, std::vector<int> >::iterator it = _helperpool->m_levelmap.begin();
	for(; it != _helperpool->m_levelmap.end(); it++)
	{
		short level = it->first;
		SubPacket.WriteShort(level);

	}	
	SubPacket.End();

	CEncryptDecrypt	encryptDecrypt;
	encryptDecrypt.EncryptBuffer(&SubPacket);

	NETOutputPacket outPkg;
	outPkg.Begin(CLIENT_PACKET);
	outPkg.WriteInt(nUid);
	outPkg.WriteInt(TGlobal::_svid);
	outPkg.WriteBinary(SubPacket.packet_buf(), SubPacket.packet_size());
	outPkg.End();

	CHelperUnit* pHelperUnit = NULL;
	map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(nSvid);
	if(iterHelper != _helperpool->m_helpermap.end())
	{
		pHelperUnit = iterHelper->second;
	}

	if( NULL != pHelperUnit)
	{
		pHelperUnit->append_pkg(outPkg.packet_buf(), outPkg.packet_size());
		if( pHelperUnit->send_to_logic(m_HelperList) < 0)
		{
			g_pErrorLog->logMsg("%s||Send Packet to get level count fail", __FUNCTION__);	

		}

	}
}


HTTP_SVR_NS_END








