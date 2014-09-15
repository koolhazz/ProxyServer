#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <log.h>
#include <helper_unit.h>
#include <net.h>
#include <memcheck.h>
#include <extend_http.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <iostream>
#include <socket.h>
#include <CHelper_pool.h>
#include <sstream>
#include <client_unit.h>
#include "ICHAT_PacketBase.h"
#include "EncryptDecrypt.h"
#include "watchdog.h"
extern Watchdog *LogFile;

#include "clib_log.h"
extern clib_log* g_pDebugLog;
extern clib_log* g_pErrorLog;

HTTP_SVR_NS_BEGIN
extern CHelperPool* _helperpool;

class TPkgHeader;
class CClientUnit; 

CHelperUnit::CHelperUnit(CPollerUnit* unit) : CPollerObject (unit),
		_stage (CONN_IDLE),
		_send_error_times(0),
		_r(*_helpMp),
		_w(*_helpMp)
{
}

CHelperUnit::~CHelperUnit ()
{
	reset_helper();
}


string CHelperUnit::IpMap(string& ip)
{
	string retIp = "";
	map<string, string>::iterator iter = _helperpool->m_ipmap.find(ip);
	if(iter != _helperpool->m_ipmap.end())
	{
		retIp = iter->second;
	}
	return retIp;
}

int CHelperUnit::send_to_logic (CTimerList* lst)
{
	_helperTimer = lst;
	EnableInput (); // 这里关注EPOLLIN,也就是发送后端数据的时候才关注
	if (connect() < 0)
	{
		reset_helper();
		log_debug("Connect logic server failed!");
		return -1;
	}
	AttachTimer(_helperTimer);
	return send_to_cgi();
}

int CHelperUnit::reConnect(void)
{
	int ret = -1;
	log_debug("*STEP: PROXY CNet::reConnect before");
	ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
	log_debug("*STEP: PROXY CNet::reConnect ret :[%d], netfd:[%d]", ret, netfd);
	if (ret < 0)
	{
		if (ret == SOCKET_CREATE_FAILED)
		{
			log_error("*STEP: helper create socket failed, errno[%d], msg[%s]", errno, strerror(errno));
			return -1;
		}
		if(errno != EINPROGRESS)
		{
			log_error("*STEP: PROXY connect to logic failed, errno[%d], msg[%s]",errno , strerror(errno));
			return -1;
		}
		_stage = CONN_CONNECTING;
		log_debug("*STEP: PROXY connecting to logic, unix fd[%d]", netfd);
		goto exit;
	}
	_stage = CONN_CONNECTED;

exit:
	return AttachPoller();
}

int CHelperUnit::connect (void)
{
	int ret = -1;
	log_debug("*STEP: PROXY CNet::tcp_connect before");

	if (_stage == CONN_IDLE)
	{
		ret = CNet::tcp_connect(&netfd, addr.c_str(), port, 0, TGlobal::_timerConnect);
	}
	else
	{
		if (netfd < 0) // 这里判断是否连接存在
		{
			ret = CNet::tcp_connect(&netfd,addr.c_str(), port, 0, TGlobal::_timerConnect);
		}
		else
		{
			ret = 0;
		}
	}

	log_debug("*STEP: PROXY CNet::tcp_connect ret :[%d], netfd:[%d]", ret, netfd);

	if (ret < 0)
	{	
		log_error("Connect Helper error, ip:[%s], port:[%d]", addr.c_str(), port);
		return -1;
	}
	_stage = CONN_CONNECTED;
	
exit:
	return AttachPoller();
}

int CHelperUnit::send_to_cgi(void)
{
	int ret = 0;
	int reConnectCount = 0;
	log_debug("_w data size:[%d]", _w.data_len());
logic:		
	if (_w.data_len() != 0)
	{
		ret = ::send (netfd, _w.data(), _w.data_len(), 0);
		log_debug("sent packet size:[%d]", ret);
		if(-1 == ret)
		{
			if(errno == EINTR || errno == EAGAIN || errno == EINPROGRESS)
			{
				_send_error_times++;
				if(_send_error_times >= 50)
				{
					g_pErrorLog->logMsg("CHelperUnit::send_to_cgi, send error more");
					reset_helper();
					_stage = CONN_FATAL_ERROR;
					return -1;
				}
				EnableOutput ();
				ApplyEvents ();		
				return CONN_DATA_SENDING;
			}
		
			log_error("*STEP: sending package to logic server failed, ret code[%d], errno:[%d], ip:[%s], port:[%d]", ret, errno, addr.c_str(), port);	
			reset_helper();
			_stage = CONN_FATAL_ERROR;
			return -1;
		}

		if(ret == (int)_w.data_len())
		{
			_w.skip(ret);
			DisableOutput ();
			ApplyEvents();
			_stage = CONN_SEND_DONE;
			//重置发送错误次数
			_send_error_times = 0;
			log_debug ("*STEP: sent package to logic server, netfd[%d] packet size[%d]", netfd, ret);
			return ret;
		}
		else if (ret < (int)_w.data_len())
		{
			EnableOutput ();
			ApplyEvents ();	
			_w.skip(ret);
			_stage = CONN_DATA_SENDING;
			//重置发送错误次数
			_send_error_times = 0;
			return ret;
		}
		DisableInput ();
		DisableOutput ();
		ApplyEvents();
		_stage = CONN_FATAL_ERROR;
		log_debug ("*STEP: sending package to logic server in exception, netfd[%d]", netfd);
		return ret;
	}
	return ret;
}


