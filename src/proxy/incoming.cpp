#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <incoming.h>
#include <memcheck.h>
#include <listener.h>
#include <config.h>
#include <log.h>
#include <Singleton.h>
#include <MarkupSTL.h>
#include <helper_unit.h>
#include <string>
using std::string;
#include <CHelper_pool.h>
#include "RealTimer.h"

#include <sstream>
#include <iostream>
#include <fstream>


HTTP_SVR_NS_BEGIN


extern CHelperPool*			_helperpool;
extern CRealTimer* 			_RealTimer;
extern CLevelCountTimer*	_LevelCountTimer;

CIncoming::CIncoming (void) :
_pollerunit (NULL),
_listener (NULL),
_timerunit (NULL),
_webtimerlist (NULL),
_helpertimerlist (NULL),
_epwaittimeout (1000)
{  
}

CIncoming::~CIncoming (void) 
{
    DELETE (_timerunit);
    DELETE (_listener);
    DELETE (_pollerunit);
}

CIncoming* CIncoming::Instance (void)
{
    return CSingleton<CIncoming>::Instance ();
}

void CIncoming::Destroy (void)
{
    return CSingleton<CIncoming>::Destroy ();
}

int CIncoming::open (void)
{ 
    //create poller unit
    NEW (CPollerUnit(TGlobal::_maxpoller), _pollerunit);

    if (NULL == _pollerunit)
    {
        log_boot ("create CPollerUnit object failed.");

        return -1;
    }

    //create listener
    NEW (CListener(TGlobal::_addr, TGlobal::_port, MAX_ACCEPT_COUNT), _listener);

    if (NULL == _listener)
    {
        log_boot ("create CListener object failed.");

        return -1;
    }

    //create CTimerUnit object
    NEW (CTimerUnit(), _timerunit);
    if (NULL == _timerunit)
    {
        log_boot ("create CTimerUnit object failed.");
        return -1;
    }

	
	_realtimerlist = _timerunit->GetTimerList(60);//时间设为1分钟
	
	_RealTimer->realTimerlist = _realtimerlist;
	_RealTimer->AttachTimer(_realtimerlist);

    //create web timer list
    _webtimerlist = _timerunit->GetTimerList (TGlobal::_webtimeout);
	
    if (NULL == _webtimerlist)
    {
        log_boot ("create web CTimerList object failed.");
        return -1;
    }

    _helpertimerlist = _timerunit->GetTimerList (TGlobal::_helpertimeout);

    if (NULL == _helpertimerlist)
    {
        log_boot ("create helper CTimerList object failed.");
        return -1;
    }
           
    //init poller unit
    if(_pollerunit->InitializePollerUnit() < 0) // _pollerunit 初始化
    {
        log_boot ("poller unit init failed.");
      	return -1;
    }
    
    //attach decoder unit  listener 关联 epoll
    if (_listener->Attach (_pollerunit, _webtimerlist, _helpertimerlist, TGlobal::_backlog) < 0)
    {
        log_boot ("invoke CListener::Attach() failed.");
        return -1;
    }
	
	//init hall connect logic server
	if(InitHelperUnit() < 0)
	{
		log_boot ("InitHelperUnit failed.");
        return -1;
	}
	
	_LevelCountTimer->SetTimerList(_timerunit->GetTimerList(60*5));
	_LevelCountTimer->SetHelperTimerList(_helpertimerlist);
	_LevelCountTimer->StartTimer();
	_LevelCountTimer->SendGetLevelCount();
    return 0;

}

