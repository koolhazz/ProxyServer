
/********************************
*�ļ�����data_platform_protocol.h
*���ܣ�����ƽ̨Э�������
*���ߣ��ż���
*�汾��1.0
*������ʱ�䣺2009.06.09
**********************************/

#ifndef _DATA_PLATFORM_PROTOCOL_H_
#define _DATA_PLATFORM_PROTOCOL_H_

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

using std::string;
using std::vector;

#include "protocol_pack.h"

namespace protocol
{
#define DP_ADD_COOKIE(req,ck) do{if(ck){int _r_ = (ck)->Pack(req.Cookie());if(_r_ == -1)return _r_;}}while(0)
/*TLV����
T(Type)���������ͣ�1�ֽ�
		0��integer8��1�ֽ�
		1��integer16��2�ֽ�
2��integer32��4�ֽ�
3��integer64��8�ֽ�
4��real��8�ֽ�
5��string���䳤
6��binary���䳤
L(Length)��value���ݳ��ȣ�4�ֽ�
V(value)���ֽ�����
*/
const unsigned char TLV_BYTE = 0;
const unsigned char TLV_WORD = 1;
const unsigned char TLV_DWORD = 2;
const unsigned char TLV_QWORD = 3;
const unsigned char TLV_REAL = 4;
const unsigned char TLV_STRING = 5;
const unsigned char TLV_BINARY = 6;




//���λ����
const unsigned char SYNC_FLAG_BIT = 1;
const unsigned short SYNC_FLAG_MASK = 0x0001;

const unsigned char PRIMARY_KEY_FLAG_BIT = 2;
const unsigned char PRIMARY_KEY_FLAG_MASK = 0x0002;


//����ƽ̨���������ֶ���
const unsigned short INSERT_MAIN_CMD = 0x0500;
const unsigned short UPDATE_MAIN_CMD = 0x0501;
const unsigned short DELETE_MAIN_CMD = 0x0502;
const unsigned short QUERY_MAIN_CMD=0x0503;
const unsigned short COUNT_MAIN_CMD = 0x0504;
const unsigned short DELTA_MAIN_CMD = 0x0505;


//��չӦ�������ֶ���

//COMBO������������
const unsigned short COMBO_REQUEST_MAIN_CMD = 0x0900;

//���ҷ�����������
const unsigned short COND_FIND_MAIN_CMD = 0x0981;

//����ƽ̨status״̬�붨��
const unsigned short DP_RET_OK = 0;                             //�ɹ�
const unsigned short DP_ERR_GENERAL = 0x0001;       //һ�����
const unsigned short DP_ERR_NOT_FOUND = 0x0002; //��¼������
const unsigned short DP_ERR_DUPLICATE_ID = 0x0003;  //�ظ���
const unsigned short DP_ERR_DB = 0x0004;  //DB����
const unsigned short DP_ERR_PROTOCOL = 0x0005;  //Э���ʽ����
const unsigned short DP_ERR_TOO_BIG = 0x0006;  //���������
const unsigned short DP_ERR_DATA_ERR = 0x0007;// for RET_DB_ERR_PARAM and RET_DB_ERR_FIELD
const unsigned short DP_ERR_TIME_OUT = 0x0008;  
const unsigned short DP_ERR_NETWORK_ERR = 0x0009;
const unsigned short DP_ERR_CMD_UNKNOWN = 0x0081;  //δ֪����



//�������ʽ�Ĳ���������
const unsigned char OP_EQUAL = 0;           // =
const unsigned char OP_NOT_EQUAL = 1;   // <>
const unsigned char OP_ABOVE = 2;           // >
const unsigned char OP_BELLOW = 3;          // <
const unsigned char OP_ABOVE_EQUAL = 4; // >=
const unsigned char OP_BELLOW_EQUAL = 5;    //<=
const unsigned char OP_BIT_ALL = 6; //����������Ϊ1��bitλ��������cond_val == (cond_val & value)
const unsigned char OP_BIT_ONE = 7;//������Ϊ1��bitλ��������һ��0 != (cond_val & value)
const unsigned char OP_LIKE = 8;    //sql like

class TLV
{
public:
    unsigned char type;
    unsigned int length;
    uint64_t int_value; //����ֵ
    double real_value; //ʵ��ֵ
    string string_value; //�ַ���ֵ�������ֵ

public:
    TLV(){}
    TLV(unsigned char v){Assign(v);}
    TLV(char v){Assign((unsigned char)v);}

    TLV(unsigned short v){Assign(v);}
    TLV(short v){Assign((unsigned short)v);}

    TLV(unsigned int v){Assign(v);}
    TLV(int v){Assign((unsigned int)v);}

    TLV(uint64_t v){Assign(v);}
    TLV(int64_t v){Assign((uint64_t)v);}

    TLV(double v){Assign(v);}
    TLV(const string& v){Assign(v);}
    TLV(const char *v){Assign(v);}
    TLV(const void *v, unsigned int len){Assign(v, len);}

    TLV(const TLV &r){*this=r;}
    TLV &operator=(const TLV &r)
    {
        this->type = r.type;
        this->length = r.length;
        this->int_value = r.int_value;
        this->real_value = r.real_value;
        if( (r.type == TLV_STRING) || (r.type == TLV_BINARY))
        {
                this->string_value = r.string_value;
        }
        return *this;
    }

