
/********************************
*�ļ�����app_combo_protocol.h
*���ܣ�Ӧ�����Э�������
*���ߣ��Ӻ���
*�汾��1.0
*������ʱ�䣺2010.06.21
**********************************/

#ifndef _APP_COMBO_PROTOCOL_H_
#define _APP_COMBO_PROTOCOL_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "protocol_pack.h"
#include "protocol_err.h"
#include "data_platform_protocol.h"

namespace protocol
{

//COMBO��������������
const unsigned short COMBO_REQUEST_SUB_CMD = 0x0001;

//���������
class AppComboReq
{
public:
    AppComboReq()
    {
        m_Timeout = 0;
        m_Header.main_cmd = COMBO_REQUEST_MAIN_CMD;
        m_Header.sub_cmd = COMBO_REQUEST_SUB_CMD;
    }
    ~AppComboReq(){}
    
    void SetTimeout(unsigned short t){m_Timeout = t;}

    /*
    *���ܣ��������ݰ�
    *OUT req:���ɵ����ݰ�
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   int MakeReq(string &req)
    {
        Packer pk;
        MakeReq(pk);
        req = pk.GetPack(m_Header);
        return req.size();
    }

    /*
    *���ܣ��������ݰ�
    *OUT buf:���ɵ����ݰ�
    *IN buf_len:buf�Ĵ�С
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   int MakeReq(char *buf, unsigned int buf_len)
    {
        FixedPacker pk;
        int ret = pk.Init(buf, buf_len);
        if(ret != 0)
        {
            return -1;
        }
        MakeReq(pk);
        unsigned int err = pk.GetErr();
        if(err != 0)
        {
            return -1;
        }
        int len = 0;
        pk.GetPack(len, m_Header);
        return len;
    }


    /*
    *���ܣ����һ���������
    *IN server_type:Ҫ���͵��ĸ�server
    *IN route_key:·��Key
    *IN p:�����
    *IN len:���������
    *����ֵ����
    */
    void Add(unsigned int server_type,unsigned int route_key, char * p,unsigned int len)
    {
        uint64_t val = server_type ;
        val = (val << 32) | route_key;
        m_ComboReq.push_back(std::make_pair<string,uint64_t>(string(p,len),val));
    }

    void Add(unsigned int server_type,unsigned int route_key,const string & pk)
    {
        uint64_t val = server_type ;
        val = (val << 32) | route_key;
        m_ComboReq.push_back(std::make_pair<string,uint64_t>(pk,val));
    }

     //����Combo����Seq
    void SetSeq(unsigned int seq){m_Header.seq = seq;}

    /*
    *���ܣ�������дcookie
    *����ֵ��cookie����
    */
    string &Cookie(){return m_Cookie;}

    /*
    *���ܣ����ɱ��·��KEY�� 
    *IN rk:·��KEY�� 
    *IN isUnique:�Ƿ���Ψһ��
    *����ֵ����
   */
   void SetRouteKey(unsigned int rk, bool isUnique)
    {
        m_Header.to_uin  =rk;
        m_Header.source_type = isUnique ? 0 : 1;
    }

protected:
    template<class PACKER>
    void MakeReq(PACKER &pk)
    {
        pk.PackWord(m_Timeout);
        unsigned short req_num = m_ComboReq.size();
        pk.PackWord(req_num);     //�������ʽ��Ŀ

        unsigned int  server_type = 0;
        unsigned int route_key = 0;
        
        for(unsigned short i = 0; i < req_num; i++)
        {
            server_type = (m_ComboReq[i].second>>32);
            route_key = m_ComboReq[i].second;
            pk.PackDWord(server_type);
            pk.PackDWord(route_key);
            string & req = m_ComboReq[i].first;
            unsigned int req_size = req.size();
            pk.PackDWord(req_size);
            pk.PackBinary(req.data(), req_size);
        }

        PackCookie(pk);
    }

protected:
   void PackCookie(Packer &pk)
    {
        unsigned short cookie_len = m_Cookie.size();
        pk.PackWord(cookie_len);
        pk.PackBinary(m_Cookie.data(), cookie_len);
    }
   void PackCookie(FixedPacker &pk)
    {
        unsigned short cookie_len = m_Cookie.size();
        pk.PackWord(cookie_len);
        pk.PackBinary(m_Cookie.data(), cookie_len);
    }

private:

    static unsigned int m_DefaultSeq;
    string m_Cookie;//Combo����Cookie
    MsgHeader m_Header;//Combo�����ͷ
    typedef std::pair<string,uint64_t> item_type;
    std::vector<item_type> m_ComboReq;//Combo�������
    unsigned short m_Timeout;//Combo����ʱʱ��
};

class AppComboRsp
{
public:
    AppComboRsp()
    {
        m_RetCode = 0xffff;
        m_RecordNum = 0;
    }
    
    ~AppComboRsp(){}

    /*
    *���ܣ���������ƽ̨����Ӧ
    *IN buf:��Ӧ��
    *IN buf_len:��Ӧ���ĳ���
    *����ֵ��0�ɹ�����0ʧ��
    */
    int ParseRsp(const char *buf, unsigned int buf_len) ;

    /*
    *���ܣ�ȡ���صĴ�����
    *����ֵ�����صĴ�����
    */
    unsigned short GetRetCode(){return m_RetCode;}

    /*
    *���ܣ�ȡ���صļ�¼��
    *����ֵ�����صļ�¼��
    */
    unsigned int GetSubRspNum(){return m_RecordNum;}

    /*
    *���ܣ�������дcookie
    *����ֵ��cookie����
    */
    string &Cookie(){return m_Cookie;}

    /*
    *����:ȡ������Ӧ��
    *����˵��
    *OUT buf:��Ӧ��
    *IN buf_len:buf����
    *IN idx:�ڼ�����Ӧ��
    *����ֵ:
    * >0:��Ӧ������,<=0ʧ��
    */
    int GetSubRsp(char * buf,unsigned int buf_len,unsigned short idx);


    /*
    *����:ȡ������Ӧ��
    *����˵��
    *OUT buf:��Ӧ��
    *IN idx:�ڼ�����Ӧ��
    *����ֵ:
    * >0:��Ӧ������,<=0ʧ��
    */
    int GetSubRsp(string& subpack,unsigned short idx);


    /*
    *����:ȡ������Ӧ����������
    *����˵��
    *OUT main_cmd:��Ӧ����������
    *OUT snb_cmd:��Ӧ����������
    *IN idx:�ڼ�����Ӧ��
    *����ֵ:
    * 0:�ɹ�,��0ʧ��
    */
    int GetSubRspCmd(unsigned short & main_cmd,unsigned short & sub_cmd,unsigned short idx);


protected:
    void UnPackCookie(UnPacker &upk)
    {
        unsigned short cookie_len = upk.UnPackWord();
        upk.UnPackBinary(m_Cookie, cookie_len);
    }

    int GetRspCmd(unsigned short &main_cmd, unsigned short &sub_cmd, 
                const char *pack, unsigned int pack_len)
    {
        if(MSG_HEADER_LEN > pack_len)
        {
            return ERR_PROTOCOL;
        }
        unsigned short *p = (unsigned short *)(pack + MSG_MAIN_CMD_POS);
        main_cmd = htons(*p);
        sub_cmd = htons(*(p+1));
        return RET_OK;
    }

    
private:

    string m_Cookie;//Combo cookie��Ϣ
    unsigned short m_RetCode;//Combo������
    unsigned int m_RecordNum;//��������

    typedef std::pair<string, unsigned int> item_type;
    vector<item_type> m_ComboRsp;

    
};

}

#endif

