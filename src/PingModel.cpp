#include "PingModel.h"

PingModel::PingModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PingModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_data.size();
}

int PingModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 9; // Target, Sent, Recv, Loss, Min, Max, Avg, TTL, Status
}

QVariant PingModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size())
        return QVariant();

    const PingStats &stats = m_data[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return stats.target;
        case 1: return stats.sent;
        case 2: return stats.received;
        case 3: {
            if (stats.sent == 0) return "0%";
            double loss = 100.0 * (stats.sent - stats.received) / stats.sent;
            return QString::number(loss, 'f', 1) + "%";
        }
        case 4: return (stats.minRtt == 999999) ? "-" : QString::number(stats.minRtt);
        case 5: return stats.maxRtt;
        case 6: return QString::number(stats.avgRtt, 'f', 1);
        case 7: return stats.lastTtl;
        case 8: return stats.status;
        }
    }
    return QVariant();
}

QVariant PingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return QVariant();

    switch (section) {
    case 0: return QString::fromUtf8("Target");
    case 1: return QString::fromUtf8("Sent");
    case 2: return QString::fromUtf8("Recv");
    case 3: return QString::fromUtf8("Loss %");
    case 4: return QString::fromUtf8("Min (ms)");
    case 5: return QString::fromUtf8("Max (ms)");
    case 6: return QString::fromUtf8("Avg (ms)");
    case 7: return QString::fromUtf8("TTL");
    case 8: return QString::fromUtf8("Status");
    }
    return QVariant();
}

void PingModel::addTarget(const QString &target)
{
    if (m_targetRowMap.contains(target)) return;

    beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
    PingStats stats;
    stats.target = target;
    m_data.append(stats);
    m_targetRowMap[target] = m_data.size() - 1;
    endInsertRows();
}

void PingModel::removeTarget(const QString &target)
{
    if (!m_targetRowMap.contains(target)) return;

    int row = m_targetRowMap[target];
    beginRemoveRows(QModelIndex(), row, row);
    m_data.removeAt(row);
    m_targetRowMap.remove(target);
    // Rebuild map indices
    for (int i = row; i < m_data.size(); ++i) {
        m_targetRowMap[m_data[i].target] = i;
    }
    endRemoveRows();
}

void PingModel::updateResult(const QString &target, int rtt, int ttl, int seq)
{
    if (!m_targetRowMap.contains(target)) return;

    int row = m_targetRowMap[target];
    PingStats &stats = m_data[row];

    stats.sent++;
    if (rtt >= 0) {
        stats.received++;
        stats.totalRtt += rtt;
        stats.lastTtl = ttl;
        if (rtt < stats.minRtt) stats.minRtt = rtt;
        if (rtt > stats.maxRtt) stats.maxRtt = rtt;
        stats.avgRtt = (double)stats.totalRtt / stats.received;
        stats.status = "Active";
    } else if (rtt == -1) {
        stats.status = "Timeout";
    } else {
        stats.status = "Error";
    }

    emit dataChanged(index(row, 1), index(row, 8));
}

void PingModel::clear()
{
    beginResetModel();
    m_data.clear();
    m_targetRowMap.clear();
    endResetModel();
}

QStringList PingModel::getTargets() const
{
    QStringList list;
    for (const auto &stats : m_data) {
        list << stats.target;
    }
    return list;
}