    //������ֵ����
    void Assign(unsigned char v){type = TLV_BYTE;length=1;int_value=v;}
    void Assign(unsigned short v){type = TLV_WORD; length=2;int_value=v;}
    void Assign(unsigned int v){type = TLV_DWORD;length = 4; int_value = v;}
    void Assign(uint64_t v){type = TLV_QWORD; length = 8; int_value = v;}
        
    //��������ֵ����
    void Assign(double v){type = TLV_REAL; length = 8; real_value = v;}

    //�ַ�����ֵ����
    void Assign(const string &v){type = TLV_STRING; length = v.size(); string_value = v;}
    void Assign(const char *v)
    {
        type = TLV_STRING; 
        if(v)
        {
            length = strlen(v); 
            string_value = v;
        }
    }

    //�����Ƹ�ֵ����    
    void Assign(const void *v, unsigned int len)
    {   
        type = TLV_BINARY; 
        length = len;
        if(len > 0)
        {
            string_value.assign((const char *)v, len);
        }
    }         

};

class ConditionExpression
{
public:
    unsigned int field_id;
    unsigned char op_code;      //������
    TLV data;

    ConditionExpression(){}

    ConditionExpression(unsigned int fid, unsigned char op, const TLV &d)
    :field_id(fid),op_code(op), data(d)
    {
    }

    ConditionExpression(const ConditionExpression &r){*this = r;}
    ConditionExpression &operator=(const ConditionExpression &r)
    {
        field_id = r.field_id;
        op_code = r.op_code;
        data = r.data;
        return *this;
    }

};

#if 0
class DataPlatformProtocol
{
public:
    /*
    *���ܣ������ݰ����ɺ��������ݿ�·��KEY
    *OUT pack:���ݰ�
    *IN key:Ҫ�ŵ���ͷ�е�key
    *IN unique:�Ƿ���Ψһkey.  
                true:�ǣ� false:���ǣ��������Ҫȫ��ɨ��
    *����ֵ��0�ɹ�����0ʧ��
    */
    static int SetRouteKey(string &pack, unsigned int key, bool unique=true);

    /*
    *���ܣ�����ֻ��һ�������Ĳ�ѯ����.

    *OUT pack:����������������ɵ������
    *IN is_primary_key:��ѯ�����Ƿ�������
            true�������������Ȳ�CACHE�������о��ٲ�DB
            false���������,����CACHE��ֱ�Ӳ�DB.
     !!!������ʱ����׼ȷ��д�����ǣ�
                ������ܵõ���׼ȷ������(dirty data).
            
    *IN cond: �������ʽ
    *IN ret_field_num:��Ҫ���ص��ֶ���
    *IN ret_fieldid_set:��Ҫ���ص��ֶ�ID��
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������
    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeQueryReq(string &pack,bool is_primary_key, const ConditionExpression &cond, 
                                                    unsigned short ret_field_num,unsigned int *ret_filedid_set, unsigned int seq);
    static int MakeQueryReq(string &pack,bool is_primary_key, const ConditionExpression &cond, 
                                                    unsigned short ret_field_num,unsigned int *ret_filedid_set, unsigned int seq,
                                                    unsigned short cookie_len, const void *cookie);

    /*
    *���ܣ������ж�������Ĳ�ѯ����.

    *OUT pack:����������������ɵ������

    *IN is_primary_key:��ѯ�����Ƿ�������
            true�������������Ȳ�CACHE�������о��ٲ�DB
            false���������,����CACHE��ֱ�Ӳ�DB.
     !!!������ʱ����׼ȷ��д�����ǣ�
                ������ܵõ���׼ȷ������(dirty data).
            
    *IN cond_num: �������ʽ�ĸ���
    *IN condtions:����������ʽ������
    *IN logic_expression:�������֮����߼����ʽ
            ����$1 AND $2
    *IN ret_field_num:��Ҫ���ص��ֶ���
    *IN ret_fieldid_set:��Ҫ���ص��ֶ�ID��
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������

    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeQueryReq(string &pack, bool is_primary_key, 
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const char *logic_expression, unsigned short ret_field_num,
                    unsigned int *ret_filedid_set, unsigned int seq);

    static int MakeQueryReq(string &pack, bool is_primary_key, 
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const char *logic_expression, unsigned short ret_field_num,
                    unsigned int *ret_filedid_set, unsigned int seq, unsigned short cookie_len, const void *cookie);

    /*
    *���ܣ�����Ӧ������ȡ���������ֺ���������
    *main_cmd:���������ȡ������������
    *sub_cmd:���������ȡ������������
    *pack:�����������Ӧ��
    *pack_len:�����������Ӧ������

    *����ֵ��0����ɹ�
    */
    static int GetRspCmd(unsigned short &main_cmd, unsigned short &sub_cmd, 
                const char *pack, unsigned int pack_len);


    /*
    *���ܣ�����Query��Ӧ��
    *OUT status:���������������
    *OUT data:�����������¼��
    *IN pack:�����������Ӧ��
    *IN pack_len:�����������Ӧ������
    *DEFAULT OUT MsgHeader *h:������������ΪNULL��ʾ����Ҫ��
            ��ΪNULL��ʾҪ�õ���Ϣͷ

    *����ֵ��0����ɹ�
    */
    static int ParseQueryRsp(unsigned short &status, std::vector<unsigned int> &fileds,
                std::vector<TLV> &data, const char *pack, unsigned int pack_len,
                MsgHeader *h = NULL);   
    /*
    *   OUT cookie: �Ӻ��ԭ�����ص�cookie����
    */
    static int ParseQueryRsp(string &cookie, unsigned short &status, std::vector<unsigned int> &fileds,
                std::vector<TLV> &data, const char *pack, unsigned int pack_len,
                MsgHeader *h = NULL);   


