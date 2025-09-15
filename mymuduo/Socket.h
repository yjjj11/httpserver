#pragma once
#include<iostream>
#include<string>
#include<fcntl.h>
#include<arpa/inet.h>
#include<cstring>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/poll.h>
// #include"Logger.h"
#include"InetAddress.h"
#include"log.h"
using std::string;
namespace yazi
{
	namespace socket
	{
		class Socket
		{
		public:
			Socket();
			virtual ~Socket();
			Socket(int sockfd);
			bool bind (const InetAddress&  servaddr);
			bool listen(int backlog=128);
			bool connect(const string &ip,int port);
			int accept(InetAddress & clientaddr);

			int send(const char*buf,int len);
			bool send(void *buffer,const size_t size );
			bool sendfile(const string & filename, const size_t size);
			
			int recv( char* buf,int len);
			//bool recv(void* buffer,const size_t size);
			bool recvfile(const string& filename,const size_t size);
			
			void close();

			bool set_send_buffer(int size);
			bool set_recv_buffer(int size);
			bool set_linger(bool active,int seconds);
			bool set_keepalive();
			bool set_reuseaddr();
			bool set_nodelay();
			bool set_reuseport();
			int get_sockfd();
			int fd(){return m_sockfd;}
			string ip() const {return m_ip;}
			uint16_t port()const{return m_port;}
			void setipport(const string& ip,uint16_t port);
		private:
			string m_ip;
			uint16_t m_port;
			int m_sockfd;
		
		};
	}
}
