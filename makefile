# 定义源文件所在的目录
MUDUO_DIR := mymuduo
LINK :=-IDatabaseConnectionPool -lpthread -lmysqlclient -ljsoncpp -lredis++ -lhiredis
# 定义需要编译的源文件（.cpp文件）
SOURCES := main.cpp \
           $(MUDUO_DIR)/InetAddress.cpp \
           $(MUDUO_DIR)/Socket.cpp \
           $(MUDUO_DIR)/server_socket.cpp \
           $(MUDUO_DIR)/Epoll.cpp \
           $(MUDUO_DIR)/Channel.cpp \
           $(MUDUO_DIR)/EventLoop.cpp \
           $(MUDUO_DIR)/TcpServer.cpp \
           $(MUDUO_DIR)/Acceptor.cpp \
           $(MUDUO_DIR)/Connection.cpp \
           $(MUDUO_DIR)/Buffer.cpp \
           httpserver.cpp \
           $(MUDUO_DIR)/ThreadPoll.cpp \
           $(MUDUO_DIR)/Timestamp.cpp \
           DatabaseConnectionPool/MysqlConn.cpp \
           DatabaseConnectionPool/ConnectionPool.cpp \
           mymuduo/http/httprequest.cpp \
           mymuduo/http/httpresponse.cpp \
           mymuduo/http/token.cpp \
           mymuduo/API/api.cpp \

# 定义所有头文件（用于依赖检测）
HEADERS := $(MUDUO_DIR)/InetAddress.h \
           $(MUDUO_DIR)/Socket.h \
           $(MUDUO_DIR)/server_socket.h \
           $(MUDUO_DIR)/Epoll.h \
           $(MUDUO_DIR)/Channel.h \
           $(MUDUO_DIR)/EventLoop.h \
           $(MUDUO_DIR)/TcpServer.h \
           $(MUDUO_DIR)/Acceptor.h \
           $(MUDUO_DIR)/Connection.h \
           $(MUDUO_DIR)/Buffer.h \
           httpserver.h \
           DatabaseConnectionPool/MysqlConn.h \
           $(MUDUO_DIR)/ThreadPoll.h \
           $(MUDUO_DIR)/Timestamp.h \
           $(MUDUO_DIR)/log.h \
           DatabaseConnectionPool/ConnectionPool.h \
           mymuduo/http/httprequest.h \
           mymuduo/http/httpresponse.h \
           mymuduo/http/token.h \
           mymuduo/API/api.h \
           mymuduo/API/functionmap.h
# 定义目标可执行文件名
TARGET := main

all: $(TARGET)

# 让可执行文件依赖于所有源文件和头文件
$(TARGET): $(SOURCES) $(HEADERS)
	g++ -g -O0 -o $@ $(SOURCES) -I$(MUDUO_DIR)  $(LINK) -std=c++20

clean:
	rm -f $(TARGET)
