# chatServer
尝试学习搭建一个可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端 基于muduo实现

编译方式
cd build
rm -rf *
cmake ..
make

需要nginx tcp的负载均衡

# 8.9 
更新了好友发消息功能，不能给不是好友的用户直接发消息，不能给不存在的用户发消息
添加了mysql和redis连接池代码，但还未引入到框架内
