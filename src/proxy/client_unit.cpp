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
#include "watchdog.h"

#include "clib_log.h"
extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;

#include "json.h"

using namespace comm::sockcommu;

#define MAX_ERR_RSP_MSG_LEN 4096

extern CMemPool* _webMp;
extern Watchdog* LogFile;

HTTP_SVR_NS_BEGIN

extern CHelperPool*	_helperpool;

inline int GetAllocFromSid(short sid)
{
	return int(sid/200);
}

static const int RANDOM_COUNT = 100;
static int m_nRandCount = 0;

int GetRand()
{
	if(m_nRandCount > 1000000000)
		m_nRandCount = 0;
	srand((int)time(NULL)+m_nRandCount++);
	return rand()%RANDOM_COUNT;
}


CClientUnit::CClientUnit(CDecoderUnit* decoderunit, int fd): 
CPollerObject (decoderunit->pollerunit(), fd),
_stage (CONN_IDLE),
_decoderunit (decoderunit),
_uid(0),
_login_flag(0),
_decodeStatus(DECODE_WAIT_HEAD),
_api(0),
_send_error_times(0),
_r(*_webMp),
_w(*_webMp)
{

}

CClientUnit::~CClientUnit (void)
{
	_w.skip(_w.data_len());
	_r.skip(_r.data_len());
}

int CClientUnit::Attach (void)
{
    EnableInput ();
    
	if (AttachPoller() == -1) // ���� netfd epoll
	{
        log_error ("invoke CPollerObject::AttachPoller() failed.");
		return -1;
	}
	_stage = CONN_IDLE;
    AttachTimer(_decoderunit->get_web_timer());
    return 0;
}

