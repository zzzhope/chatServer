#include "redis_pool.hpp"
#include <iostream>

// 获取单例实例
RedisPoolManager &RedisPoolManager::getInstance()
{
    static RedisPoolManager instance;
    return instance;
}

// 初始化连接池
void RedisPoolManager::initialize(size_t initialSize, size_t maxSize)
{
    if (_initialized)
    {
        std::cout << "Redis connection pool already initialized" << std::endl;
        return;
    }

    auto factory = [this]() -> RedisPtr
    { return createRedisConnection(); };
    auto validator = [this](const RedisPtr &conn) -> bool
    { return validateRedisConnection(conn); };

    _pool = std::make_unique<RedisPool>(factory, validator, initialSize, maxSize);
    _initialized = true;

    std::cout << "Redis connection pool initialized with " << initialSize << " initial connections, max " << maxSize << " connections" << std::endl;
}

// 获取连接池
RedisPoolManager::RedisPool &RedisPoolManager::getPool()
{
    if (!_initialized || !_pool)
    {
        throw std::runtime_error("Redis connection pool not initialized");
    }
    return *_pool;
}

// 获取连接（RAII方式）
RedisPoolManager::RedisConnection RedisPoolManager::getConnection()
{
    return RedisConnection(getPool());
}

// 关闭连接池
void RedisPoolManager::shutdown()
{
    if (_pool)
    {
        _pool->shutdown();
        _pool.reset();
        _initialized = false;
        std::cout << "Redis connection pool shutdown" << std::endl;
    }
}

// 连接工厂函数
RedisPoolManager::RedisPtr RedisPoolManager::createRedisConnection()
{
    auto redis = std::make_shared<Redis>();
    if (redis->connect())
        return redis;
    return nullptr;
}

// 连接验证函数
bool RedisPoolManager::validateRedisConnection(const RedisPtr &conn)
{
    if (!conn)
        return false;

    // 使用简单的publish测试来验证连接
    // 因为原始Redis类没有ping方法，我们用一个无害的操作来测试
    try
    {
        // 发布一个测试消息到一个测试频道
        return conn->publish(999999, "ping");
    }
    catch (const std::exception &e)
    {
        // 记录具体的错误信息
        std::cerr << "Redis validation error: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        // 未知问题
        std::cerr << "Redis validation unknown error" << std::endl;
        return false;
    }
}

// 便捷的全局函数
RedisPoolManager::RedisConnection getRedisConnection()
{
    return RedisPoolManager::getInstance().getConnection();
}
void initRedisPool(size_t initialSize, size_t maxSize)
{
    RedisPoolManager::getInstance().initialize(initialSize, maxSize);
}