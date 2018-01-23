/*
 * _webSocket.h
 *
 *  Created on: August 8, 2016
 *      Author: yankai
 */

#ifndef OpenKAI_src_IO__webSocket_H_
#define OpenKAI_src_IO__webSocket_H_

#include "../Base/common.h"
#include "../Base/_ThreadBase.h"
#include "../Protocol/Peer.h"
#include "../Script/Kiss.h"
#include "../UI/Window.h"

#define N_BUF_IO 128
#define TIMEOUT_RECV_USEC 1000

namespace kai
{

class _webSocket: public _ThreadBase
{
public:
	_webSocket();
	virtual ~_webSocket();

	bool init(void* pKiss);
	bool link(void);
	bool start(void);
	void close(void);
	void reset(void);
	bool draw(void);

	bool write(uint8_t* pBuf, int nByte);
	int  read(uint8_t* pBuf, int nByte);

private:
	bool connect(void);
	void send(void);
	void recv(void);

	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_webSocket*) This)->update();
		return NULL;
	}

public:
	string	m_strStatus;

	struct sockaddr_in m_serverAddr;
	string	m_strAddr;
	uint16_t m_port;

	bool m_bClient;
	bool m_bConnected;
	int m_webSocket;
	uint32_t m_timeoutRecv;

	int m_nBuf;
	uint8_t* m_pBuf;
	std::queue<uint8_t> m_queSend;
	std::queue<uint8_t> m_queRecv;

	pthread_mutex_t m_mutexSend;
	pthread_mutex_t m_mutexRecv;


};

}

#endif