int CClientUnit::recv (void) // InputNotify
{    
    int ret = proc_pkg();
    g_pDebugLog->logMsg("recv ret [%d]", ret);
    switch (ret)
    {
        default:
        case DECODE_FATAL_ERROR:
            DisableInput ();
            g_pDebugLog->logMsg ("decode fatal error netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_FATAL_ERROR;
            return -1;

        case DECODE_DATA_ERROR:
            DisableInput ();
            g_pDebugLog->logMsg ("decode error, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_DATA_ERROR;
            break;
		case DECODE_WAIT_HEAD:
		case DECODE_WAIT_CONTENT:
			_stage = CONN_DATA_RECVING;
			g_pDebugLog->logMsg ("recving data, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
			break; 
        case DECODE_DONE:
            _stage = CONN_RECV_DONE;
            g_pDebugLog->logMsg ("decode done, netfd[%d], stage[%d]", netfd, _stage);
            break;     

        case DECODE_DISCONNECT_BY_USER:
            DisableInput ();
            g_pDebugLog->logMsg ("disconnect by user, netfd[%d] stage[%d] msg[%s]", netfd, _stage, strerror(errno));
            _stage = CONN_DISCONNECT;
            break;
    }

	return 0;
}

int CClientUnit::send (void)
{
    log_error("CClientUnit response bufLen:[%d] netfd[%d]", _w.data_len(), netfd);	
	if (_w.data_len() != 0)
	{		
		log_error("send packet before, length[%d]", _w.data_len());
		int ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		log_error("sent packet length[%d] netfd[%d]", ret, netfd);
		if(-1 == ret)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
			{
				log_error("errno:[%d]", errno);
				//¼ÓÈësend´íÎó´ÎÊýÏÞÖÆ
				_send_error_times++;
				if(_send_error_times >= 50)
				{
					g_pErrorLog->logMsg("%s|CClientUnit::send, send error more,%d|%d", __FUNCTION__, _uid, _api);
					DisableInput ();
					DisableOutput ();
					ApplyEvents ();
					_w.skip( _w.data_len() );
					_r.skip( _r.data_len() );
					_stage = CONN_FATAL_ERROR;
					return -1;
				}	
				EnableOutput ();
				ApplyEvents ();
				_stage = CONN_DATA_SENDING;
				return CONN_DATA_SENDING;
			}
		
			log_error ("sending package to client failed, ret[%d], errno[%d]",  ret, errno);	
			DisableInput ();
			DisableOutput ();
			ApplyEvents ();
			_w.skip( _w.data_len() );
			_r.skip( _r.data_len() );
			_stage = CONN_FATAL_ERROR;
			return -1;
		}

		if(ret == (int)_w.data_len())
		{
			log_debug("send complete");
			DisableOutput();
			ApplyEvents ();
			_w.skip(ret);	
			_stage = CONN_SEND_DONE;
			_send_error_times = 0;
			return ret;
		}
		else if (ret < (int)_w.data_len())
		{
			log_debug("had sent part of data");
			EnableOutput ();
			ApplyEvents ();
			_w.skip(ret);
			_stage = CONN_DATA_SENDING;
			_send_error_times = 0;
			return ret;
		}
	}

	DisableOutput();
	ApplyEvents ();	
	_stage = CONN_FATAL_ERROR;
	log_error("send process failure");
	return -1;
}

int CClientUnit::InputNotify (void) // ���¼�����
{
    update_timer();
    recv();
     g_pDebugLog->logMsg("InputNotify");
    return _decoderunit->web_input ();
}

int CClientUnit::OutputNotify (void)
{
    update_timer();
    send();
    return _decoderunit->web_output();
}

int CClientUnit::HangupNotify (void)
{
    update_timer();
    log_error("*STEP: web connection is hangup, netfd[%d] stage[%d]", netfd, _stage);
    return _decoderunit->web_hangup();
}

void CClientUnit::TimerNotify(void)
{
    log_warning("*STEP: web client timer expired, fd[%d] timeout, stage[%d]", netfd, _stage);
	_decoderunit->web_timer();
    return;
}

void CClientUnit::update_timer(void)
{
    DisableTimer();	
    AttachTimer(_decoderunit->get_web_timer());
}

void CClientUnit::add_rsp_buf(const char* data, unsigned int len)
{
	_w.append(data, len);
}

TDecodeStatus CClientUnit::proc_pkg () // recv
{
	int     curr_recv_len   = 0;
	char    curr_recv_buf[MAX_WEB_RECV_LEN] = {'\0'};

	curr_recv_len = ::recv (netfd, curr_recv_buf, MAX_WEB_RECV_LEN, 0);
	g_pDebugLog->logMsg("*STEP: receiving HTTP request, length[%d]", curr_recv_len);
	if(-1 == curr_recv_len)
	{
		if(errno != EAGAIN && errno != EINTR && errno != EINPROGRESS)
		{
			_decodeStatus = DECODE_FATAL_ERROR;
			log_warning ("recv failed from fd[%d], msg[%s]", netfd, strerror(errno));
			return _decodeStatus;
		}
		else
		{
			return _decodeStatus;
		}
	}

	if(0 == curr_recv_len)
	{
		_decodeStatus = DECODE_DISCONNECT_BY_USER;
		g_pDebugLog->logMsg("%s||Connect disconnect by client, uid:[%d]", __FUNCTION__, _uid);
		return _decodeStatus;
	}

	if(curr_recv_len==23 && curr_recv_buf[0]=='<' && curr_recv_buf[1]=='p')		
	{	
		_decoderunit->set_conn_type(CONN_OTHER);
		std::string policy = "<policy-file-request/>";			
		for(int i=0; i<23; ++i)			
		{				
			if(curr_recv_buf[i] != policy[i])				
			{					
				_decodeStatus = DECODE_FATAL_ERROR;	
				return _decodeStatus;
			}
		}
		std::string resPolicy ="<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>\0";
		_w.append(resPolicy.c_str(), resPolicy.size());
		send();	
		_decodeStatus = DECODE_FATAL_ERROR;					
		return _decodeStatus;
	}
	g_pDebugLog->logMsg("AAAAAAAAAAAAA");
	_r.append(curr_recv_buf, curr_recv_len);
	g_pDebugLog->logMsg("BBBBBBBBBBBBB");
	while(_r.data_len() > 0)
	{
		g_pDebugLog->logMsg("CCCCCCCCCCCC");
		int inputRet = HandleInput(_r.data(), _r.data_len());
		g_pDebugLog->logMsg("DDDDDDDDDDDDDD  [%d]",inputRet);
		if(inputRet < 0)
		{
			_decodeStatus = DECODE_FATAL_ERROR;
			return _decodeStatus;
		}
		else if(inputRet == 0)
		{
			_decodeStatus = DECODE_WAIT_CONTENT;
			return _decodeStatus;
		}
		
		int handleInputBufRet = HandleInputBuf(_r.data() , inputRet);   
		g_pDebugLog->logMsg("eeeeeeeeeee  [%d]",handleInputBufRet);         
		if( handleInputBufRet < 0 )           
		{ 
			_decodeStatus = DECODE_FATAL_ERROR;                 
			return _decodeStatus;            
		}              
		_r.skip(inputRet);
	}
	_decodeStatus = DECODE_WAIT_HEAD;
	return _decodeStatus;
}

int CClientUnit::HandleInput(const char* data,  int len) // ����> 0 ��ʾУ��ɹ�
{
	int headLen = sizeof(struct TPkgHeader);
	g_pDebugLog->logMsg("ffffffffffff  [%d]",headLen);  
	g_pDebugLog->logMsg("ggggggggg  [%d]",len);
	if(len < headLen)
	{
		g_pDebugLog->logMsg("hhhhhhh  [%d]",len);
		return 0;
	}
	
	TPkgHeader *pHeader = (struct TPkgHeader*)data; // �����ж����ݰ��ĺϷ���
	if(pHeader->flag[0]!='B' || pHeader->flag[1]!='Y')
	{
		g_pErrorLog->logMsg("%s||Invalid packet, uid:[%d]", __FUNCTION__, _uid);
		return -1;
	}
	
	int pkglen = sizeof(short) + ntohs(pHeader->length);// ��ȡ��ǰ���ݰ��ĳ���
	g_pErrorLog->logMsg("client packet body length:[%d], cmd:[%d]", ntohs(pHeader->length), ntohs(pHeader->cmd));
	if(pkglen<0 || pkglen>8*1000)
	{
		g_pErrorLog->logMsg("%s||Invalid packet, uid:[%d], pkglen:[%d]", __FUNCTION__, _uid, pkglen);
		return -1;
	}

	if(len < pkglen)
	{
		return 0;
	}
	return pkglen;
}


bool CClientUnit::CheckCmd(int cmd)
{
	for(int i=0; i<(int)_helperpool->m_cmdlist.size(); i++)
	{
		if(cmd == _helperpool->m_cmdlist[i])
			return true;
	}
	return false;
}

int CClientUnit::HandleInputBuf(const char *pData, int len)
{
	NETInputPacket reqPacket;
	reqPacket.Copy(pData, len);
	int cmd = reqPacket.GetCmdType();
	g_pErrorLog->logMsg("%s||HandleInputBuf cmd:[0x%x]",__FUNCTION__, cmd);
	if(TGlobal::_debugLogSwitch && _uid>0)
	{
		g_pDebugLog->logMsg("%s|0x%x|%d|%d|", __FUNCTION__, cmd, _uid, _api);
	}
	
	if(cmd == CLIENT_COMMAND_BREAK_TIME)
	{
		NETOutputPacket resPacket;
		resPacket.Begin(SERVER_COMMAND_BREAK_TIME);
		resPacket.End();
		add_rsp_buf(resPacket.packet_buf(), resPacket.packet_size());
		g_pErrorLog->logMsg("111111111111111111");
		return send();
	}
	else if(cmd == SERVER_GET_PLAYER_COUNT)				///»ñÈ¡serverÈËÊý
	{
		g_pErrorLog->logMsg("333333333333333");
		return ProcGetUserCount(&reqPacket);
	}
	else if(cmd == SERVER_BROADCAST_INFO)//system broadcast
	{
		CEncryptDecrypt encryptdecrypt;
		encryptdecrypt.DecryptBuffer(&reqPacket);
		string key = reqPacket.ReadString();
		if(key != TGlobal::_key)
		{
			g_pErrorLog->logMsg("%s||Invalid key, key:[%s], TGlobalKey:[%s]", __FUNCTION__, key.c_str(), TGlobal::_key.c_str());
			return -1;
		}

		short type = reqPacket.ReadShort();
		string info = reqPacket.ReadString();

		NETOutputPacket resPacket;
		resPacket.Begin(SERVER_BROADCAST_INFO_NEW);
		resPacket.WriteShort(type);
		resPacket.WriteString(info.c_str());
		//g_pErrorLog->logMsg("type[%d]info[%s]",type,info.c_str());
		resPacket.End();
		encryptdecrypt.EncryptBuffer(&resPacket);
		
		map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.begin();
		for(; iter!=_helperpool->m_objmap.end(); iter++)		
		{
			CDecoderUnit* pDecoderUnit = iter->second;
			if(pDecoderUnit != NULL)
			{
				CClientUnit* pClientUnit = pDecoderUnit->get_web_unit();
				if(pClientUnit != NULL)
				{
					if(pClientUnit->get_state() != CONN_FATAL_ERROR)
					{
						pClientUnit->add_rsp_buf(resPacket.packet_buf(), resPacket.packet_size());
						pClientUnit->send();
					}
				}
			}
		}
		return 0;
	}
	else if( cmd == CLINET_REQUEST_BROADCAST_INFO )
	{
		CEncryptDecrypt encryptdecrypt;
		encryptdecrypt.DecryptBuffer(&reqPacket);
		string info = reqPacket.ReadString();

		NETOutputPacket resPacket;
		resPacket.Begin(CLINET_REQUEST_BROADCAST_INFO);
		resPacket.WriteString(info.c_str());
		//g_pErrorLog->logMsg("info[%s]",info.c_str());
		resPacket.End();
		encryptdecrypt.EncryptBuffer(&resPacket);
		
		map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.begin();
		for(; iter!=_helperpool->m_objmap.end(); iter++)		
		{
			CDecoderUnit* pDecoderUnit = iter->second;
			if(pDecoderUnit != NULL)
			{
				CClientUnit* pClientUnit = pDecoderUnit->get_web_unit();
				if(pClientUnit != NULL)
				{
					if(pClientUnit->get_state() != CONN_FATAL_ERROR)
					{
						//g_pErrorLog->logMsg(" send info[%s]",info.c_str());
						pClientUnit->add_rsp_buf(resPacket.packet_buf(), resPacket.packet_size());
						pClientUnit->send();
					}
				}
			}
		}
		return 0;
	}

	if(cmd > CMD_SEPARATE)//游戏server命令字
	{
		g_pErrorLog->logMsg("44444444444");
		if(_uid <= 0)
		{
			g_pErrorLog->logMsg("%s||Invalid user active, cmd:[0x%x], api:[%d]",
				__FUNCTION__, cmd, _api);//未登陆直接发包，此处走不通
			return -1;
		}	
		CGameUnit *pGameUnit = _decoderunit->get_game_unit();
		if(pGameUnit != NULL)
		{
			if(pGameUnit->tid==0 && TGlobal::InWhiteList(_helperpool->m_whitelist, cmd))
			{
				g_pErrorLog->logMsg("%s||Invalid GameServer commond, cmd:[0x%x], uid:[%d], api:[%d], ip:[%s], port:[%d]", 
					__FUNCTION__, cmd, _uid, _api, pGameUnit->addr.c_str(), pGameUnit->port);
				return 0;
			}
			
			pGameUnit->append_pkg(pData, len);
			SendIPSetPacket(pGameUnit, reqPacket, cmd);
				
			if(pGameUnit->send_to_logic(_decoderunit->get_helper_timer()) < 0)
			{ 
				g_pErrorLog->logMsg("%s||Send to GameServer failed, cmd:[0x%x], uid:[%d], api:[%d], ip:[%s], port:[%d]", 
					__FUNCTION__, cmd, _uid, _api, pGameUnit->addr.c_str(), pGameUnit->port);
				return -1;
			}
		}
		else
		{
			g_pErrorLog->logMsg("%s||pGameUnit==NULL, cmd:[0x%x], uid:[%d], api:[%d]", 
				__FUNCTION__, cmd, _uid, _api);
			return -1;
		}
		g_pErrorLog->logMsg("5555555555555555");

		return 0;
	}
	else
	{
		if(cmd == SYS_RELOAD_CONFIG)
		{
			string strFlag = reqPacket.ReadString();
			if(strFlag != "!@#$%^&*()")//¼òµ¥ÑéÖ¤ÏÂ
				return -1;
			if(ResetHelperUnit() < 0)
			{
				log_error("HandleInputBuf||Reload config failed, svid:[%d]", TGlobal::_svid);
			}
			else
			{
				log_error("HandleInputBuf||Reload config success, svid:[%d]", TGlobal::_svid);
			}
		g_pErrorLog->logMsg("666666666666666");

			return 0;
		}
		else if(cmd == SYS_RELOAD_IP_MAP)
		{
			string strFlag = reqPacket.ReadString();
			if(strFlag != "!@#$%^&*()")//¼òµ¥ÑéÖ¤ÏÂ
				return -1;
			if(ResetIpMap() < 0)
			{
				log_error("HandleInputBuf||Reload ip map failed, svid:[%d]", TGlobal::_svid);
			}
			else
			{
				log_error("HandleInputBuf||Reload ip map success, svid:[%d]", TGlobal::_svid);
			}
		g_pErrorLog->logMsg("7777777777777");

			return 0;
		}
		else if(cmd == SYS_RESET_CONNECT_TIMER)
		{
			string strFlag = reqPacket.ReadString();			
			if(strFlag != "!@#$%^&*()")//¼òµ¥ÑéÖ¤ÏÂ
				return -1;
			int timeout = reqPacket.ReadInt();
			TGlobal::_timerConnect = timeout;
			log_error("HandleInputBuf||Reset Connect timer success, svid:[%d], timer[%d]", TGlobal::_svid, TGlobal::_timerConnect);
			return 0;	
		}
		else if(cmd == SYS_OPEN_DEBUG)
		{
		g_pErrorLog->logMsg("88888888888");

			return ProcessOpenDebug(&reqPacket);
		}
		
		if(!CheckCmd(cmd))
		{
		g_pErrorLog->logMsg("99999999999999");

			return 0;
		}

		CHelperUnit *pHelperUnit = NULL;
		if(cmd == CMD_REQUIRE_IP_PORT)//ÓÃ»§ÇëÇóÄ³¸öserverµÄipºÍ¶Ë¿Ú
		{
			if(_uid <= 0)
			{
				g_pErrorLog->logMsg("121212");
				return -1;
			}
			
			CEncryptDecrypt encryptdecrypt;
			encryptdecrypt.DecryptBuffer(&reqPacket);
			short serverId = reqPacket.ReadShort();
			int svid = GetAllocFromSid(serverId);
			map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
			if(iterHelper != _helperpool->m_helpermap.end())
			{
				pHelperUnit = iterHelper->second;
			}
			else
			{
				g_pErrorLog->logMsg("%s||Can not find helper, cmd:[0x%x], uid:[%d], api:[%d], svid:[%d]", 
					__FUNCTION__, cmd, _uid, _api, svid); //ÕÒ²»µ½alloc helper
				return -1;
			}	
		}
		else if(cmd == CMD_GET_NEW_ROOM)//È¥Ä³¸öserverÖÐ»ñÈ¡×À×Ó
		{
			g_pErrorLog->logMsg("00000021");
			return ProcUserGetNewRoom(&reqPacket);
		}
		else if(cmd==CLIENT_CMD_REQUEST_LIST_ROOM  )//Ë½ÈË·¿
		{
			if(_uid <= 0)
			{
				g_pErrorLog->logMsg("131414");
				return -1;
			}
			
			CEncryptDecrypt encryptdecrypt;
			encryptdecrypt.DecryptBuffer(&reqPacket);
			int nLevel = reqPacket.ReadInt();

			g_pErrorLog->logMsg("%s||qequest room list cmd:[0x%x], uid:[%d], level:[%d]", __FUNCTION__, cmd, _uid, nLevel);

			map<int, vector<int> >::iterator iter = _helperpool->m_levelmap.find(nLevel);//Ë½ÈË·¿µÈ¼¶0
			if(iter != _helperpool->m_levelmap.end())
			{
				vector<int>& v = iter->second;
				if((int)v.size() > 0)
				{
					int svid = TGlobal::RandomSvid(v);
					map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
					if(iterHelper != _helperpool->m_helpermap.end())
					{
						pHelperUnit = iterHelper->second;
					}
					else
					{
						g_pErrorLog->logMsg("%s||Can not find helper, cmd:[0x%x], uid:[%d], api:[%d], svid:[%d]", 
							__FUNCTION__, cmd, _uid, _api, svid); //找不到alloc helper
						return -1;
					}
				}
			}
			else
			{
				g_pErrorLog->logMsg("%s||Can not find level, cmd:[0x%x], uid:[%d], api:[%d], level:[%d]", 
					__FUNCTION__, cmd, _uid, _api, nLevel); //找不到alloc helper
				return -1;
			}
		}
		else if ( cmd==CLIENT_CMD_ENTER_ROOM )
		{
			if(_uid <= 0)
			{
				g_pErrorLog->logMsg("6768686");
				return -1;
			}
			
			CEncryptDecrypt encryptdecrypt;
			encryptdecrypt.DecryptBuffer(&reqPacket);
			int nLevel = reqPacket.ReadInt();
			int ntid = reqPacket.ReadInt();

			g_pErrorLog->logMsg(" tid [%d] room list cmd:[0x%x], uid:[%d], level:[%d]", ntid, cmd, _uid, nLevel);

			map<int, vector<int> >::iterator iter = _helperpool->m_levelmap.find(nLevel);//Ë½ÈË·¿µÈ¼¶0
			if(iter != _helperpool->m_levelmap.end())
			{
				vector<int>& v = iter->second;
				if((int)v.size() > 0)
				{
					int svid = TGlobal::RandomSvid(v);
					map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
					if(iterHelper != _helperpool->m_helpermap.end())
					{
						pHelperUnit = iterHelper->second;
					}
					else
					{
						g_pErrorLog->logMsg("%s||Can not find helper, cmd:[0x%x], uid:[%d], api:[%d], svid:[%d]", 
							__FUNCTION__, cmd, _uid, _api, svid); //找不到alloc helper
						return -1;
					}
				}
			}
			else
			{
				g_pErrorLog->logMsg("%s||Can not find level, cmd:[0x%x], uid:[%d], api:[%d], level:[%d]", 
					__FUNCTION__, cmd, _uid, _api, 0); //找不到alloc helper
				return -1;
			}
		}
		else if( cmd == CMD_GET_ROOM_LEVER_NUM )
		{
			return ProcessGetLevelCount(&reqPacket);
		}		
		else 
		{
			if(cmd == CLIENT_CMD_CLIENT_LOGIN)
			{
				CEncryptDecrypt encryptdecrypt;
				encryptdecrypt.DecryptBuffer(&reqPacket);
				int uid = reqPacket.ReadInt();
				short api = reqPacket.ReadShort();

				g_pErrorLog->logMsg("%s||HandleInputBuf uid:[%d], api[%d]",__FUNCTION__, uid, api);

				if(uid <= 0)//×öuid³õ²½ÅÐ¶Ï
				{
					struct in_addr addr;
					addr.s_addr = _decoderunit->get_ip();
					string ip = inet_ntoa(addr);
					g_pErrorLog->logMsg("%s||Invalid Client, cmd:[0x%x], uid:[%d], ip:[%s], api:[%hd]", __FUNCTION__, cmd, uid, ip.c_str(),api);
					return -1;
				}

				/* �����ж��Ƿ��ظ���¼ ��������ҵ���Ӧ��clientunit˵���Ѿ���¼  */
				map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.find(uid);
				if(iter != _helperpool->m_objmap.end())
				{
					CDecoderUnit *pDecoderUnit = iter->second;
					if(pDecoderUnit!=NULL && pDecoderUnit!=_decoderunit)//增加一条连接处理连续2个登陆包
					{
						NETOutputPacket resPacket;
						resPacket.Begin(SERVER_CMD_KICK_OUT);
						resPacket.End();
						encryptdecrypt.EncryptBuffer(&resPacket);
						CClientUnit *pClientUnit = pDecoderUnit->get_web_unit(); /* ���������clientunit */
						if(pClientUnit != NULL)
						{
							pClientUnit->add_rsp_buf(resPacket.packet_buf(), resPacket.packet_size());
							pClientUnit->send();
							pDecoderUnit->set_conn_type(CONN_OTHER);
							pDecoderUnit->complete();
							
							struct in_addr addr1;
							addr1.s_addr = _decoderunit->get_ip();
							string ip1 = inet_ntoa(addr1);
							
							struct in_addr addr2;
							addr2.s_addr = pDecoderUnit->get_ip();
							string ip2 = inet_ntoa(addr2);
							
							g_pErrorLog->logMsg("%s||Kick the same user, cmd:[0x%x], uid:[%d], api:[%d], [%s-%s]", 
								__FUNCTION__, cmd, uid, pClientUnit->_api, ip1.c_str(), ip2.c_str());
						}
						else
						{
							g_pErrorLog->logMsg("%s||pClientUnit==NULL, cmd:[0x%x], uid:[%d], api:[%d]", 
								__FUNCTION__, cmd, uid, api);
						}
					}
				}

				if(_uid>0 && _uid!=uid)//已经登陆过了,并且uid不同
				{
					g_pErrorLog->logMsg("%s||Already Login, uid:[%d], _uid:[%d], api:[%hd]", 
						__FUNCTION__, uid, _uid, _api);
					return -1;
				}
				_decoderunit->set_flag(uid);
				_decoderunit->set_conn_type(CONN_CLINET);
				_uid = uid;
				_api = api;
				_helperpool->m_objmap[uid] = _decoderunit; // ���������� uid ��unit�Ĺ���

				if(TGlobal::_debugLogSwitch && _uid>0)
				{
					g_pDebugLog->logMsg("%s|0x%x|%d|%d", __FUNCTION__, cmd, _uid, _api);
				}
			}
			else
			{
				if(_uid <= 0)
				{
					g_pErrorLog->logMsg("%s||Invalid user active, cmd:[0x%x], api:[%d]", 
						__FUNCTION__, cmd, _api);//未登陆直接发包，此处走不通
					return -1;
				}
			}
			
			if((int)_helperpool->m_svidlist.size() > 0)
			{
				int svid = TGlobal::LoopSvid(_helperpool->m_svidlist);
				map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
				if(iterHelper != _helperpool->m_helpermap.end())
				{
					pHelperUnit = iterHelper->second;
				}
				else
				{
					g_pErrorLog->logMsg("%s||Can not find helper, cmd:[0x%x], uid:[%d], api:[%d], svid:[%d]",
						__FUNCTION__, cmd, _uid, _api, svid); 
					return -1;
				}
			}
		}
		g_pErrorLog->logMsg("QQQQQQQQQQQQQQQQ");
		if(pHelperUnit != NULL)
		{
			NETOutputPacket transPacket;
			transPacket.Begin(CLIENT_PACKET2);
			transPacket.WriteInt(_uid);
			transPacket.WriteInt(TGlobal::_svid);//标志哪个server发的
			transPacket.WriteInt(_decoderunit->get_ip());
			transPacket.WriteShort(_api);
			transPacket.WriteBinary(pData, len);
			transPacket.End();
			pHelperUnit->append_pkg(transPacket.packet_buf(), transPacket.packet_size());
			if(pHelperUnit->send_to_logic(_decoderunit->get_helper_timer()) < 0)
			{
				g_pErrorLog->logMsg("%s||Send to AllocServer failed, cmd:[0x%x], uid:[%d] api:[%d]", __FUNCTION__, cmd, _uid,_api); //ÕÒ²»µ½alloc helper
				if(cmd==CLIENT_CMD_REQUEST_LIST_ROOM)//Ë½ÈË·¿ÓÐÎÊÌâÒ²ÈÃÓÃ»§µÇÂ¼³É¹¦
				{
					g_pErrorLog->logMsg("2224678222");
					return 0;
				}
				else
				{
					g_pErrorLog->logMsg("98898");
					return -1;
				}
			}
		}
	}
	g_pErrorLog->logMsg("7523");
	return 0;
}


int CClientUnit::ResetHelperUnit()
{	
	CMarkupSTL  markup;
    if(!markup.Load("../conf/server.xml"))
    {       
        log_error("Load server.xml failed.");
        return -1;
    }

    if(!markup.FindElem("SYSTEM"))
    {
        log_error("Can not FindElem [SYSTEM] in server.xml failed.");
        return -1;
    }

	if (!markup.IntoElem())    
	{        
		log_error ("IntoElem [SYSTEM] failed.");       
		return -1;    
	}
	
	if(markup.FindElem("Node"))
	{
		map<int, vector<int> >::iterator iterLevel = _helperpool->m_levelmap.begin();
		for(; iterLevel!=_helperpool->m_levelmap.end(); iterLevel++)
		{
			vector<int>& v = iterLevel->second;
			v.clear();
		}
		_helperpool->m_levelmap.clear();
		
		_helperpool->m_svidlist.clear();

		map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.begin();
		for(; iterHelper!=_helperpool->m_helpermap.end(); iterHelper++)
		{
			CHelperUnit* pHelperUnit = iterHelper->second;
			if(pHelperUnit != NULL)
			{
				delete pHelperUnit;
				pHelperUnit = NULL;
			}
		}

		_helperpool->m_helpermap.clear();
			
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem failed.");       
			return -1;    
		}

		if(!markup.FindElem("ServerList"))
		{
			log_error ("IntoElem [ServerList] failed.");     
			return -1; 
		}
		
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [ServerList] failed.");       
			return -1;    
		}
		
		while(markup.FindElem("Server"))
		{
			int svid =  atoi(markup.GetAttrib("svid").c_str());
			int level = atoi(markup.GetAttrib("level").c_str());
			string ip = markup.GetAttrib("ip");
			int port = atoi(markup.GetAttrib("port").c_str());
			CHelperUnit *pHelperUnit = NULL;
			map<int, CHelperUnit*>::iterator iter = _helperpool->m_helpermap.find(svid);
			if(iter != _helperpool->m_helpermap.end())
			{
				pHelperUnit = iter->second;
			}
			else
			{
				pHelperUnit = new CHelperUnit(_decoderunit->pollerunit());
				if(pHelperUnit == NULL)
				{
					log_error("New CHelpUnit error");
					return -1;
				}	
				_helperpool->m_helpermap[svid] = pHelperUnit;
			}
			 
			if(pHelperUnit == NULL)
			{
				log_boot("pHelperUnit NULL");
				return -1;
			}	


			_helperpool->m_svidlist.push_back(svid);

			pHelperUnit->addr = ip;
			pHelperUnit->port = port;

			vector<int>& v = _helperpool->m_levelmap[level];
			v.push_back(svid);
			log_error("alloc server id:[%d], level:[%d], ip:[%s], port:[%d]", svid, level, ip.c_str(), port);
		}


		if (!markup.OutOfElem())    
		{        
			log_error ("OutOfElem [CmdList] failed.");       
			return -1;    
		}
	
		if(!markup.FindElem("CmdList"))
		{
			log_error("Can not FindElem [CmdList] in server.xml failed.");
			return -1;
		}
		
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [CmdList] failed.");       
			return -1;    
		}

		_helperpool->m_cmdlist.clear();
		while(markup.FindElem("Cmd"))
		{
			int cmd = atoi(markup.GetAttrib("value").c_str());
			log_error("cmd:[%d]", cmd);
			_helperpool->m_cmdlist.push_back(cmd);
		}
		
		if (!markup.OutOfElem())    
		{        
			log_error ("OutOfElem [ServerList] failed.");       
			return -1;    
		}
	}
	return 0;
}