int CHelperUnit::recv_from_cgi(void)
{
	int ret = ::recv (netfd, _curRecvBuf, MAX_HELPER_RECV_BUF, 0);	
	log_error("*STEP: recv from logic server fd[%d] recv len[%d] this time", netfd, ret);	
	if (-1 == ret)
	{
		log_debug("*STEP: ret = -1");		
		if((errno == EINTR) || (errno == EAGAIN) || (errno == EINPROGRESS))
		{
			_stage = CONN_DATA_RECVING;
			return 0;
		}		
		_stage = CONN_FATAL_ERROR;
		log_error ("*STEP: recv from logic server failed, unix fd[%d]", netfd);
		return -1;
	}

	if (0 == ret)
	{
		_stage = CONN_DISCONNECT;
		log_debug ("*STEP:logic server close, unix fd[%d]", netfd);
		return -1;
	}
		
	_r.append(_curRecvBuf, ret);
	if(process_pkg() < 0)
		return -1;
	_stage = CONN_RECV_DONE;
	log_debug("*STEP: recving from logic server complete, unix fd[%d] current recv len[%d]", netfd, ret);
	return ret;
} 

int CHelperUnit::InputNotify (void)
{
	update_timer();
	int ret = recv_from_cgi();
	if (ret < 0)
	{
		log_error("call recv failed, helper client netfd[%d]", netfd); 
		reset_helper();
		return POLLER_COMPLETE;
	}
	return POLLER_SUCC;
}

int CHelperUnit::OutputNotify (void)
{
	update_timer();
	send_to_cgi();
	return POLLER_SUCC;
}

int CHelperUnit::HangupNotify (void)
{
	log_debug("CHelperUnit object hangup notify: fd[%d]", netfd);
	update_timer();
	process_pkg();
	reset_helper();
	return POLLER_COMPLETE;
}

void CHelperUnit::TimerNotify (void)
{
	log_debug ("*STEP: timer expired, unix fd[%d], timeout.", netfd);
	reset_helper();
}

void CHelperUnit::update_timer(void)
{
	DisableTimer();
	AttachTimer(_helperTimer);
}

int CHelperUnit::append_pkg(const char* buf, unsigned int len)
{
	_w.append(buf, len);
	return 0;
}

int CHelperUnit::ProcessUserLoginSuccess(CDecoderUnit* pDecoder, NETInputPacket* pPacket)
{
	g_pDebugLog->logMsg("--------------- CHelperUnit::ProcessUserLoginSuccess being --------------");
	CEncryptDecrypt decrypt;
	decrypt.DecryptBuffer(pPacket);
	int levelCount = pPacket->ReadInt();
	g_pDebugLog->logMsg("levelCount %d",levelCount);
	for(int i=0; i<levelCount; i++)
	{
		int level = pPacket->ReadInt();
		g_pDebugLog->logMsg("level %d",level);
	}
	int tid = pPacket->ReadInt();
	g_pDebugLog->logMsg("tid %d",tid);
	if(tid > 0)
	{
		string ip = pPacket->ReadString();
		int port = pPacket->ReadInt();
		CGameUnit *pGameUnit = pDecoder->get_game_unit();
		if(pGameUnit != NULL) /* 这里记录allocserver 返回的GameServer的地址? 应该是Alloc的地址? */
		{
			pGameUnit->addr = IpMap(ip);
			pGameUnit->port = port;
			pGameUnit->tid = tid;	
		}

		g_pErrorLog->logMsg(" ProcessUserLoginSuccess tid:[%d], ip:[%s], port:[%d]", 
			tid, pGameUnit->addr.c_str(), pGameUnit->port);
	}
	
	g_pDebugLog->logMsg("--------------- CHelperUnit::ProcessUserLoginSuccess end --------------");
	return 0;
}

