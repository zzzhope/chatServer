#include "mysql_pool.hpp"
#include <iostream>

MySQLPoolManager &MySQLPoolManager::getInstance()
{
    static MySQLPoolManager instance;
    return instance;
}

// 初始化连接池
void MySQLPoolManager::initialize(size_t initialSize, size_t maxSize)
{
    if (_initialized)
    {
        std::cout << "MySQL connection pool already initialized" << std::endl;
        return;
    }

    auto factory = [this]() -> MySQLPtr
    { return createMySQLConnection(); };
    auto validator = [this](const MySQLPtr &conn) -> bool
    { return validateMySQLConnection(conn); };

    _pool = std::make_unique<MySQLPool>(factory, validator, initialSize, maxSize);
    _initialized = true;

    std::cout << "MySQL connection pool initialized with " << initialSize << " initial connections, max " << maxSize << " connections" << std::endl;
}

// 获取连接池
MySQLPoolManager::MySQLPool &MySQLPoolManager::getPool()
{
    if (!_initialized || !_pool)
    {
        throw std::runtime_error("MySQL connection pool not initialized");
    }
    return *_pool;
}

// 获取连接（RAII方式）
MySQLPoolManager::MySQLConnection MySQLPoolManager::getConnection()
{
    return MySQLConnection(getPool());
}

// 关闭连接池
void MySQLPoolManager::shutdown()
{
    if (_pool)
    {
        _pool->shutdown();
        _pool.reset();
        _initialized = false;
        std::cout << "MySQL connection pool shutdown" << std::endl;
    }
}

// 连接工厂函数
MySQLPoolManager::MySQLPtr MySQLPoolManager::createMySQLConnection()
{
    auto mysql = std::make_shared<MySQL>();
    if (mysql->connect())
        return mysql;
    return nullptr;
}

// 连接验证函数
bool MySQLPoolManager::validateMySQLConnection(const MySQLPtr &conn)
{
    if (!conn)
        return false;

    // 简单的查询测试来验证连接
    try
    {
        MYSQL_RES *res = conn->query("SELECT 1");
        if (res)
        {
            mysql_free_result(res);
            return true;
        }
        return false;
    }
    catch (const std::exception &e)
    {
        // 记录具体的错误信息
        std::cerr << "MySQL validation error: " << e.what() << std::endl;
        return false;
    }
    catch (...)
    {
        // 未知问题
        std::cerr << "MySQL validation unknown error" << std::endl;
        return false;
    }
}

// 便捷的全局函数
MySQLPoolManager::MySQLConnection getMySQLConnection()
{
    return MySQLPoolManager::getInstance().getConnection();
}
void initMySQLPool(size_t initialSize, size_t maxSize)
{
    MySQLPoolManager::getInstance().initialize(initialSize, maxSize);
}