int CClientUnit::ResetIpMap()
{
	CMarkupSTL  markup;
    if(!markup.Load("../conf/server.xml"))
    {       
        log_error("Load server.xml failed.");
        return -1;
    }

    if(!markup.FindElem("SYSTEM"))
    {
        log_error("Can not FindElem [SYSTEM] in server.xml failed.");
        return -1;
    }

	if (!markup.IntoElem())    
	{        
		log_error ("IntoElem [SYSTEM] failed.");       
		return -1;    
	}
	
	if(markup.FindElem("IPMap"))
	{
		if (!markup.IntoElem())    
		{        
			log_error ("IntoElem [IPMap] failed.");       
			return -1;    
		}

		while(markup.FindElem("IP"))
		{
			string eth0 = markup.GetAttrib("eth0");
			string eth1 = markup.GetAttrib("eth1");
			log_error("%s||eth0:[%s], eth1:[%s]", __FUNCTION__, eth0.c_str(), eth1.c_str());
			_helperpool->m_ipmap[eth0] = eth1;
		}
	}
	return 0;
}


int CClientUnit::ProcessGetLevelCount(NETInputPacket* pPacket)
{
	CEncryptDecrypt	encryptDecrypt;
	encryptDecrypt.DecryptBuffer(pPacket);
	short levelCount = pPacket->ReadShort();
	NETOutputPacket resPacket;
	resPacket.Begin(CMD_GET_ROOM_LEVER_NUM);
	resPacket.WriteShort(levelCount);
	for(int i=0; i<levelCount; i++)
	{
		int randCount = GetRand();
		short level = pPacket->ReadShort();
		resPacket.WriteShort(level);
		map<short, int>::iterator iter = _helperpool->m_LevelCountMap.find(level);
		if(iter != _helperpool->m_LevelCountMap.end())
		{
			int& userCount = iter->second;
			if(userCount > 32000)
			{
				userCount = 32000;
			}
			resPacket.WriteShort(userCount+randCount);
		}
		else
		{
			resPacket.WriteShort(randCount);
		}
		
	}
	resPacket.End();
	encryptDecrypt.EncryptBuffer(&resPacket);
	add_rsp_buf(resPacket.packet_buf(), resPacket.packet_size());
	return send();
}