int CHelperUnit::ProcessEnterRoom(CDecoderUnit* pDecoder, NETInputPacket* pPacket)
{
	g_pErrorLog->logMsg("--------- CHelperUnit::ProcessEnterRoom begin  ---------------");
	CEncryptDecrypt decrypt;
	decrypt.DecryptBuffer(pPacket);
	int res = pPacket->ReadShort();

	g_pErrorLog->logMsg("res : %d",res);

	if(res)
	{
		string ip = pPacket->ReadString();
		int port = pPacket->ReadInt();
		int tid = pPacket->ReadInt();
		g_pErrorLog->logMsg(" ProcessEnterRoom tid:[%d], ip:[%s], port:[%d]",tid, ip.c_str(),port);
		CGameUnit *pGameUnit = pDecoder->get_game_unit();
		if(pGameUnit != NULL)
		{
			log_debug("ProcessEnterRoom||uid:[%d], tid:[%d], ip:[%s], port:[%d]", pDecoder->get_flag(), tid, ip.c_str(), port);
			if(pGameUnit->get_netfd()>0 && (pGameUnit->tid>>16)!=(tid>>16))//连接还在并且不是同一个server
			{
				pGameUnit->reset_helper();
			}
			pGameUnit->addr = IpMap(ip);
			pGameUnit->port = port;
			pGameUnit->tid = tid;	

			g_pErrorLog->logMsg(" ProcessEnterRoom tid:[%d], ip:[%s], port:[%d]", 
			tid, pGameUnit->addr.c_str(), pGameUnit->port);
		}
	}
	g_pErrorLog->logMsg("--------- CHelperUnit::ProcessEnterRoom end  ---------------");
	return 0;
}

int CHelperUnit::ProcessGetNewRoom(CDecoderUnit* pDecoder, NETInputPacket* pPacket)
{
	CEncryptDecrypt decrypt;
	decrypt.DecryptBuffer(pPacket);
	int tid = pPacket->ReadInt();
	string ip = pPacket->ReadString();
	int port = pPacket->ReadInt();
	CGameUnit *pGameUnit = pDecoder->get_game_unit();
	if(pGameUnit != NULL)
	{
		if(pGameUnit->get_netfd()>0 && (pGameUnit->tid>>16)!=(tid>>16))//连接还在并且不是同一个server
		{
			pGameUnit->reset_helper();
		}
		pGameUnit->addr = IpMap(ip);
		pGameUnit->port = port;
		pGameUnit->tid = tid;	

		g_pErrorLog->logMsg(" ProcessGetNewRoom tid:[%d], ip:[%s], port:[%d]", 
			tid, pGameUnit->addr.c_str(), pGameUnit->port);
	}
	return 0;
}

int CHelperUnit::process_pkg(void)
{
	g_pDebugLog->logMsg("--------------- CHelperUnit::process_pkg being --------------");
	int headLen  = sizeof(struct TPkgHeader);
	int totalLen = _r.data_len();
	g_pDebugLog->logMsg("forward packets to headLen [%d]:",headLen);
	g_pDebugLog->logMsg("forward packets to totalLen [%d]:",totalLen);
	g_pDebugLog->logMsg("forward packets to the client start headLen[%d],totalLen[%d]", headLen, totalLen);
	while( totalLen > 0)
	{
		if (totalLen < headLen)
			break;
		TPkgHeader *pHeader = (struct TPkgHeader*)(_r.data());
		if(pHeader == NULL)
		{
			log_info("pHeader==NULL");
			return -1;
		}
		int contenLen = ntohs(pHeader->length);
		log_error("helper packet body length:[%d], cmd:[%d]", ntohs(pHeader->length), ntohs(pHeader->cmd));
		if(contenLen < 0 || contenLen>10*1024)
		{
			return -1;
		}
		int pkglen = sizeof(short) + contenLen;
		if (totalLen < pkglen)
			 break;
		NETInputPacket tranPacket;
		tranPacket.Copy(_r.data(), pkglen);
		int nCmd = tranPacket.GetCmdType();
		int uid = tranPacket.ReadInt();
		int svid = tranPacket.ReadInt();
		log_error("nCmd:[0x%x]", nCmd);
		log_error("uid:[%d]", uid);
		log_error("svid:[%d]", svid);
		if(_helperpool == NULL)
		{
			log_error("_helperpool is null");
		}
		else
		{
			if(CLIENT_PACKET == nCmd)
			{
				char szTemp[10240];
				int rLen = tranPacket.ReadBinary(szTemp, sizeof(szTemp));
				g_pDebugLog->logMsg("rLen:[%d]", rLen);
				NETInputPacket tempPacket;
				int cmd = 0;
				if(rLen != -1)
				{
					tempPacket.Copy(szTemp, rLen);
					cmd = tempPacket.GetCmdType();
				}
				else
				{
					log_error("Helper ReadBinary error");
					return -1;	
				}
				g_pDebugLog->logMsg("cmd:[0x%x]", cmd);
				if( cmd == CMD_GET_ROOM_LEVER_NUM)
				{
					ProcessGetLevelCount(&tempPacket);
					_r.skip(pkglen);
					totalLen -= pkglen;
					continue;
				} 
			
				CDecoderUnit *_decoder = NULL;
				CClientUnit* clt = GetClientUintByUid(uid, &_decoder);
				if( NULL == clt || _decoder == NULL)
				{
					SendClientClose(uid);
				}
				else
				{
					if(TGlobal::_debugLogSwitch && clt->get_uid()>0)
					{
						g_pDebugLog->logMsg("%s|0x%x|%d|%d", __FUNCTION__, cmd, clt->get_uid(), clt->_api);
					}
					
					if(cmd == SERVER_CMD_LOGIN_SUCCESS)
					{	
						ProcessUserLoginSuccess(_decoder, &tempPacket); /* 这里记录AllocServer 与 Client 关联 */
					}
					else if(cmd == SERVER_CMD_ENTER_ROOM)
					{
						ProcessEnterRoom(_decoder, &tempPacket); /* 这里记录AllocServer 与 Client 关联 */
					}
					else if(cmd == CMD_GET_NEW_ROOM)
					{
						ProcessGetNewRoom(_decoder, &tempPacket); /* 这里记录AllocServer 与 Client 关联 */
					}

					if(clt->get_state() != CONN_FATAL_ERROR) /* 这里协议包返回给client端 */
					{
						clt->add_rsp_buf(szTemp, rLen);
						clt->send();
					}
					else
					{
						_decoder->complete();
					}
				}
				
			}
			else if(nCmd == SERVER_CLOSE_PACKET)
			{
				CDecoderUnit *_decoder = NULL;
				CClientUnit* clt = GetClientUintByUid(uid, &_decoder);
				if( _decoder != NULL)
				{
					_decoder->set_conn_type(CONN_OTHER);
					_decoder->complete();
				}
			}
			else
			{
				log_error("helper invalid cmd");
			}
			log_error("processing a packet completed");		
		}	
		_r.skip(pkglen);
		totalLen -= pkglen;
	}
	log_error("forward packets to the client end");
	return 0;
}

