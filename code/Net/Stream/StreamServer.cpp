#include "Core/Io/Reader.h"
#include "Core/Io/Writer.h"
#include "Core/Log/Log.h"
#include "Core/Math/MathUtils.h"
#include "Core/Thread/Thread.h"
#include "Core/Thread/ThreadManager.h"
#include "Core/Timer/Timer.h"
#include "Net/SocketAddressIPv4.h"
#include "Net/SocketStream.h"
#include "Net/TcpSocket.h"
#include "Net/Stream/StreamServer.h"

#define T_MEASURE_THROUGHPUT 0

namespace traktor
{
	namespace net
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.net.StreamServer", StreamServer, Object)

StreamServer::StreamServer()
:	m_listenPort(0)
,	m_serverThread(0)
,	m_nextId(1)
{
}

bool StreamServer::create(uint16_t listenPort)
{
	m_listenSocket = new TcpSocket();
	if (!m_listenSocket->bind(SocketAddressIPv4(listenPort)))
		return false;
	if (!m_listenSocket->listen())
		return false;

	m_listenPort = listenPort;

	m_serverThread = ThreadManager::getInstance().create(
		makeFunctor(this, &StreamServer::threadServer),
		L"Stream server"
	);
	if (!m_serverThread)
		return false;

	m_serverThread->start();

	log::info << L"Stream server @" << listenPort << L" created" << Endl;
	return true;
}

void StreamServer::destroy()
{
	if (m_serverThread)
	{
		m_serverThread->stop();
		ThreadManager::getInstance().destroy(m_serverThread);
		m_serverThread = 0;
	}

	for (std::list< Thread* >::iterator i = m_clientThreads.begin(); i != m_clientThreads.end(); ++i)
	{
		(*i)->stop();
		ThreadManager::getInstance().destroy(*i);
	}

	m_clientThreads.clear();
	m_streams.clear();
}

uint32_t StreamServer::publish(IStream* stream)
{
	uint32_t id = m_nextId;
	m_streams[id] = stream;
	if (++m_nextId == 0)
		m_nextId = 1;
	return id;
}

uint16_t StreamServer::getListenPort() const
{
	return m_listenPort;
}

TcpSocket* StreamServer::getListenSocket() const
{
	return m_listenSocket;
}

void StreamServer::threadServer()
{
	while (!m_serverThread->stopped())
	{
		if (m_listenSocket->select(true, false, false, 100) > 0)
		{
			Ref< TcpSocket > clientSocket = m_listenSocket->accept();
			if (!clientSocket)
				continue;

			Thread* clientThread = ThreadManager::getInstance().create(
				makeFunctor< StreamServer, Ref< TcpSocket > >(this, &StreamServer::threadClient, clientSocket),
				L"Stream"
			);
			if (!clientThread)
				continue;

			clientThread->start(Thread::Above);
			m_clientThreads.push_back(clientThread);
		}
		else
		{
			for (std::list< Thread* >::iterator i = m_clientThreads.begin(); i != m_clientThreads.end(); )
			{
				if ((*i)->finished())
				{
					ThreadManager::getInstance().destroy(*i);
					i = m_clientThreads.erase(i);
				}
				else
					++i;
			}
		}
	}	
}

void StreamServer::threadClient(Ref< TcpSocket > clientSocket)
{
	uint8_t buffer[65536];
	Ref< IStream > stream;
	uint32_t streamId;

	SocketStream ss(clientSocket);
	Reader rd(&ss);
	Writer wr(&ss);
	Timer timer;

	timer.start();

	double start = 0.0;
	int32_t totalRx = 0;
	int32_t totalTx = 0;
	int32_t countRx = 0;
	int32_t countTx = 0;

	Thread* currentThread = ThreadManager::getInstance().getCurrentThread();
	while (!currentThread->stopped())
	{
		int32_t result = clientSocket->select(true, false, false, 100);
		if (result == 0)
			continue;
		else if (result < 0)
			break;

		int32_t command = clientSocket->recv();
		if (command < 0)
			break;

		switch (command)
		{
		case 0x01:	// Acquire stream.
			{
				rd >> streamId;

				std::map< uint32_t, Ref< IStream > >::const_iterator i = m_streams.find(streamId);
				if (i != m_streams.end() && i->second != 0)
				{
					stream = i->second;

					uint8_t status = 0x00;
					if (stream->canRead())
						status |= 0x01;
					if (stream->canWrite())
						status |= 0x02;
					if (stream->canSeek())
						status |= 0x04;
					
					wr << status;

					start = timer.getElapsedTime();
					totalRx = 0;
					totalTx = 0;
					countRx = 0;
					countTx = 0;
				}
				else
					wr << uint8_t(0);
			}
			break;

		case 0x02:	// Release stream.
			{
				if (stream)
				{
					std::map< uint32_t, Ref< IStream > >::const_iterator i = m_streams.find(streamId);
					if (i != m_streams.end())
						m_streams.erase(i);

					double end = timer.getElapsedTime();

#if T_MEASURE_THROUGHPUT
					log::info << L"Stream " << streamId << Endl;
					log::info << L"RX " << totalRx << L" -- " << int32_t(totalRx / (end - start)) << L" bytes/s (" << countRx << L")" << Endl;
					log::info << L"TX " << totalTx << L" -- " << int32_t(totalTx / (end - start)) << L" bytes/s (" << countTx << L")" << Endl;
#endif

					stream = 0;
					streamId = 0;
				}
			}
			break;

		case 0x03:	// Close
			{
				if (stream)
				{
					stream->close();
					wr << bool(true);
				}
				else
					wr << bool(false);
			}
			break;

		case 0x04:	// Tell
			{
				if (stream)
					wr << int32_t(stream->tell());
				else
					wr << int32_t(-1);
			}
			break;

		case 0x05:	// Available
			{
				if (stream)
					wr << int32_t(stream->available());
				else
					wr << int32_t(-1);
			}
			break;

		case 0x06:	// Seek
			{
				if (stream)
				{
					int32_t origin, offset, result;
					rd >> origin;
					rd >> offset;

					result = stream->seek((IStream::SeekOriginType)origin, offset);
					wr << result;
				}
			}
			break;

		case 0x07:	// Read
			{
				if (stream)
				{
					int32_t nrequest = 0;
					rd >> nrequest;

					while (nrequest > 0)
					{
						int32_t navail = min< int32_t >(nrequest, sizeof(buffer));
						int32_t nread = stream->read(buffer, navail);

						wr << nread;
						if (nread <= 0)
							break;

						wr.write(buffer, nread);

						nrequest -= nread;
						totalRx += nread;
					}

					++countRx;
				}
			}
			break;

		case 0x08:	// Write
			{
				if (stream)
				{
					int32_t nbytes = 0;
					rd >> nbytes;

					while (nbytes > 0)
					{
						int32_t nread = min< int32_t >(nbytes, sizeof(buffer));
						int32_t nrecv = rd.read(buffer, nread);
						if (nrecv <= 0)
							break;

						stream->write(buffer, nrecv);

						totalTx += nrecv;
						nbytes -= nrecv;
					}

					++countTx;
				}
			}
			break;

		case 0x09:	// Flush
			{
				if (stream)
				{
					stream->flush();
					wr << bool(true);
				}
				else
					wr << bool(false);
			}
			break;
		}
	}
}

	}
}
