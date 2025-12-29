#ifndef PINGMODEL_H
#define PINGMODEL_H

#include <QAbstractTableModel>
#include <QString>
#include <QList>
#include <QMap>

struct PingStats {
    QString target;
    int sent = 0;
    int received = 0;
    int minRtt = 999999;
    int maxRtt = 0;
    double avgRtt = 0.0;
    long long totalRtt = 0;
    int lastTtl = 0;
    QString status = "Idle";
};

class PingModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PingModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addTarget(const QString &target);
    void removeTarget(const QString &target);
    void updateResult(const QString &target, int rtt, int ttl, int seq);
    void clear();
    
    QStringList getTargets() const;

private:
    QList<PingStats> m_data;
    QMap<QString, int> m_targetRowMap;
};

#endif // PINGMODEL_H
