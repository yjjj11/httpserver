#include"server_socket.h"
using namespace yazi::socket;
ServerSocket::ServerSocket(const InetAddress& servaddr):Socket()
{
//	set_non_blocking();
    set_send_buffer(65536);//设置发送缓冲区大小
	set_reuseaddr();
    set_nodelay();
    set_reuseport();
    set_keepalive();
    bind(servaddr);
    listen();
}
ServerSocket::ServerSocket():Socket()
{
//	set_non_blocking();
	set_reuseaddr();
    set_nodelay();
    set_reuseport();
    set_keepalive();
}

void ServerSocket::next(InetAddress servaddr)
{
    bind(servaddr);
    listen();
}
