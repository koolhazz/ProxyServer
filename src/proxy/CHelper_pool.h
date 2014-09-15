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
	map<int, CHelperUnit*> 			m_helpermap;				//svid��ӦAllocServer
	vector<int> 					m_svidlist;
	map<int, CDecoderUnit*> 		m_objmap;
	vector<int> 					m_cmdlist;
	map<int, std::vector<int> > 	m_levelmap;					//level��Ӧsvid
	map<string, string> 			m_ipmap;					// ��¼AllocServer ��ip eth0 eth1
	map<short, int> 				m_LevelCountMap;			//level---count ÿ���ȼ���������
	vector<int> 					m_whitelist;            	//�ݴ����������
};

HTTP_SVR_NS_END
#endif

