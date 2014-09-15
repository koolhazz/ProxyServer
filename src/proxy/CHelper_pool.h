#ifndef __C_HELPERPOOL_H__
#define __C_HELPERPOOL_H__

#include <string.h>
#include <vector>
#include <helper_unit.h>
using std::map;
using std::vector;
using std::string;
HTTP_SVR_NS_BEGIN

class CHelperPool
{
public:
	CHelperPool();
	~CHelperPool();

public:
	map<int, CHelperUnit*> 			m_helpermap;				//svid对应AllocServer
	vector<int> 					m_svidlist;
	map<int, CDecoderUnit*> 		m_objmap;
	vector<int> 					m_cmdlist;
	map<int, std::vector<int> > 	m_levelmap;					//level对应svid
	map<string, string> 			m_ipmap;					// 记录AllocServer 的ip eth0 eth1
	map<short, int> 				m_LevelCountMap;			//level---count 每个等级场的人数
	vector<int> 					m_whitelist;            	//容错命令白名单
};

HTTP_SVR_NS_END
#endif