CHelperUnit* CClientUnit::GetRandomHelper(short level)
{
	CHelperUnit* pHelperUnit = NULL;
	map<int, vector<int> >::iterator iter = _helperpool->m_levelmap.find(level);
	if(iter != _helperpool->m_levelmap.end())
	{
		vector<int>& v = iter->second;
		if((int)v.size() > 0)
		{
			int svid = TGlobal::RandomSvid(v);
			map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
			if(iterHelper != _helperpool->m_helpermap.end())
			{
				pHelperUnit = iterHelper->second;	
			}
			else
			{
				g_pErrorLog->logMsg("%s||Can not find helper, uid:[%d], api:[%d], svid:[%d]", 
					__FUNCTION__, _uid, _api, svid); //找不到alloc helper
			}
		}
	}
	return pHelperUnit;
}

int CClientUnit::SendPacketToHelperUnit(CHelperUnit* pHelperUnit, char *pData, int nSize)
{
	NETOutputPacket transPacket;
	transPacket.Begin(CLIENT_PACKET2);
	transPacket.WriteInt(_uid);
	transPacket.WriteInt(TGlobal::_svid);
	transPacket.WriteInt(_decoderunit->get_ip());
	transPacket.WriteShort(_api);
	transPacket.WriteBinary(pData, nSize);
	transPacket.End();

	pHelperUnit->append_pkg(transPacket.packet_buf(), transPacket.packet_size());
	if(pHelperUnit->send_to_logic(_decoderunit->get_helper_timer()) < 0)
	{
		g_pErrorLog->logMsg("%s||Send to AllocServer failed, ip:[%s], port:[%d], uid:[%d], api:[%d]",
			__FUNCTION__, pHelperUnit->addr.c_str(), pHelperUnit->port, _uid, _api);
		return -1;
	}
	return 0;
}

