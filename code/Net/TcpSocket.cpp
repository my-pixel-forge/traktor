#include "Net/TcpSocket.h"
#include "Net/SocketAddressIPv4.h"
#include "Net/SocketAddressIPv6.h"

namespace traktor
{
	namespace net
	{

T_IMPLEMENT_RTTI_CLASS(L"traktor.net.TcpSocket", TcpSocket, Socket)

TcpSocket::TcpSocket()
:	Socket()
{
}

TcpSocket::TcpSocket(SOCKET socket_)
:	Socket(socket_)
{
}

bool TcpSocket::bind(const SocketAddressIPv4& socketAddress)
{
	struct sockaddr_in local = socketAddress.getSockAddr();

	if (m_socket == INVALID_SOCKET)
	{
		m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_socket == INVALID_SOCKET)
			return false;
	}

	if (::bind(m_socket, (sockaddr *)&local, sizeof(local)) < 0)
		return false;

	return true;
}

bool TcpSocket::bind(const SocketAddressIPv6& socketAddress)
{
#if !defined(_PS3)
	const addrinfo* info = socketAddress.getAddrInfo(SOCK_STREAM);
	if (!info)
		return false;

	if (m_socket == INVALID_SOCKET)
	{
		m_socket = ::socket(info->ai_family, SOCK_STREAM, info->ai_protocol);
		if (m_socket == INVALID_SOCKET)
			return false;
	}

#if !defined(WINCE)
	if (info->ai_family == AF_INET6)
	{
		int on = 1;
		::setsockopt(m_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&on, sizeof(on));
	}
#endif

	if (::bind(m_socket, (sockaddr *)info->ai_addr, info->ai_addrlen) < 0)
		return false;

	return true;
#else
	return false;
#endif
}

bool TcpSocket::connect(const SocketAddressIPv4& socketAddress)
{
	struct sockaddr_in remote = socketAddress.getSockAddr();

	if (m_socket == INVALID_SOCKET)
	{
		m_socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (m_socket == INVALID_SOCKET)
			return false;
	}

	if (::connect(m_socket, (sockaddr *)&remote, sizeof(remote)) < 0)
		return false;

	return true;
}

bool TcpSocket::connect(const SocketAddressIPv6& socketAddress)
{
#if !defined(_PS3)
	const addrinfo* info = socketAddress.getAddrInfo(SOCK_STREAM);
	if (!info)
		return false;

	if (m_socket == INVALID_SOCKET)
	{
		m_socket = ::socket(info->ai_family, SOCK_STREAM, info->ai_protocol);
		if (m_socket == INVALID_SOCKET)
			return false;
	}

	if (::connect(m_socket, (sockaddr *)info->ai_addr, info->ai_addrlen) < 0)
		return false;

	return true;
#else
	return false;
#endif
}

bool TcpSocket::listen()
{
	return ::listen(m_socket, SOMAXCONN) == 0;
}

Ref< TcpSocket > TcpSocket::accept()
{
	struct sockaddr_in in;
	SOCKET client;

	socklen_t len = sizeof(in);
	if ((client = ::accept(m_socket, (struct sockaddr *)&in, &len)) == INVALID_SOCKET)
		return 0;

	return new TcpSocket(client);
}

Ref< SocketAddress > TcpSocket::getLocalAddress()
{
	return 0;
}

Ref< SocketAddress > TcpSocket::getRemoteAddress()
{
	struct sockaddr_in in;

	socklen_t len = sizeof(in);
	if (getpeername(m_socket, (struct sockaddr *)&in, &len) != 0)
		return 0;

	return new SocketAddressIPv4(in);
}

	}
}
