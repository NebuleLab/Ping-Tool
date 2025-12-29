#ifndef PINGLOGMODEL_H
#define PINGLOGMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>

struct PingLogEntry {
    QDateTime timestamp;
    QString target;
    int seq;
    int rtt;
    int ttl;
    QString status;
};

class PingLogModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PingLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addEntry(const QString &target, int rtt, int ttl, int seq);
    void clear();

private:
    QList<PingLogEntry> m_data;
    const int MAX_LOG_SIZE = 1000; // Limit memory usage
};

#endif // PINGLOGMODEL_H