int CClientUnit::ProcUserGetNewRoom(NETInputPacket * pPacket)
{
	if(_uid <= 0)
	{
		g_pErrorLog->logMsg("%s||Invalid user active, api:[%d]", __FUNCTION__, _api);//未登陆直接发包，此处走不通
		return -1;
	}
	CGameUnit* pGameUnit = _decoderunit->get_game_unit();
	if( NULL == pGameUnit)
	{
		g_pErrorLog->logMsg("%s||GameUnit is NULL",__FUNCTION__);
		return -1;
	}
	if( pGameUnit->tid >0)
	{
		return ProcUserGetNewRoom1(pGameUnit);	
	}
	else if( 0 == pGameUnit->tid)
	{
		return ProcUserGetNewRoom2(pPacket);	
	}
	return 0;
}

int CClientUnit::ProcUserGetNewRoom1(CGameUnit * pGameUnit)
{
	if(NULL == pGameUnit)
	{
		return -1;
	}
	NETOutputPacket	outPkg;
	outPkg.Begin(CMD_GET_NEW_ROOM);
	outPkg.WriteInt(pGameUnit->tid);
	outPkg.WriteString(pGameUnit->addr.c_str());
	outPkg.WriteInt(pGameUnit->port);
	outPkg.End();

	if(TGlobal::_debugLogSwitch && _uid>0)
	{
		g_pDebugLog->logMsg("%s|0x%x|%d|", __FUNCTION__, CMD_GET_NEW_ROOM, _uid);
	}
	
	CEncryptDecrypt	encryptDecrypt;
	encryptDecrypt.EncryptBuffer(&outPkg);
	add_rsp_buf(outPkg.packet_buf(), outPkg.packet_size());
	return send();
}

