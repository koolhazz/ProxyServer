#ifndef __SOCKETIMPL_H__
#define __SOCKETIMPL_H__

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef WIN32 //for windows nt/2000/xp

	#include <time.h>
	#include <signal.h>
	#ifndef _WINSOCKAPI_ 
		#pragma message("Including winsock2.h")
		#include <winsock2.h>
	#endif

	#pragma comment(lib,"ws2_32.lib")
#else //for unix
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/signal.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#if _AIX  //for ibm aix
		#include <sys/libcsys.h>
	#elif __HP_aCC //for hp_ux
	#else	//here for sun solaris
	#endif

	//����������ת��ʱͬWindows���ּ���
	typedef int SOCKET;
#endif

#ifdef WIN32
#define INIT_SOCKET_ENV static int volatile _g_socketFlag=SOCKET_OS::InitSocketEnv();
#define CLEAN_SOCKET_ENV
#else
#define INIT_SOCKET_ENV
#define CLEAN_SOCKET_ENV
#endif

#ifndef WIN32
	typedef struct sockaddr_in SOCKADDR_IN;
	typedef struct sockaddr    SOCKADDR;
#endif

typedef struct
{
	char LocalIP[32];
	char RemoteIP[32];
	int  LocalPort;
	int  RemotePort;
}SOCKET_INFO, *LPSOCKET_INFO;

class SOCKET_OS
{
public:
	static const char *PROTO_UDP;
	static const char *PROTO_TCP;

	/**
	 * ��ʼ��Socket������ֻ����windows
	 * ϵͳ�²��õõ���
	 */
	static int InitSocketEnv();

	/**
	 * ��ʼ��sockaddr_in���ݿ�ṹ�����ݲ���host��service
	 * ���sockaddr_in��sin_addr��sin_port��
	 */
	static int BuildSockAddr(SOCKADDR_IN *addr, 
							 const char *host,
							 int port);

	/**
	 * ���ݷ������Ʋ�ѯ�˿ں�
	 * ����ϵͳ�ļ�/etc/services�в�ѯ�����Ӧ�Ķ˿ں�
	 * ����ֵ��
	 *     <0��û�в�ѯ��
	 *     >0���˿ں�
	 */ 
	static int GetPortByName(const char *service, const char *proto = PROTO_TCP);

	/**
	 * ��ȡSocket����Ϣ���������غ�Զ�˵���Ϣ
	 * ����˵����
	 *     sockfd��Socket���
	 *     sinfo�� Socket��Ϣ�ṹ��ַ
	 * 
	 * ����ֵ��
	 *     =0�����óɹ�
	 *     !=0��ʧ��
	 */
	static int GetSocketInfo(int sockfd, LPSOCKET_INFO sinfo);

	/**
	 *  ����dlen���ֽڵ�Socket��
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *      sockfd:  Socket���
	 *      data:    Ҫд������ݿ�
	 *      dlen:    ���ݿ�ĳ���
	 *      timeout: ��ʱ����
	 *  ����ֵ��
	 *    >0: �ɹ����͵��ֽ���
	 *     0: ��ʱ��д����ֽ���������dlen��
	 *    <0: �������
	 */
	static int SendBytes(int sockfd, const void *data, int dlen, int timeout=-1);

	/**
	 *  ��ָ��ʱ���ڴ�Socket����dlen���ֽڣ�
	 *  ͬʱ��ʵ�ʶ������ֽ�������dlen��
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *      sockfd:  Socket���
	 *      data:    �����ڴ�
	 *      dlen:    ���ݿ�ĳ���
	 *      timeout: ��ʱ����
	 *  ����ֵ��
	 *    >0:  �ɹ���ȡ���ֽ���
	 *     0:  ��ʱ��ʵ�ʶ�ȡ���ֽ��������dlen��
	 *    <0:  �������
	 */
	static int RecvBytes(int sockfd, void *data, int dlen, int timeout=-1);

	/**
	 *  �ر�Socket
	 *  ��Ӧ������TCP & UDP
	 */
	static void CloseSocket(int sockfd);

	/**
	 *  �����ر�socket��ʵ��������������
	 *  Linger�ȴ�ʱ��Ϊ0��Ȼ���ڵ���CloseSocket
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *      sockfd��Socket���
	 */
	static void CloseImmediate(int sockfd);


	/**
	 * �ر�Socket��
	 */
	static void Shutdown(int socketfd);

	/**
	 * ����Socket,�ɹ����ش�����Socket�����<0����ʧ�ܡ�
	 */
	static int CreateTCPSocket();
	static int CreateUDPSocket();

