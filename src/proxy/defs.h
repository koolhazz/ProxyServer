#ifndef _DEFS_H_
#define _DEFS_H_

#define HTTP_SVR_NS_BEGIN namespace __http_svr_ns__ {
#define HTTP_SVR_NS_END }
#define USING_HTTP_SVR_NS using namespace __http_svr_ns__;

#define MAX_WEB_RECV_LEN            102400
#define MAX_ACCEPT_COUNT            256
enum CConnState
{
    CONN_IDLE,
    CONN_FATAL_ERROR,
    CONN_DATA_ERROR,
    CONN_CONNECTING,
    CONN_DISCONNECT,
    CONN_CONNECTED,
    CONN_DATA_SENDING,
    CONN_DATA_RECVING,
    CONN_SEND_DONE,
    CONN_RECV_DONE,
    CONN_APPEND_SENDING,
    CONN_APPEND_DONE
};

enum TDecodeStatus
{
    DECODE_FATAL_ERROR,
    DECODE_DATA_ERROR,
    DECODE_WAIT_HEAD,
    DECODE_WAIT_CONTENT,
    DECODE_HEAD_DONE,
    DECODE_CONTENT_DONE,
    DECODE_DONE,
    DECODE_DISCONNECT_BY_USER
};

typedef struct tagRspList
{
    char*   _buffer;
    int     _len;
    int     _sent;
    int     _type;

    struct tagRspList* _next;

}TRspList;

enum
{
	CLIENT_CMD_SYC = 0x0002,
	SERVER_CMD_SYC = 0x0001
};

enum ConnType
{
	CONN_CLINET,
	CONN_OTHER
};

//server间通信协议
#define CLIENT_PACKET  		 0x0001//用户发过来的数据包或者server发给用户的包
#define CLIENT_PACKET2		 0x0004//用户发过来的数据包或者server发给用户的包(新命令字)

#define CLIENT_CLOSE_PACKET  0x0002//用户断开连接，hall给逻辑server发的包
#define SERVER_CLOSE_PACKET  0x0003//逻辑server主动断开连接的包
#define SYS_RELOAD_CONFIG    0x0010//重新加载配置
const int SYS_RELOAD_IP_MAP = 0x0011;//重新加载ip映射
const int SYS_RESET_CONNECT_TIMER = 0x0012;//重新设置connect超时时间
const int SYS_OPEN_DEBUG = 0x000D;//设置debug日志开关

const int CLIENT_CMD_CLIENT_LOGIN = 0x0101;//用户大厅登录包
const int CMD_GET_NEW_ROOM = 0x0117;//随机场点击了准备,重新请求房间号
const int CLIENT_CMD_REQUEST_LIST_ROOM = 0x11D;//用户请求房间列表
const int CLIENT_CMD_ENTER_ROOM = 0x0125;//用户请求进入房间

const int SERVER_CMD_LOGIN_SUCCESS = 0x0201;//用户登录大厅成功
const int SERVER_CMD_KICK_OUT = 0x0203;//剔除用户
const int SERVER_CMD_ENTER_ROOM = 0x0212;//返回用户进入房间

const int SERVER_CMD_SET_IP = 0x0301;
const int CMD_GET_ROOM_LEVER_NUM = 0x0311;//获取各等级场人数

const int CMD_REQUIRE_IP_PORT = 0x0604;//请求server的ip和端口

const int CMD_SEPARATE = 0x0800;//命令分隔  分割游戏服务器命令字

const int UDP_SERVER_LOG    =0x0901;    //上报命令字日志server
const int UDP_SERVER_CLIENT_CLOSE_LOG = 0x0902;//上报用户离开大厅
const int UDP_SERVER_USER_COUNT      = 0x0903;//上报当前用户有多少人

const int CLIENT_COMMAND_LOGIN = 0x1001;//用户登录某个房间
const int SERVER_COMMAND_LOGOUT_SUCCESS = 0x2013;//用户成功退出房间

const int CLIENT_COMMAND_BREAK_TIME = 0x1016;//心跳包

const int SERVER_GET_PLAYER_COUNT   = 0x3018;  //获到进程有多少玩家

const int SERVER_COMMAND_BREAK_TIME = 0x2017;//返回心跳

const int SERVER_COMMAND_CHANGE_GAMESERVER = 0x7213;

const int SERVER_BROADCAST_INFO = 0x7050;  // broadcast system
const int SERVER_BROADCAST_INFO_NEW = 0x7054;//新的广播协议
const int CLINET_REQUEST_BROADCAST_INFO = 0x7055; //区分终端类型的广播

#endif