void CHelperUnit::SendClientClose(int uid)
{
	NETOutputPacket transPacket;
	transPacket.Begin(CLIENT_CLOSE_PACKET);
	transPacket.WriteInt(uid);
	transPacket.WriteInt(TGlobal::_svid);
	transPacket.End();
	
	if((int)_helperpool->m_svidlist.size() > 0)
	{
		int svid = TGlobal::LoopSvid(_helperpool->m_svidlist);
		CHelperUnit *pHelperUnit = NULL;
		map<int, CHelperUnit*>::iterator iterHelper = _helperpool->m_helpermap.find(svid);
		if(iterHelper != _helperpool->m_helpermap.end())
		{
			pHelperUnit = iterHelper->second;
			if(pHelperUnit != NULL)
			{
				pHelperUnit->append_pkg(transPacket.packet_buf(), transPacket.packet_size());
				pHelperUnit->send_to_logic(_helperTimer);
			}
		}
		else
		{
			g_pErrorLog->logMsg("%s||Cannot find helper, svid:[%d]", __FUNCTION__, svid);
		}
	}
}

void CHelperUnit::reset_helper(void)
{
	log_debug("reset helper object");
	_w.skip( _w.data_len() );
	_r.skip( _r.data_len() );
	DisableInput();
	DisableOutput();
	ApplyEvents();
	CPollerObject::DetachPoller();
	if(netfd > 0)
	{
		::close(netfd);
	}
	netfd  = -1;
	_stage = CONN_IDLE;
}


int CHelperUnit::ProcessGetLevelCount(NETInputPacket * pPacket)
{
	CEncryptDecrypt decrypt;
	decrypt.DecryptBuffer(pPacket);
	short nCount = pPacket->ReadShort();
	for( int i = 0; i < nCount; i++)
	{
		short nLevel 	= pPacket->ReadShort();
		int& nUserCount = _helperpool->m_LevelCountMap[nLevel];
		nUserCount		= pPacket->ReadShort();
	}	

	return 0;
}

CClientUnit* CHelperUnit::GetClientUintByUid(const int &uid, CDecoderUnit **pDecoder)
{
	CClientUnit* clt = NULL;
	std::map<int, CDecoderUnit*>::iterator iter = _helperpool->m_objmap.find(uid);
	if( iter != _helperpool->m_objmap.end())
	{
		(*pDecoder) = iter->second;
		if(*pDecoder != NULL)
		{
			clt = (*pDecoder)->get_web_unit();
		}
	}
	return clt;
}
HTTP_SVR_NS_END

