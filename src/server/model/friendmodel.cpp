#include "friendmodel.hpp"
#include "db.h"
#include <memory>

// 添加好友关系
void FriendModel::insert(int userId, int friendId)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend values('%d','%d')", userId, friendId);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表
vector<User> FriendModel::query(int userId)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select u.id,u.name,u.state from user as u inner join friend as f on f.friendid = u.id where f.userid=%d", userId);

    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> resGuard(res, mysql_free_result);
        if (res != nullptr)
        {
            // 把userid用户的所有好友信息放入vec中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            return vec;
        }
    }
    return vec; // 返回空vector表示没有好友信息
}

// 查询是否为好友
bool FriendModel::isFriend(int userId, int friendId)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from friend where userid=%d and friendid=%d", userId, friendId);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> resGuard(res, mysql_free_result);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            return row != nullptr; // 如果有记录则为好友
        }
    }
    return false;
}