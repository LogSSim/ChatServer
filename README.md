# ChatServer
可以在Nginx TCP负载均衡环境中的集群聊天服务器和客户端源码 基于muduo、mysql、Redis实现，可以实现基本的注册、添加好友、创建群、添加群、一对一聊天、群聊的功能

# 准备工作(preparation)
  download 

# 编译方式(how to compile):
  cd build
  rm -rf *
  cmake..
  make
# 运行方式(how to run)
cd bin
启动服务器(activate Server)
  ./ChatServer 127.0.0.1 6000
  or ./ChatServer 127.0.0.1 6002
启动客户端(activate Client)
  ./ChatClient 127.0.0.1 6000
  or ./ChatClient 127.0.0.1 6002
