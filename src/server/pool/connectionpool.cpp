#include "connectionpool.hpp"
#include "db.h"
#include "redis.hpp"
#include <iostream>

// 构造函数
template <typename T>
ConnectionPool<T>::ConnectionPool(ConnectionFactory factory, ConnectionValidator validator, size_t initialSize, size_t maxSize, std::chrono::seconds maxIdleTime) : _factory(factory), _validator(validator), _initialSize(initialSize), _maxSize(maxSize), _maxIdleTime(maxIdleTime), _activeConnections(0), _shutdown(false), _cleanupThread(&ConnectionPool::cleanupWorker, this)
{
    initializeConnections();
}

// 析构函数
template <typename T>
ConnectionPool<T>::~ConnectionPool()
{
    shutdown();
}

// 初始化连接
template <typename T>
void ConnectionPool<T>::initializeConnections()
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (size_t i = 0; i < _initialSize; i++)
    {
        auto conn = createConnection();
        if (conn)
            _availableConnections.emplace(conn);
    }
}

// 创建连接
template <typename T>
typename ConnectionPool<T>::ConnectionPtr ConnectionPool<T>::createConnection()
{
    try
    {
        return _factory();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to create connection: " << e.what() << std::endl;
        return nullptr;
    }
}

// 获取连接
template <typename T>
typename ConnectionPool<T>::ConnectionPtr ConnectionPool<T>::getConnection()
{
    std::unique_lock<std::mutex> lock(_mutex);

    // 等待可用连接，或创建新连接
    _condition.wait(lock, [this]
                    { return !_availableConnections.empty() || _activeConnections.load() < _maxSize || _shutdown.load(); });

    if (_shutdown.load())
        return nullptr;

    ConnectionPtr conn = nullptr;

    // 尝试从池中获取连接
    while (!_availableConnections.empty())
    {
        auto wrapper = _availableConnections.front();
        _availableConnections.pop();

        // 验证连接是否有效
        if (_validator(wrapper.connection))
        {
            conn = wrapper.connection;
            break;
        }
    }

    // 如果没有可用连接，创建新连接
    if (!conn && _activeConnections.load() < _maxSize)
        conn = createConnection();

    if (conn)
        _activeConnections++;

    return conn;
}

// 归还连接
template <typename T>
void ConnectionPool<T>::returnConnection(ConnectionPtr conn)
{
    if (!conn || _shutdown.load())
        return;

    std::lock_guard<std::mutex> lock(_mutex);

    // 验证有效性
    if (_validator(conn))
        _availableConnections.emplace(conn);
    _activeConnections--;
    _condition.notify_one();
}

// 获取连接池状态
template <typename T>
size_t ConnectionPool<T>::getActiveConnections() const
{
    return _activeConnections.load();
}
template <typename T>
size_t ConnectionPool<T>::getIdleConnections() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _availableConnections.size();
}

// 清理
template <typename T>
void ConnectionPool<T>::cleanupIdleConnections()
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto now = std::chrono::steady_clock::now();
    std::queue<ConnectionWrapper> validConnections;

    while (!_availableConnections.empty())
    {
        auto wrapper = _availableConnections.front();
        _availableConnections.pop();

        auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - wrapper.lastUsed);

        if (idleTime < _maxIdleTime && _validator(wrapper.connection))
            validConnections.push(wrapper);
    }
    _availableConnections = std::move(validConnections);
}

// 清理
template <typename T>
void ConnectionPool<T>::cleanupWorker()
{
    while (!_shutdown.load())
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        cleanupIdleConnections();
    }
}

// 关闭连接池
template <typename T>
void ConnectionPool<T>::shutdown()
{
    _shutdown.store(true);
    _condition.notify_all();
    if (_cleanupThread.joinable())
        _cleanupThread.join();

    std::lock_guard<std::mutex> lock(_mutex);
    while (!_availableConnections.empty())
        _availableConnections.pop();
}

// RAII连接管理器实现
template <typename T>
ConnectionRAII<T>::ConnectionRAII(ConnectionPool<T> &pool) : _pool(&pool), _connection(pool.getConnection()) {}

template <typename T>
ConnectionRAII<T>::~ConnectionRAII()
{
    if (_pool && _connection)
        _pool->returnConnection(_connection);
}

// 允许移动
template <typename T>
ConnectionRAII<T>::ConnectionRAII(ConnectionRAII &&other) noexcept : _pool(other._pool), _connection(std::move(other._connection))
{
    other._pool = nullptr;
}
template <typename T>
ConnectionRAII<T> &ConnectionRAII<T>::operator=(ConnectionRAII &&other) noexcept
{
    if (this != &other)
    {
        if (_pool && _connection)
            _pool->returnConnection(_connection);
        _pool = other._pool;
        _connection = std::move(other._connection);
        other._pool = nullptr;
    }
    return *this;
}

template <typename T>
std::shared_ptr<T> ConnectionRAII<T>::get() const
{
    return _connection;
}

template <typename T>
std::shared_ptr<T> ConnectionRAII<T>::operator->() const
{
    return _connection;
}

template <typename T>
T &ConnectionRAII<T>::operator*() const
{
    return *_connection;
}

template <typename T>
bool ConnectionRAII<T>::isValid() const
{
    return _connection != nullptr;
}

// 显式实例化模板类
template class ConnectionPool<MySQL>;
template class ConnectionPool<Redis>;
template class ConnectionRAII<MySQL>;
template class ConnectionRAII<Redis>;