int CClientUnit::ProcUserGetNewRoom2(NETInputPacket* pPacket)
{
	CEncryptDecrypt encryptdecrypt;
	encryptdecrypt.DecryptBuffer(pPacket);
	short level = pPacket->ReadShort();
	CHelperUnit* pHelperUnit = GetRandomHelper(level);
	if(NULL == pHelperUnit)
	{
		g_pErrorLog->logMsg("%s||pHelperUnit==NULL, uid:[%d], api:[%d], level:[%d]", __FUNCTION__, _uid, _api, level); //找不到alloc helper
		return -1;	
	}
	NETOutputPacket tempPacket;
	tempPacket.Copy(pPacket->packet_buf(), pPacket->packet_size()); 
	encryptdecrypt.EncryptBuffer(&tempPacket);
	if(SendPacketToHelperUnit(pHelperUnit, tempPacket.packet_buf(), tempPacket.packet_size()) < 0)
	{
		return -1;
	}
	return 0;

}

int CClientUnit::ProcGetUserCount(NETInputPacket* pPacket)
{
	CEncryptDecrypt encryptdecrypt;
	encryptdecrypt.DecryptBuffer(pPacket);
	string str = pPacket->ReadString();
	if(str != "boyaa")
	{
		return -1;
	}

	NETOutputPacket outpkg;
	outpkg.Begin(SERVER_GET_PLAYER_COUNT);
	
	int allUserCount = _helperpool->m_objmap.size();
	outpkg.WriteInt(allUserCount);
	
	int tempSid = TGlobal::_svid;
	outpkg.WriteInt(tempSid);
	outpkg.End();
	encryptdecrypt.EncryptBuffer(&outpkg);
	if(TGlobal::_debugLogSwitch && _uid>0)
	{
		g_pDebugLog->logMsg("%s|0x%x|%d|%d", __FUNCTION__, SERVER_GET_PLAYER_COUNT, _uid, _api);
	}
	
	add_rsp_buf(outpkg.packet_buf(), outpkg.packet_size());
	return send();		

}

