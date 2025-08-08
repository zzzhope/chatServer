#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include "db.h"
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>

template <typename T>
class ConnectionPool
{
public:
    using ConnectionPtr = std::shared_ptr<T>;
    using ConnectionFactory = std::function<ConnectionPtr()>;
    using ConnectionValidator = std::function<bool(const ConnectionPtr &)>;

    // 构造函数
    ConnectionPool(ConnectionFactory factory, ConnectionValidator validator, size_t initialSize = 5, size_t maxSize = 20, std::chrono::seconds maxIdleTime = std::chrono::seconds(300));

    // 析构函数
    ~ConnectionPool();

    // 获取连接
    ConnectionPtr getConnection();

    // 归还连接
    void returnConnection(ConnectionPtr conn);

    // 获取连接池状态
    size_t getActiveConnections() const;
    size_t getIdleConnections() const;

    // 关闭连接池
    void shutdown();

private:
    struct ConnectionWrapper
    {
        ConnectionPtr connection;
        std::chrono::steady_clock::time_point lastUsed;

        ConnectionWrapper(ConnectionPtr conn) : connection(conn), lastUsed(std::chrono::steady_clock::now()) {}
    };

    ConnectionFactory _factory;
    ConnectionValidator _validator;
    size_t _initialSize;
    size_t _maxSize;
    std::chrono::seconds _maxIdleTime;

    std::queue<ConnectionWrapper> _availableConnections;
    std::atomic<size_t> _activeConnections;
    std::atomic<bool> _shutdown;

    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::thread _cleanupThread;

    // 私有方法
    void initializeConnections();
    void cleanupIdleConnections();
    void cleanupWorker();
    ConnectionPtr createConnection();
};

// RAII连接管理器
template <typename T>
class ConnectionRAII
{
public:
    ConnectionRAII(ConnectionPool<T> &pool);
    ~ConnectionRAII();

    // 禁止拷贝
    ConnectionRAII(const ConnectionRAII &) = delete;
    ConnectionRAII &operator=(const ConnectionRAII &) = delete;

    // 允许移动
    ConnectionRAII(ConnectionRAII &&other) noexcept;
    ConnectionRAII &operator=(ConnectionRAII &&other) noexcept;

    // 获取连接
    std::shared_ptr<T> get() const;
    std::shared_ptr<T> operator->() const;
    T &operator*() const;

    // 检查连接是否有效
    bool isValid() const;

private:
    ConnectionPool<T> *_pool;
    std::shared_ptr<T> _connection;
};

#endif