#ifndef BOYAA_ENCRYPT_DECRYPT_H
#define BOYAA_ENCRYPT_DECRYPT_H

#include "ICHAT_PacketBase.h"

class CEncryptDecrypt
{
public:
	CEncryptDecrypt(void);
	~CEncryptDecrypt(void);

public:
	WORD EncryptBuffer(NETOutputPacket *pPacket);	// ��������
	int  DecryptBuffer(NETInputPacket *pPacket);	// ��������

private:
	BYTE MapRecvByte(BYTE const cbData);
	BYTE MapSendByte(BYTE const cbData); 
private:
	static BYTE	m_SendByteMap[256];		//�ֽ�ӳ���
	static BYTE	m_RecvByteMap[256];		//�ֽ�ӳ���
};
#endif

