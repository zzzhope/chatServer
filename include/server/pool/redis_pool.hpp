#ifndef REDIS_POOL_H
#define REDIS_POOL_H

#include "connectionpool.hpp"
#include "redis.hpp"
#include <iostream>

class RedisPoolManager
{
public:
    using RedisPtr = std::shared_ptr<Redis>;
    using RedisPool = ConnectionPool<Redis>;
    using RedisConnection = ConnectionRAII<Redis>;

    // 获取单例实例
    static RedisPoolManager &getInstance();

    // 初始化连接池
    void initialize(size_t initialSize = 3, size_t maxSize = 10);

    // 获取连接池
    RedisPool &getPool();

    // 获取连接（RAII方式）
    RedisConnection getConnection();

    // 关闭连接池
    void shutdown();

private:
    RedisPoolManager() = default;
    ~RedisPoolManager() = default;

    // 禁止拷贝赋值
    RedisPoolManager(const RedisPoolManager &) = delete;
    RedisPoolManager &operator=(const RedisPoolManager &) = delete;

    // 连接工厂函数
    RedisPtr createRedisConnection();

    // 连接验证函数
    bool validateRedisConnection(const RedisPtr &conn);

    std::unique_ptr<RedisPool> _pool;
    bool _initialized = false;
};

// 便捷的全局函数
RedisPoolManager::RedisConnection getRedisConnection();
void initRedisPool(size_t initialSize = 3, size_t maxSize = 10);

#endif