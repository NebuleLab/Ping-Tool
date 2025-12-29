#ifndef PINGWORKER_H
#define PINGWORKER_H

#include <QThread>
#include <QString>
#include <QDateTime>
#include <atomic>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#endif

class PingWorker : public QThread
{
    Q_OBJECT
public:
    explicit PingWorker(const QString &target, uint32_t timeoutMs = 1000, QObject *parent = nullptr);
    ~PingWorker();

    void stop();
    void setTimeout(uint32_t timeoutMs);

protected:
    void run() override;

signals:
    // rtt: Round Trip Time in ms. -1 indicates timeout/error.
    // ttl: Time To Live.
    // seq: Sequence number.
    // startTime: Timestamp when ping was sent (ms since epoch)
    // returnTime: Timestamp when reply was received or timeout occurred (ms since epoch)
    // timeoutMs: The timeout value used for this ping
    void newResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs);

private:
    QString m_target;
    uint32_t m_timeoutMs;
    std::atomic<bool> m_running;
    int m_seq;

#ifdef Q_OS_WIN
    HANDLE m_hIcmpFile;
#endif
};

#endif // PINGWORKER_H
