#ifndef PUBLIC_H
#define PUBLIC_H

/*
server和client的公共文件
*/

enum EnMsgType
{
    LOGIN_MSG = 1,      // 登录消息
    LOGIN_MSG_ACK,      // 登录响应消息
    REG_MSG,            // 注册消息
    REG_MSG_ACK,        // 注册响应消息
    ONE_CHAT_MSG,       // 聊天消息
    ONE_CHAT_MSG_ACK,   // 聊天响应信息
    ADD_FRIEND_MSG,     // 添加好友消息
    ADD_FRIEND_MSG_ACK, // 添加好友响应消息

    CREATE_GROUP_MSG, // 创建群组
    ADD_GROUP_MSG,    // 加入群组
    GROUP_CHAT_MSG,   // 群聊天

    CHANGE_PWD_MSG,     // 修改密码
    CHANGE_PWD_MSG_ACK, // 修改密码响应消息

    LOGOUT_MSG, // 注销消息
};

#endif