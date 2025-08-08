#ifndef MYSQL_POOL_H
#define MYSQL_POOL_H

#include "connectionpool.hpp"
#include "db.h"
#include <memory>

class MySQLPoolManager
{
public:
    using MySQLPtr = std::shared_ptr<MySQL>;
    using MySQLPool = ConnectionPool<MySQL>;
    using MySQLConnection = ConnectionRAII<MySQL>;

    // 获取单例实例
    static MySQLPoolManager &getInstance();

    // 初始化连接池
    void initialize(size_t initialSize = 5, size_t maxSize = 20);

    // 获取连接池
    MySQLPool &getPool();

    // 获取连接（RAII方式）
    MySQLConnection getConnection();

    // 关闭连接池
    void shutdown();

private:
    MySQLPoolManager() = default;
    ~MySQLPoolManager() = default;

    // 禁止拷贝赋值
    MySQLPoolManager(const MySQLPoolManager &) = delete;
    MySQLPoolManager &operator=(const MySQLPoolManager &) = delete;

    // 连接工厂函数
    MySQLPtr createMySQLConnection();

    // 连接验证函数
    bool validateMySQLConnection(const MySQLPtr &conn);

    std::unique_ptr<MySQLPool> _pool;
    bool _initialized = false;
};

// 便捷的全局函数
MySQLPoolManager::MySQLConnection getMySQLConnection();
void initMySQLPool(size_t initialSize = 5, size_t maxSize = 20);

#endif