	/**
	 *  ��Socket��ָ���˿ں͵�ַ
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *     sockfd��Socket���
     *     port��  Ҫ�󶨵�Socket�˿ں�
	 *     addr��  �󶨵�ַ��Ĭ��ΪANY
	 *  ����ֵ��
	 *    >0������Socket���
	 *    <0������ʧ��
	 */
	static int Bind(int sockfd, int port, const char *addr=0);

	/**
	 * ���������˿ڣ��˿ں�Ϊport������Tcp����
	 * ��Ӧ������TCP only
	 * ����˵����
	 *     port�������˿�
	 *     backflag���������д�С
	 *     addr���󶨵�ַ��Ĭ��ΪANY
	 * ����ֵ��
	 *    >0������Listener Socket���
	 *    <0������ʧ��
	 */
	static int CreateListener(int port, const char *addr=0, int backflag=20);

	/**
	 * �ȴ��ͻ��˵�����
	 * ��Ӧ������TCP & UDP
	 * ����˵����
	 *    timeout����ʱ����λ�����룩��С�����ʾ������ʱ
	 * ����ֵ��
	 *    <0������ϵͳ����acceptʧ��
	 *    >0���ͻ���socket���Ӿ��
	 *    =0���ȴ����ӳ�ʱ
	 */
	static int Accept(int sockfd);
	static int AcceptEx(int sockfd,int timeout=-1);

	/**
	 * ���ӵ�������
	 * ��Ӧ������TCP & UDP��ͨ������
	 * ����˵����
	 *     sockfd��Socket���
	 *     addr��  �Ѿ���ʼ���õ�struct sockaddr_in
	 * ����ֵ��
	 *     >0���ɹ��������Ӻ��Socket���
	 *     <0������ʧ�ܣ�
	 * 
	 */
	static int Connect(int sockfd, SOCKADDR_IN *addr);
	static int TCPConnect(const char *host, int port);
	static int UDPConnect(const char *host, int port);


	/**
	 *  ����Socket���ò�����
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *     opt=true  ���ö˿���������
	 *     opt=false ���ö˿ڲ���������
	 */
	static void SetReuseOption(int sockfd, bool opt);

	/**
	 *  ����Socket Linger����
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *      opt=true : enable linger  =false disable linger
	 *      timeout : �����ͻ��廹������ʱ����Socket����������ʱ��
	 */
	static void SetLingerOption(int sockfd, bool opt, int timeout=0);

	/**
	 *  �������������ȴ�Socket��д�¼�����selectʵ��
	 *  ��Ӧ������TCP & UDP
	 *  ����˵����
	 *      sockfd�� Socket���
	 *      timeout����ʱ����
	 *  ���ؽ����
	 *      >0���ȵ��������¼�
	 *      =0���ȴ���ʱ
	 *      <0���������
	 */
	static int WaitForW(int sockfd, int timeout);
	static int WaitForR(int sockfd, int timeout);


	/**
	 * ��Socket��ȡ����
	 * ��Ӧ������TCP & UDP
	 * ����˵����
	 *     sockfd�� Socket���
	 *     buf��    �ڴ�
	 *     buf_len���ڴ��������
	 * ���ؽ����
	 *     ���ض����ֽ���
	 *     ������󷵻ؽ��<0
	 */
	static int Send(int sockfd, void *buf, int buf_len);

	/**
	 * д���ݵ�ָ��Socket
	 * ��Ӧ������ TCP & UDP
	 * ����˵����
	 *     sockfd��  Socket���
	 *     buf��     �����ڴ�
	 *     data_len�����ݴ�С
	 * ���ؽ����
	 *     ����д����ֽ���
	 *     ������󷵻ؽ��<0
	 */
	static int Recv(int sockfd, void *buf, int data_len);


	static int Select(int width, fd_set *rfds, fd_set *wfds=0, fd_set *efds=0, int timeout=-1);

	/////////////////////////////////////////////////////////////////
	// ����ķ���ֻ��ӦUPD Socket
	////////////////////////////////////////////////////////////////

	/**
	 * ��socket��ȡ���ݣ���װAPI recvfrom
	 * ����ֵ˵����
	 *     ���ض�ȡ�����ֽ�������ʧ��ʱ���ؽ��С��0��
	 */
	static int RecvFrom(int sockfd, void *pbuf, int buf_len, SOCKADDR_IN *addr_in, int timeout);

	/**
	 * д���ݿ�鵽Socket����װAPI sendto
	 * ����ֵ:
	 *     ����д�ɹ����ֽ�����дʧ��ʱ�Żؽ��С��0
	 * 
	 */
	static int SendTo(int sockfd, const void *pBuf, int data_len, SOCKADDR_IN *addr, int timeout);

	static int GetHostName(char *host_name, int name_len);

	static int GetLastError();
	static char *GetErrorMsg(int err, char *szMsg, int len);
	static void SetLastError(int err);
};


#endif /* __SOCKETIMPL_H__ */
