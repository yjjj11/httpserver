all: client tcpepoll

# 客户端编译规则，添加头文件依赖
client: client.cpp InetAddress.cpp InetAddress.h
	g++ -g -o client client.cpp InetAddress.cpp

# 服务器编译规则，明确添加所有相关头文件
tcpepoll: tcpepoll.cpp InetAddress.cpp Socket.cpp server_socket.cpp \
          Socket.h server_socket.h InetAddress.h Epoll.cpp Channel.cpp \
		  EventLoop.h EventLoop.cpp TcpServer.h TcpServer.cpp Acceptor.h Acceptor.cpp \
		  Connection.h Connection.cpp Buffer.h Buffer.cpp EchoServer.h EchoServer.cpp \
		  ThreadPoll.h ThreadPoll.cpp Timestamp.h Timestamp.cpp
	g++ -g -O0 -o tcpepoll tcpepoll.cpp InetAddress.cpp Socket.cpp server_socket.cpp \
	Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp \
	Buffer.cpp EchoServer.cpp ThreadPoll.cpp Timestamp.cpp -lpthread 

clean:
	rm -f client tcpepoll