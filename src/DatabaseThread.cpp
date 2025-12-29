#include "DatabaseThread.h"
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include "DatabaseThread.h"
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

DatabaseThread::DatabaseThread(QObject *parent)
    : QThread(parent)
    , m_running(true)
    , m_batchCount(0)
    , m_totalGenerated(0)
    , m_totalWritten(0)
{
}

DatabaseThread::~DatabaseThread()
{
    stop();
    wait();
}

void DatabaseThread::stop()
{
    {
        QMutexLocker locker(&m_mutex);
        m_running = false;
        m_cond.wakeOne();
    }
}

void DatabaseThread::saveResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    LogEntry entry;
    entry.target = target;
    entry.rtt = rtt;
    entry.ttl = ttl;
    entry.seq = seq;
    entry.startTime = startTime;
    entry.returnTime = returnTime;
    entry.timeoutMs = timeoutMs;
    // entry.timestamp = QDateTime::currentDateTime(); // Use returnTime as timestamp reference if needed
    
    m_queue.append(entry);
    
    m_totalGenerated++;
    emit statusUpdated(m_totalGenerated, m_totalWritten, "Pending");
    
    m_cond.wakeOne();
}

void DatabaseThread::run()
{
    // Initialize DB in this thread
    m_db = QSqlDatabase::addDatabase("QSQLITE", "PingLogConnection");
    
    // Use application directory for easier access
    QString dbPath = QCoreApplication::applicationDirPath();
    QString dbFile = dbPath + "/pinglog.db";
    
    qDebug() << "Database file path:" << dbFile;
    
    m_db.setDatabaseName(dbFile);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return;
    }

    QSqlQuery query(m_db);
    // Create table if not exists (basic schema)
    if (!query.exec("CREATE TABLE IF NOT EXISTS ping_log ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "timestamp DATETIME, "
                    "target TEXT, "
                    "rtt INTEGER, "
                    "ttl INTEGER, "
                    "seq INTEGER)")) {
        qCritical() << "Failed to create table:" << query.lastError().text();
    }

    // Ensure new columns exist (idempotent-ish in SQLite via separate statements, ignoring errors if exist)
    query.exec("ALTER TABLE ping_log ADD COLUMN start_time INTEGER");
    query.exec("ALTER TABLE ping_log ADD COLUMN return_time INTEGER");
    query.exec("ALTER TABLE ping_log ADD COLUMN timeout_val INTEGER");

    m_db.transaction();

    while (true) {
        QList<LogEntry> currentBatch;
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running && m_queue.isEmpty()) {
                break;
            }
            if (m_queue.isEmpty()) {
                m_cond.wait(&m_mutex, 1000); // Wait or timeout to commit partial batch
            }
            currentBatch = m_queue;
            m_queue.clear();
        }

        if (!currentBatch.isEmpty()) {
            QSqlQuery insertQuery(m_db);
            insertQuery.prepare("INSERT INTO ping_log (timestamp, target, rtt, ttl, seq, start_time, return_time, timeout_val) "
                                "VALUES (:ts, :target, :rtt, :ttl, :seq, :start, :ret, :tmo)");
            
            for (const auto &entry : currentBatch) {
                // Use returnTime as the main timestamp for compatibility/display
                insertQuery.bindValue(":ts", QDateTime::fromMSecsSinceEpoch(entry.returnTime));
                insertQuery.bindValue(":target", entry.target);
                insertQuery.bindValue(":rtt", entry.rtt);
                insertQuery.bindValue(":ttl", entry.ttl);
                insertQuery.bindValue(":seq", entry.seq);
                insertQuery.bindValue(":start", entry.startTime);
                insertQuery.bindValue(":ret", entry.returnTime);
                insertQuery.bindValue(":tmo", entry.timeoutMs);
                insertQuery.exec();
                
                m_totalWritten++;
                m_batchCount++;
                
                if (m_batchCount >= BATCH_SIZE) {
                    m_db.commit();
                    m_db.transaction();
                    m_batchCount = 0;
                    emit statusUpdated(m_totalGenerated, m_totalWritten, "Committed");
                }
            }
            // Emit update after batch processing if not committed yet
            if (m_batchCount > 0) {
                emit statusUpdated(m_totalGenerated, m_totalWritten, QString("Writing (%1/%2)").arg(m_batchCount).arg(BATCH_SIZE));
            }
        } else {
            // If queue was empty (timeout), we just continue waiting.
            // We only commit when BATCH_SIZE is reached or on exit.
        }
    }

    // Final commit
    if (m_batchCount > 0) {
        m_db.commit();
        emit statusUpdated(m_totalGenerated, m_totalWritten, "Committed (Exit)");
    }
    m_db.close();
}