    /*
    *���ܣ�����count����

    *OUT pack:����������������ɵ������

            
    *IN cond_num: �������ʽ�ĸ���
    *IN condtions:����������ʽ������
    *IN logic_expression:�������֮����߼����ʽ
            ����$1 AND $2
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������

    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeCountReq(string &pack, 
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const char *logic_expression, unsigned int seq);

    static int MakeCountReq(string &pack, 
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const char *logic_expression, unsigned int seq, 
                    unsigned short cookie_len, const void *cookie);

    /*
    *���ܣ�����count��Ӧ��
    *status:���������������
    *record_num:�����������¼��
    *pack:�����������Ӧ��
    *pack_len:�����������Ӧ������
    *MsgHeader *h:������������ΪNULL��ʾ����Ҫ��
            ��ΪNULL��ʾҪ�õ���Ϣͷ

    *����ֵ��0����ɹ�
    */
    static int ParseCountRsp(unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len,MsgHeader *h = NULL);   

    /*
    *   OUT cookie: �Ӻ��ԭ�����ص�cookie����
    */
    static int ParseCountRsp(string &cookie, unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len,MsgHeader *h = NULL);   


    /*
    *���ܣ�����delete����

    *OUT pack:����������������ɵ������


     !!!��key����������.
    *IN is_asyn:�Ƿ��첽���
                true:�첽��⣬��Ӧ���٣����п���ʧ�ܣ�
                    ��Ȼ���ʺ�С���������п��ܷ�����
                    �Ժ������ݣ����鲻Ҫʹ���첽��ʽ��
                false:ͬ����⣬��Ӧ��������ȷ���ɹ�.                    
    *IN cond_num: �������ʽ�ĸ���
    *IN condtions:����������ʽ������
    *IN logic_expression:�������֮����߼����ʽ
            ����$1 AND $2
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������

    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeDeleteReq(string &pack, bool is_async,
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    unsigned int seq);

    static int MakeDeleteReq(string &pack, bool is_async,
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    unsigned int seq, unsigned short cookie_len, const void *cookie);


    /*
    *���ܣ�����delete��Ӧ��
    *status:���������������
    *record_num:���������ɾ���ļ�¼��
    *pack:�����������Ӧ��
    *pack_len:�����������Ӧ������
    *MsgHeader *h:������������ΪNULL��ʾ����Ҫ��
            ��ΪNULL��ʾҪ�õ���Ϣͷ
    *����ֵ��0����ɹ�
    */
    static int ParseDeleteRsp(unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

    /*
    *   OUT cookie: �Ӻ��ԭ�����ص�cookie����
    */
    static int ParseDeleteRsp(string &cookie, unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

    /*
    *���ܣ�����update����

    *OUT pack:����������������ɵ������

     !!!��key����������.
    *IN is_asyn:�Ƿ��첽���
                true:�첽��⣬��Ӧ���٣����п���ʧ�ܣ�
                    ��Ȼ���ʺ�С���������п��ܷ�����
                    �Ժ������ݣ����鲻Ҫʹ���첽��ʽ��
                false:ͬ����⣬��Ӧ��������ȷ���ɹ�.                    
    *IN cond_num: �������ʽ�ĸ���
    *IN condtions:����������ʽ������
    *IN logic_expression:�������֮����߼����ʽ
            ����$1 AND $2
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������

    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeUpdateReq(string &pack, bool is_async,
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const unsigned int update_field_num, const unsigned int *update_field_set,
                    const TLV *update_value_set,  unsigned int seq);

    static int MakeUpdateReq(string &pack, bool is_async,
                    unsigned short cond_num, const ConditionExpression *condtions,  
                    const unsigned int update_field_num, const unsigned int *update_field_set,
                    const TLV *update_value_set,  unsigned int seq,
                    unsigned short cookie_len, const void *cookie);

    /*
    *���ܣ�����update��Ӧ��
    *status:���������������
    *record_num:���������update�ļ�¼��
    *pack:�����������Ӧ��
    *pack_len:�����������Ӧ������
    *MsgHeader *h:������������ΪNULL��ʾ����Ҫ��
            ��ΪNULL��ʾҪ�õ���Ϣͷ
    *����ֵ��0����ɹ�
    */
    static int ParseUpdateRsp(unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

    /*
    *   OUT cookie: �Ӻ��ԭ�����ص�cookie����
    */
    static int ParseUpdateRsp(string &cookie, unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

    
    /*
    *���ܣ�����insert����

    *OUT pack:����������������ɵ������

     !!!��key����������.
    *IN is_asyn:�Ƿ��첽���
                true:�첽��⣬��Ӧ���٣����п���ʧ�ܣ�
                    ��Ȼ���ʺ�С���������п��ܷ�����
                    �Ժ������ݣ����鲻Ҫʹ���첽��ʽ��
                false:ͬ����⣬��Ӧ��������ȷ���ɹ�.                    
    *IN ins_field_num:���������Ҫ������ֶθ���
    *IN ins_field_set:���������Ҫ������ֶ�ID�б�
    *IN ins_value_set:���������Ҫ������ֶ�ֵ�б�
    *IN seq:�������к�
    *IN cookie_len:cookie�ĳ���
    *IN cookie: cookie������

    *����ֵ��0�ɹ�����������������������Э������붨��
    */
    static int MakeInsertReq(string &pack, bool is_async,
                    const unsigned int ins_field_num, const unsigned int *ins_field_set,
                    const TLV *ins_value_set,  unsigned int seq);

    static int MakeInsertReq(string &pack, bool is_async,
                    const unsigned int ins_field_num, const unsigned int *ins_field_set,
                    const TLV *ins_value_set,  unsigned int seq, 
                    unsigned short cookie_len, const void *cookie);

    /*
    *���ܣ�����insert��Ӧ��
    *status:���������������
    *record_num:���������insert�ļ�¼��
    *pack:�����������Ӧ��
    *pack_len:�����������Ӧ������
    *MsgHeader *h:������������ΪNULL��ʾ����Ҫ��
            ��ΪNULL��ʾҪ�õ���Ϣͷ
    *����ֵ��0����ɹ�
    */
    static int ParseInsertRsp(unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

    /*
    *   OUT cookie: �Ӻ��ԭ�����ص�cookie����
    */
    static int ParseInsertRsp(string &cookie, unsigned short &status, unsigned int &record_num, 
        const char *pack, unsigned int pack_len, MsgHeader *h = NULL);   

};

#endif


//======New API.



class BaseCookie
{
public:
        virtual ~BaseCookie(){}
       /*
        *���ܣ�����cookie
        *OUT cookie:���ɵ�cookie����
        *����ֵ��cookie����
        */
       virtual int Pack(string &cookie) = 0;

       /*
        *���ܣ�����cookie
        *OUT buf:���ɵ�cookie����
        *IN len:buf�Ĵ�С
        *����ֵ�����ɵ�cookie��С
        */
       virtual int Pack(char *buf, unsigned int len) = 0;

       /*
        *���ܣ�����cookie
        *IN buf:cookie����
        *IN len:buf�Ĵ�С
        *����ֵ��0�ɹ�����0ʧ��
        */
       virtual int UnPack(const char *buf, unsigned int len) = 0;    
    
};


class CommCookie : public BaseCookie
{
public:
        /*
        *���ܣ�Ĭ�Ϲ��캯��������û�г�ʼ��
        */
        CommCookie():m_pReqPack(0),m_ReqPackLen(0){}

        CommCookie( unsigned short source, unsigned int flow,
                                        const char *pReqPack, unsigned int reqPackLen 
                                        ) : m_Source(source), m_Flow(flow),
                                        m_pReqPack(pReqPack), m_ReqPackLen(reqPackLen)
                                        {}

        CommCookie( unsigned short source, unsigned int flow,
                                        const char *pReqPack, unsigned int reqPackLen, 
                                        const string &extInfo) : m_Source(source), m_Flow(flow),
                                        m_pReqPack(pReqPack), m_ReqPackLen(reqPackLen),
                                        m_strExtInfo(extInfo){}

        virtual ~CommCookie(){}
       /*
        *���ܣ�����cookie
        *OUT cookie:���ɵ�cookie����
        *����ֵ��cookie����
        */
       virtual int Pack(string &cookie) ;

       /*
        *���ܣ�����cookie
        *OUT buf:���ɵ�cookie����
        *IN len:buf�Ĵ�С
        *����ֵ�����ɵ�cookie��С
        */
       virtual int Pack(char *buf, unsigned int len) ;

       /*
        *���ܣ�����cookie
        *IN buf:cookie����
        *IN len:buf�Ĵ�С
        *����ֵ��0�ɹ�����0ʧ��
        */
       virtual int UnPack(const char *buf, unsigned int len);    
       virtual int UnPack(const string & ck){return UnPack(ck.data(), ck.size());}    
       
       unsigned short m_Source;     //cookie����Դ
       unsigned int m_Flow;         //ԭ�����flow
       const char *m_pReqPack;             //ԭ����İ�������Ϊ��
       unsigned int m_ReqPackLen;

       string m_strExtInfo;         //ҵ���Զ�����չ���ݡ�����Ϊ��

};


/*
*�ֶμ���
*/
class DpFieldSet : public vector<unsigned int>
{
public:
     typedef vector<unsigned int> parent;
public:
    /*
    *���ܣ�����һ���ֶ�
    *IN id: �ֶ�ID
    *����ֵ����
    */
     void Add(unsigned int id){ parent::push_back(id);}
};

/*
*�ֶ�  ���� ����   ����
*/
class DpFieldValueSet : public vector<ConditionExpression>
{
public:
        typedef vector<ConditionExpression> parent;
        
public:
        /*
        *���ܣ�����һ���ֶβ�������
        *IN fid:�ֶ�ID
        *IN optype:��������
        *IN v:����
        *����ֵ����
        */
       void Add(unsigned int fid, unsigned char optype, const TLV &v)
       {
            parent::push_back(ConditionExpression(fid, optype,v));
       }

        /*
        *���ܣ�����һ���ֶβ�������,��������Ϊ=
        *IN fid:�ֶ�ID
        *IN v:����
        *����ֵ����
        */
       void Add(unsigned int fid, const TLV &v)
       {
           Add(fid, OP_EQUAL, v);
       }
};          
   

//����ƽ̨������Ļ���
class BaseDpReq
{
public:
        /*
        *���ܣ�Ĭ�Ϲ��캯������ʼ����seq �� sub_cmd
        */
       BaseDpReq(){m_Header.seq = m_DefaultSeq++; m_Header.sub_cmd = 0;}
       virtual ~BaseDpReq(){}

        /*
        *���ܣ��������ݰ�
        *OUT req:���ɵ����ݰ�
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(string &req)=0;

        /*
        *���ܣ��������ݰ�
        *OUT buf:���ɵ����ݰ�
        *IN buf_len:buf�Ĵ�С
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(char *buf, unsigned int buf_len) = 0;
        
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

       /*
        *���ܣ����ɱ��·��KEY�� 
        *IN rk:·��KEY�� 
        *IN isUnique:�Ƿ���Ψһ��
        *����ֵ����
       */
       void SetRouteKey(const char *rk, bool isUnique);
       void SetRouteKey(const string &rk, bool isUnique)
        {
            return SetRouteKey(rk.c_str(),isUnique);
        }
        
        static unsigned int MakeRouteKey(const char *rk);
        static unsigned int MakeRouteKey(const string &rk){return MakeRouteKey(rk.c_str());}    

       /*
        *���ܣ�ȡ·��KEY�� 
        *����ֵ��·��key
       */
       unsigned int GetRouteKey(){return m_Header.to_uin;}

       //����ʡ�������캯���л��Զ����ɡ�
       void SetSeq(unsigned int seq){m_Header.seq = seq;}
    
       /*
        *���ܣ�������дcookie
        *����ֵ��cookie����
       */
      string &Cookie(){return m_Cookie;}

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

protected:
       static unsigned int m_DefaultSeq;
        string m_Cookie;
        MsgHeader m_Header;
};

//����ƽ̨��Ӧ���Ļ���    
class BaseDpRsp
{
public:
        /*
        *���ܣ�Ĭ�Ϲ��캯������ʼ����m_RetCode �� m_RecordNum
        */
       BaseDpRsp():m_RetCode(0xffff),m_RecordNum(0){}
       virtual ~BaseDpRsp(){}

        /*
        *���ܣ���������ƽ̨����Ӧ
        *IN buf:��Ӧ��
        *IN buf_len:��Ӧ���ĳ���
        *����ֵ��0�ɹ�����0ʧ��
        */
       virtual int ParseRsp(const char *buf, unsigned int buf_len) ;

    
        /*
        *���ܣ�ȡ���صĴ�����
        *����ֵ�����صĴ�����
        */
       unsigned short GetRetCode(){return m_RetCode;}

        /*
        *���ܣ�ȡ���صļ�¼��
        *����ֵ�����صļ�¼��
        */
       unsigned int GetRecordNum(){return m_RecordNum;}


        /*
        *���ܣ�ȡ��Ӧ���е�������
        *OUT main_cmd:��������
        *OUT sub_cmd:��������
        *IN pack:���ݰ�
        *IN pack_len:���ݰ��Ĵ�С
        *����ֵ��0�ɹ�����0 ʧ��
        */
        static int GetRspCmd(unsigned short &main_cmd, unsigned short &sub_cmd, 
                const char *pack, unsigned int pack_len);

       /*
        *���ܣ�������дcookie
        *����ֵ��cookie����
       */
      string &Cookie(){return m_Cookie;}

protected:
       void UnPackCookie(UnPacker &upk)
        {
            unsigned short cookie_len = upk.UnPackWord();
            upk.UnPackBinary(m_Cookie, cookie_len);
        }

        string m_Cookie;
       unsigned short m_RetCode;
       unsigned int m_RecordNum;
};
 
//����ƽ̨query������
class DpQueryReq : public BaseDpReq
{
public:
    /*
    *���ܣ�Ĭ�Ϲ��캯����
        ��ʼ����m_LogicExp �� m_bPrimaryKey
    */
    DpQueryReq():m_LogicExp("$1"),m_bPrimaryKey(true){m_Header.main_cmd = QUERY_MAIN_CMD;}
    virtual ~DpQueryReq(){}

    /*
    *���ܣ��������ݰ�
    *OUT req:���ɵ����ݰ�
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   virtual int MakeReq(string &req);

    /*
    *���ܣ��������ݰ�
    *OUT buf:���ɵ����ݰ�
    *IN buf_len:buf�Ĵ�С
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
    virtual int MakeReq(char *buf, unsigned int buf_len);

    /*
    *���ܣ��������
    *IN fid:�ֶ�ID
    *IN op_type:��������
    *IN TLV:����ֵ. 
    *����ֵ����
    */
    void AddCond(unsigned int fid, unsigned char optype, const TLV &v){m_CondSet.Add(fid,optype,v);}

    /*
    *���ܣ����Ҫ���ص��ֶ�ID
    *IN fid:�ֶ�ID
    *����ֵ����
    */
    void AddRetField(unsigned int id){m_RetFieldSet.Add(id);}

    /*
    *���ܣ������߼����ʽ.
                        example: $1 AND $2 OR $3 limit 100
    *IN exp:�߼����ʽ
    *����ֵ����
    */
    void SetLogicExp(const char *exp){m_LogicExp = exp;}

    /*
    *���ܣ���������
    *IN isPrimaryKey:�Ƿ�Ϊ����
    *����ֵ����
    */
    void SetPrimaryKey(bool isPrimaryKey){m_bPrimaryKey = isPrimaryKey;}

private: 
 
    DpFieldSet m_RetFieldSet;
    DpFieldValueSet m_CondSet;
    string m_LogicExp;
    bool m_bPrimaryKey;
};
    

//����ƽ̨update������    
class DpUpdateReq : public BaseDpReq
{
public:

    DpUpdateReq():m_bAsyn(false){m_Header.main_cmd = UPDATE_MAIN_CMD;}
    virtual ~DpUpdateReq(){}

    /*
    *���ܣ��������ݰ�
    *OUT req:���ɵ����ݰ�
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   virtual int MakeReq(string &req);

    /*
    *���ܣ��������ݰ�
    *OUT buf:���ɵ����ݰ�
    *IN buf_len:buf�Ĵ�С
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
    virtual int MakeReq(char *buf, unsigned int buf_len);

    /*
    *���ܣ����Ҫ���µ��ֶ�
    *IN fid:�ֶ�ID
    *IN TLV:����ֵ. 
    *����ֵ����
    */
    void AddUpdData(unsigned int fid, const TLV &v){m_UpdFieldSet.Add(fid, v);}

    /*
    *���ܣ��������
    *IN fid:�ֶ�ID
    *IN op_type:��������
    *IN TLV:����ֵ. 
    *����ֵ����
    */
    void AddCond(unsigned int fid,unsigned char optype, const TLV &v){m_CondFieldSet.Add(fid,optype, v);}

    /*
    *���ܣ������߼����ʽ.
                        ĿǰJCACHE��֧���߼����ʽ.
                        ֻ����������
    *IN exp:�߼����ʽ
    *����ֵ����
    */
    void SetLogicExp(const char *exp){m_LogicExp = exp;}

    /*
    *���ܣ�����JCACHE�Ĳ�����ʽ�Ƿ�Ϊ�첽
    *IN b:�Ƿ��첽
    *����ֵ����        
    */
    void SetAsync(bool b){m_bAsyn = b;}

protected:
    DpFieldValueSet m_UpdFieldSet;
    DpFieldValueSet m_CondFieldSet;             
    string m_LogicExp;
    bool m_bAsyn;
};  


//����ƽ̨insert������
class DpInsertReq : public BaseDpReq
{
public:

        DpInsertReq():m_bAsyn(false){m_Header.main_cmd = INSERT_MAIN_CMD;}
        virtual ~DpInsertReq(){}
        /*
        *���ܣ��������ݰ�
        *OUT req:���ɵ����ݰ�
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(string &req);

        /*
        *���ܣ��������ݰ�
        *OUT buf:���ɵ����ݰ�
        *IN buf_len:buf�Ĵ�С
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(char *buf, unsigned int buf_len);

        /*
        *���ܣ����Ҫ������ֶ�
        *IN fid:�ֶ�ID
        *IN TLV:����ֵ. 
        *����ֵ����
        */
        void AddInsData(unsigned int fid, const TLV &v){m_InsFieldSet.Add(fid, v);}

        void SetAsync(bool b){m_bAsyn = b;}

protected:
    DpFieldValueSet m_InsFieldSet;

    bool m_bAsyn;

};

//����ƽ̨delete������
class DpDeleteReq : public BaseDpReq
{
public:
        DpDeleteReq():m_bAsyn(false){m_Header.main_cmd = DELETE_MAIN_CMD;}
        virtual ~DpDeleteReq(){}
        /*
        *���ܣ��������ݰ�
        *OUT req:���ɵ����ݰ�
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(string &req);

        /*
        *���ܣ��������ݰ�
        *OUT buf:���ɵ����ݰ�
        *IN buf_len:buf�Ĵ�С
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(char *buf, unsigned int buf_len);
    
        /*
        *���ܣ��������
        *IN fid:�ֶ�ID
        *IN op_type:��������
        *IN TLV:����ֵ. 
        *����ֵ����
        */
        void AddCond(unsigned int fid,unsigned char optype, const TLV &v){m_CondFieldSet.Add(fid,optype, v);}

        //Ŀǰֻ��������������ƽ̨������߼����ʽ.
        void SetLogicExp(const char *exp){m_LogicExp = exp;}

protected:
    DpFieldValueSet m_CondFieldSet;
    string m_LogicExp;
    bool m_bAsyn;
};

//����ƽ̨����ͳ��������
class DpCountReq : public BaseDpReq
{
public:
        DpCountReq():m_bAsyn(false){m_Header.main_cmd = COUNT_MAIN_CMD;}
        virtual ~DpCountReq(){}
        /*
        *���ܣ��������ݰ�
        *OUT req:���ɵ����ݰ�
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(string &req);

        /*
        *���ܣ��������ݰ�
        *OUT buf:���ɵ����ݰ�
        *IN buf_len:buf�Ĵ�С
        *IN pCookie:cookie����. 
        *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
        */
       virtual int MakeReq(char *buf, unsigned int buf_len);
    
        /*
        *���ܣ��������
        *IN fid:�ֶ�ID
        *IN op_type:��������
        *IN TLV:����ֵ. 
        *����ֵ����
        */
        void AddCond(unsigned int fid,unsigned char optype, const TLV &v){m_CondFieldSet.Add(fid,optype, v);}

        /*
        *���ܣ������߼����ʽ.
        *IN exp:�߼����ʽ
        *����ֵ����
        */
        void SetLogicExp(const char *exp){m_LogicExp = exp;}

protected:
    DpFieldValueSet m_CondFieldSet;
    string m_LogicExp;
    bool m_bAsyn;

};

//����һ���ֶμӻ��һ������ֵ��
class DpDeltaReq : public DpUpdateReq
{
public:
    DpDeltaReq(){m_Header.main_cmd = DELTA_MAIN_CMD;}
    virtual ~DpDeltaReq(){}

    /*
    *���ܣ���ĳ�������ֶ����ӻ����һ����ֵ��
    *ע�⣺V�����ͱ������ֶε�����һ��
    */
    void Delta(unsigned int fid,  char v){m_UpdFieldSet.Add(fid, TLV(v));}
    void Delta(unsigned int fid,  short v){m_UpdFieldSet.Add(fid, TLV(v));}
    void Delta(unsigned int fid,  int v){m_UpdFieldSet.Add(fid, TLV(v));}
    void Delta(unsigned int fid,  int64_t v){m_UpdFieldSet.Add(fid, TLV(v));}
};  

//����ƽ̨��Ӧ������
class DpQueryRsp : public BaseDpRsp
{
public:
 
    /*
    *���ܣ���������ƽ̨����Ӧ
    *IN buf:��Ӧ��
    *IN buf_len:��Ӧ���ĳ���
    *����ֵ��0�ɹ�����0ʧ��
    */
     virtual int ParseRsp(const char *buf, unsigned int buf_len);

     /*
    *���ܣ�ȡ����
    *�������Խ�磬��throw�쳣��
    *IN row_idx:�к�(��0��ʼ����)
    *IN col_idx:�к�(��0��ʼ����)
    *����ֵ��ȡ�õ�����
    */
     TLV &GetData(unsigned int row_idx, unsigned int col_idx)
     {
      return m_RetData[row_idx*m_RetFields.size() + col_idx];
     }

     /*
    *���ܣ�ȡ���ص��ֶθ���
    *����ֵ���ֶθ���
    */
    unsigned int GetFieldNum(){return m_RetFields.size();}

    /*
    *����:ȡ�÷��ص��ֶ�ID
    *����ֵ:�ֶ�ID
    */
    unsigned int GetFieldID(unsigned int col_idx)
    {
        return m_RetFields[col_idx];
    }

    
private:
     DpFieldSet m_RetFields;
     vector<TLV> m_RetData;
};


//����ƽ̨update��Ӧ��
typedef BaseDpRsp DpUpdateRsp;

//����ƽ̨insert��Ӧ��
typedef BaseDpRsp DpInsertRsp;

//����ƽ̨delete��Ӧ��
typedef BaseDpRsp DpDeleteRsp;

//����ƽ̨count��Ӧ��
typedef BaseDpRsp DpCountRsp;

//����ƽ̨delta��Ӧ��
typedef BaseDpRsp DpDeltaRsp;



//���������
class DpComboReq : public BaseDpReq
{
public:
    DpComboReq()
    {
        m_Timeout = 0;
        m_Header.main_cmd = COMBO_REQUEST_MAIN_CMD;
    }
    virtual ~DpComboReq(){}
    
    void SetTimeout(unsigned short t){m_Timeout = t;}

    /*
    *���ܣ��������ݰ�
    *OUT req:���ɵ����ݰ�
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   virtual int MakeReq(string &req)
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
    *IN pCookie:cookie����. 
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   virtual int MakeReq(char *buf, unsigned int buf_len)
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
    *���ܣ����һ������ƽ̨�����
    *IN jcache_server_type:Ҫ���͵��ĸ�jcache
    *IN p:�����
    *����ֵ����
    */
    void Add(unsigned int jcache_server_type, BaseDpReq *p)
    {
        m_ComboReq.push_back(std::make_pair<BaseDpReq *, unsigned int>(p,jcache_server_type));
    }
    

protected:
    template<class PACKER>
    void MakeReq(PACKER &pk)
    {
        pk.PackWord(m_Timeout);
        unsigned short req_num = m_ComboReq.size();
        pk.PackWord(req_num);     //�������ʽ��Ŀ

        unsigned int table_id = 0;
        BaseDpReq *pReq = 0;
        for(unsigned short i = 0; i < req_num; i++)
        {
            table_id = m_ComboReq[i].second;
            pReq = m_ComboReq[i].first;
            unsigned int route_key = pReq->GetRouteKey();
            pk.PackDWord(table_id);
            pk.PackDWord(route_key);
            string req;
            pReq->MakeReq(req);
            unsigned int req_size = req.size();
            pk.PackDWord(req_size);
            pk.PackBinary(req.data(), req_size);
        }

        PackCookie(pk);
    }

    typedef std::pair<BaseDpReq *, unsigned int> item_type;
    vector<item_type> m_ComboReq;    
    unsigned short m_Timeout;
};

//�����Ӧ��
class DpComboRsp : public BaseDpRsp
{
public:
    DpComboRsp(){}
    virtual ~DpComboRsp();

    /*
    *���ܣ���������ƽ̨����Ӧ
    *IN buf:��Ӧ��
    *IN buf_len:��Ӧ���ĳ���
    *����ֵ��0�ɹ�����0ʧ��
    */
    virtual int ParseRsp(const char *buf, unsigned int buf_len);

    /*
    *���ܣ�ȡ����е�ĳһ����Ӧ
    *OUT cmd:��Ӧ��������
    *IN idx:�ڼ�����Ӧ��
    *����ֵ����Ӧ��. NULLʧ��
    */
    BaseDpRsp *GetRsp( unsigned short &cmd, unsigned int idx)
    {
        if(idx >= m_RecordNum)
        {
            return NULL;
        }
        item_type &t = m_ComboRsp[idx];
        cmd = t.second;
        return t.first;                    
    }

    /*
    *����:�Ƿ����������󶼳ɹ��ˡ�
    *����ֵ:true:�ǣ���false:����
    */
    bool IsAllCmdOK()
    {
        unsigned short cmd=0;
        for(unsigned int i = 0; i < m_RecordNum; i++)
        {
            BaseDpRsp *p = GetRsp(cmd, i);
            if(!p)
            {
                return false;
            }
            if(p->GetRetCode() != 0)
            {
                return false;
            }
        }
        return true;
    }
protected:
    typedef std::pair<BaseDpRsp *, unsigned int> item_type;
    vector<item_type> m_ComboRsp;

};


//ģ������������
class DpCondFindReq:public BaseDpReq
{
public:

    /*
    *����:���캯��
    */
    DpCondFindReq():m_nLimitNumber(0),m_nStatus(0){m_Header.main_cmd = COND_FIND_MAIN_CMD;}

    /*
    *����:��������
    */
    virtual ~DpCondFindReq(){}

    /*
    *���ܣ��������ݰ�
    *OUT req:���ɵ����ݰ�
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
   virtual int MakeReq(string &req);

    /*
    *���ܣ��������ݰ�
    *OUT buf:���ɵ����ݰ�
    *IN buf_len:buf�Ĵ�С
    *����ֵ�����ɵ����ݰ��Ĵ�С��<=0����
    */
    virtual int MakeReq(char *buf, unsigned int buf_len);

    

    /*
    *���ܣ��������
    *IN fid:�ֶ�ID
    *IN op_type:��������
    *IN TLV:����ֵ. 
    *����ֵ����
    *��ע:���ҽ������ֻ��UIN
    */
    void AddCond(unsigned int fid, unsigned char optype, const TLV &v){m_CondSet.Add(fid,optype,v);}

    /*
    *����:�������
    *IN limit_number:��󷵻صļ�¼��
    *IN status:����״̬��0-���У�1-���ߣ�2-����
    *����ֵ:��
    */
    void AddCond(unsigned short limit_number,unsigned short status){m_nLimitNumber = limit_number;m_nStatus = status;}


  private:
    DpFieldValueSet m_CondSet;
    unsigned short m_nLimitNumber;
    unsigned short m_nStatus;

    
};

class DpCondFindRsp:public BaseDpRsp
{
public:
    DpCondFindRsp(){}
    virtual ~DpCondFindRsp(){}

    /*
    *���ܣ���������ƽ̨����Ӧ
    *IN buf:��Ӧ��
    *IN buf_len:��Ӧ���ĳ���
    *����ֵ��0�ɹ�����0ʧ��
    */
    virtual int ParseRsp(const char *buf, unsigned int buf_len);

    /*
    *����:ȡ����״̬�Ľ����
    *����ֵ:���ߵĽ����
    */
    unsigned int GetOnlineRecordNum(){return m_RetOnLineUIN.size();}

     /*
    *����:ȡ����״̬�Ľ����
    *����ֵ:���ߵĽ����
    */
    unsigned int GetOffLineRecordNum(){return m_RetOffLineUIN.size();}

    /*
    *����:ȡ�����û���UIN
    *����˵��
    *index:�����û��б��±�
    *����ֵ:uin
    */
    unsigned int GetOnLineUin(unsigned int index){return m_RetOnLineUIN[index];}

    /*
    *����:ȡ�����û���UIN
    *����˵��
    *index:�����û��б��±�
    *����ֵ:uin
    */
    unsigned int GetOffLineUin(unsigned int index){return m_RetOffLineUIN[index];}

private:
    
     vector<unsigned int> m_RetOnLineUIN;//�����û������
     vector<unsigned int> m_RetOffLineUIN;//�����û������
    
};


//ģ������Ӧ����
};


#if 0
//for example:
using namespace protocol;
int GetBlogArticle(unsigned int UIN, unsigned int aid)
{
        string pack;
        bool is_primary_key = true;
        unsigned short cond_num = 2;
        ConditionExpression exp[2];

        exp[0].field_id = FIELD_ID_BLOG_ARTICLE_UIN;
        exp[0].op_code = OP_EQUAL;
        exp[0].data.type = TLV_DWORD;
        exp[0].data.length = 4;
        exp[0].data.int_value = UIN;

        exp[1].field_id = FIELD_ID_BLOG_ARTICLE_AID;
        exp[1].op_code = OP_EQUAL;
        exp[1].data.type = TLV_DWORD;
        exp[1].data.length = 4;
        exp[1].data.int_value = aid;

        const char *logic_expression = "$1 AND $2";
        unsigned short ret_field_num = 2;
        unsigned int ret_filedid_set[2];
        ret_filedid_set[0] = FIELD_ID_BLOG_ARTICLE_TITLE;
        ret_filedid_set[1] = FIELD_ID_BLOG_ARTICLE_CONTENT;

        static unsigned int seq = 0;

        int ret = DataPlatformProtocol::PackQueryReq(pack, 
            is_primary_key, cond_num, exp, logic_expression, 
            ret_field_num, ret_filedid_set, seq++);
        if(ret != RET_OK)
        {
                return -1;
        }

        send(fd, pack.data(), pack.size(), 0);
        return 0;
};
#endif

#endif