int CIncoming::InitHelperUnit() // called by CIncoming::open()  初始化 server.xml
{
	map<int, vector<int> >::iterator iter = _helperpool->m_levelmap.begin();
	for(; iter!=_helperpool->m_levelmap.end(); iter++)
	{
		vector<int>& v = iter->second;
		v.clear();
	}
	
	_helperpool->m_svidlist.clear();
	
	CMarkupSTL  markup;
    if(!markup.Load("../conf/server.xml"))
    {       
        log_boot("Load server.xml failed.");
        return -1;
    }

    if(!markup.FindElem("SYSTEM"))
    {
        log_boot("Can not FindElem [SYSTEM] in server.xml failed.");
        return -1;
    }

	if (!markup.IntoElem())    
	{        
		log_boot ("IntoElem [SYSTEM] failed.");       
		return -1;    
	}
	
	if(markup.FindElem("Node"))
	{
		if (!markup.IntoElem())    
		{        
			log_boot ("IntoElem failed.");       
			return -1;    
		}

		if(!markup.FindElem("ServerList"))
		{
			log_boot ("FindElem [ServerList] failed.");     
			return -1; 
		}
		
		if (!markup.IntoElem())    
		{        
			log_boot ("IntoElem [ServerList] failed.");       
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
				pHelperUnit = new CHelperUnit(_pollerunit);
				_helperpool->m_helpermap[svid] = pHelperUnit; // 每个server对应的helper
			}
			 
			if(pHelperUnit == NULL)
			{
				log_boot("pHelperUnit NULL");
				return -1;
			}	

			_helperpool->m_svidlist.push_back(svid);  // server列表

			pHelperUnit->addr = ip;
			pHelperUnit->port = port;

			vector<int>& v = _helperpool->m_levelmap[level];
			v.push_back(svid);
			log_boot("alloc server id:[%d], level:[%d], ip:[%s], port:[%d]", svid, level, ip.c_str(), port);
		}

		if (!markup.OutOfElem())    
		{        
			log_boot ("OutOfElem [ServerList] failed.");       
			return -1;    
		}
		
		if(!markup.FindElem("CmdList"))
		{
			log_boot("Can not FindElem [CmdList] in server.xml failed.");
			return -1;
		}
		
		if (!markup.IntoElem())    
		{        
			log_boot ("IntoElem [CmdList] failed.");       
			return -1;    
		}

		_helperpool->m_cmdlist.clear();
		
		while(markup.FindElem("Cmd"))
		{
			int cmd = atoi(markup.GetAttrib("value").c_str());
			log_boot("cmd:[%d]", cmd);
			_helperpool->m_cmdlist.push_back(cmd);
		}
		
		if (!markup.OutOfElem())    
		{        
			log_boot ("OutOfElem [CmdList] failed.");       
			return -1;    
		}
	}

//新增ip映射
	if (!markup.OutOfElem())    
	{        
		log_boot ("OutOfElem [ServerList] failed.");       
		return -1;    
	}

	if(!markup.FindElem("IPMap"))
	{
		log_boot ("FindElem [IPMap] failed.");     
		return -1; 
	}

	if (!markup.IntoElem())    
	{        
		log_boot ("IntoElem [IPMap] failed.");       
		return -1;    
	}

	while(markup.FindElem("IP"))
	{
		string eth0 = markup.GetAttrib("eth0");
		string eth1 = markup.GetAttrib("eth1");
		log_boot("%s||eth0:[%s], eth1:[%s]", __FUNCTION__, eth0.c_str(), eth1.c_str());
		_helperpool->m_ipmap[eth0] = eth1;
	}

	if (!markup.OutOfElem())    
	{        
		log_boot ("OutOfElem [IPMap] failed.");       
		return -1;    
	}

	_helperpool->m_whitelist.clear();

	if(!markup.FindElem("WhiteCmdList"))
	{
		log_boot ("FindElem [WhiteCmdList] failed.");     
		return -1; 
	}

	if (!markup.IntoElem())    
	{        
		log_boot ("IntoElem [WhiteCmdList] failed.");       
		return -1;    
	}
	
	while(markup.FindElem("CmdList"))
	{
		string strCmds = markup.GetAttrib("cmds");
		vector<string> vec;
		TGlobal::split_str(strCmds.c_str(), ",", vec);
		for(int i=0; i<(int)vec.size(); i++)
		{
			int cmd = atoi(vec[i].c_str());
			_helperpool->m_whitelist.push_back(cmd);
			log_boot("%s||cmd:[%d]", __FUNCTION__, cmd);
		}
	}
	return 0;
}


int CIncoming::run (void)
{
    while(!(*(TGlobal::_module_close)))
    {
        _pollerunit->WaitPollerEvents (_timerunit->ExpireMicroSeconds(_epwaittimeout));
        uint64_t now = GET_TIMESTAMP();
        _pollerunit->ProcessPollerEvents(); // 处理事件
        _timerunit->CheckExpired (now);
        _timerunit->CheckPending ();
    }

    return 0;
}
HTTP_SVR_NS_END

