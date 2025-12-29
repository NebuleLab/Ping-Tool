#ifndef PINGMANAGER_H
#define PINGMANAGER_H

#include <QObject>
#include <QMap>
#include <QMutex>
#include "PingWorker.h"

class PingManager : public QObject
{
    Q_OBJECT
public:
    explicit PingManager(QObject *parent = nullptr);
    ~PingManager();

    void startPing(const QString &target, uint32_t timeoutMs);
    void stopPing(const QString &target);
    void stopAll();

signals:
    void newResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs);

private:
    QMap<QString, PingWorker*> m_workers;
    QMutex m_mutex;
};

#endif // PINGMANAGER_H
