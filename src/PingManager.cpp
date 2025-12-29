#include "PingManager.h"

PingManager::PingManager(QObject *parent)
    : QObject(parent)
{
}

PingManager::~PingManager()
{
    stopAll();
}

void PingManager::startPing(const QString &target, uint32_t timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    if (m_workers.contains(target)) {
        // Already running, maybe update timeout?
        // For now, ignore or restart.
        return;
    }

    PingWorker *worker = new PingWorker(target, timeoutMs, this);
    connect(worker, &PingWorker::newResult, this, &PingManager::newResult);
    // Connect finished to cleanup if needed, but we manage them manually in map.
    
    m_workers.insert(target, worker);
    worker->start();
}

void PingManager::stopPing(const QString &target)
{
    QMutexLocker locker(&m_mutex);
    if (m_workers.contains(target)) {
        PingWorker *worker = m_workers.take(target);
        worker->stop();
        worker->quit();
        worker->wait();
        delete worker;
    }
}

void PingManager::stopAll()
{
    QMutexLocker locker(&m_mutex);
    for (auto worker : m_workers) {
        worker->stop();
        worker->quit();
        worker->wait();
        delete worker;
    }
    m_workers.clear();
}
