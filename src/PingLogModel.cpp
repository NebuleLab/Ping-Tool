#include "PingLogModel.h"

PingLogModel::PingLogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PingLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_data.size();
}

int PingLogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 6; // Time, Target, Seq, RTT, TTL, Status
}

QVariant PingLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    // Show newest first
    const PingLogEntry &entry = m_data[m_data.size() - 1 - index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return entry.timestamp.toString("HH:mm:ss.zzz");
        case 1: return entry.target;
        case 2: return entry.seq;
        case 3: return (entry.rtt >= 0) ? QString("%1 ms").arg(entry.rtt) : "-";
        case 4: return (entry.ttl > 0) ? QString::number(entry.ttl) : "-";
        case 5: return entry.status;
        }
    }
    return QVariant();
}

QVariant PingLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return QString::fromUtf8("Time");
    case 1: return QString::fromUtf8("Target");
    case 2: return QString::fromUtf8("Seq");
    case 3: return QString::fromUtf8("RTT");
    case 4: return QString::fromUtf8("TTL");
    case 5: return QString::fromUtf8("Status");
    }
    return QVariant();
}

void PingLogModel::addEntry(const QString &target, int rtt, int ttl, int seq)
{
    beginInsertRows(QModelIndex(), 0, 0); // Insert at top visually (but we append to list and reverse in data())
    // Actually, standard is append to list, but if we want newest at top, we can prepend or map index.
    // Let's append and map index in data() for performance (prepend is O(N)).
    
    PingLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.target = target;
    entry.seq = seq;
    entry.rtt = rtt;
    entry.ttl = ttl;
    
    if (rtt >= 0) entry.status = "Reply";
    else if (rtt == -1) entry.status = "Timeout";
    else entry.status = "Error";

    m_data.append(entry);
    endInsertRows();

    // Enforce size limit
    if (m_data.size() > MAX_LOG_SIZE) {
        beginRemoveRows(QModelIndex(), MAX_LOG_SIZE, MAX_LOG_SIZE);
        m_data.removeFirst(); // Remove oldest (index 0 in m_data, which corresponds to last row in view)
        endRemoveRows();
    }
}

void PingLogModel::clear()
{
    beginResetModel();
    m_data.clear();
    endResetModel();
}
