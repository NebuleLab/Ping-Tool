#ifndef DATABASETHREAD_H
#define DATABASETHREAD_H

#include <QThread>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMutex>
#include <QList>
#include <QDateTime>
#include <QWaitCondition>

struct LogEntry {
    QString target;
    int rtt;
    int ttl;
    int seq;
    qint64 startTime;
    qint64 returnTime;
    int timeoutMs;
};

class DatabaseThread : public QThread
{
    Q_OBJECT
public:
    explicit DatabaseThread(QObject *parent = nullptr);
    ~DatabaseThread();

    void stop();

signals:
    void statusUpdated(long long generated, long long written, QString lastAction);

public slots:
    void saveResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs);

protected:
    void run() override;

private:
    void processQueue();
    void commitTransaction();

    QSqlDatabase m_db;
    QList<LogEntry> m_queue;
    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_running;
    int m_batchCount;
    const int BATCH_SIZE = 500;
    
    long long m_totalGenerated;
    long long m_totalWritten;
};

#endif // DATABASETHREAD_H