void CClientUnit::SendIPSetPacket(CGameUnit* pGameUnit, NETInputPacket &reqPacket, int cmd)
{
	if(NULL!=_decoderunit && CLIENT_COMMAND_LOGIN==cmd)
	{
		CEncryptDecrypt encryptdecrypt;
		encryptdecrypt.DecryptBuffer(&reqPacket);
		int nTableId = reqPacket.ReadInt();
		int nUserId = reqPacket.ReadInt();

		g_pDebugLog->logMsg("nTableId [%d], nUserId [%d]", nTableId, nUserId);

		NETOutputPacket outPkg;
		outPkg.Begin(SERVER_CMD_SET_IP);
		outPkg.WriteInt(nTableId);
		outPkg.WriteInt(nUserId); 
		outPkg.WriteInt(ntohl(_decoderunit->get_ip())); 
		outPkg.End();
		encryptdecrypt.EncryptBuffer(&outPkg);
 		pGameUnit->append_pkg(outPkg.packet_buf(), outPkg.packet_size());
	}
}	

int CClientUnit::ProcessOpenDebug(NETInputPacket *pPacket)
{	
	CEncryptDecrypt encryptdecrypt;
	encryptdecrypt.DecryptBuffer(pPacket);
	string strFlag = pPacket->ReadString();
	if(strFlag != "!@#$%^&*()")//¼òµ¥ÑéÖ¤ÏÂ
		return -1;
	TGlobal::_debugLogSwitch = pPacket->ReadInt();
	log_error("%s|_debugLogSwitch:[%d]", __FUNCTION__, TGlobal::_debugLogSwitch);
	return 0;
}
HTTP_SVR_NS_END

