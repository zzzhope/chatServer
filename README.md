# chatServer
尝试学习搭建一个可以工作在nginx tcp负载均衡环境中的集群聊天服务器和客户端 基于muduo实现

编译方式
cd build
rm -rf *
cmake ..
make

需要nginx tcp的负载均衡
