#pragma once

#include"Socket.h"

namespace yazi
{
namespace socket
{
	class ServerSocket:public Socket
	{
	public:
		ServerSocket();
		ServerSocket(const InetAddress& servaddr);
		~ServerSocket()=default;

		void myclient(int sockfd);
		void next(InetAddress servaddr);
	};
	
}
}
