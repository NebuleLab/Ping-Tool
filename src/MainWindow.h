#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableView>
#include "PingManager.h"
#include "PingModel.h"
#include "PingLogModel.h"
#include "DatabaseThread.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onStopClicked();
    void onStopAllClicked();
    void onAddClicked();
    void onRemoveClicked();
    void onTargetDoubleClicked(const QModelIndex &index);
    void onNewResult(QString target, int rtt, int ttl, int seq);
    void updateDbStatus(long long generated, long long written, QString lastAction);

private:
    void setupUi();

    QLineEdit *m_targetInput;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QSpinBox *m_timeoutSpin;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_stopAllBtn;
    
    QTableView *m_summaryView;
    QTableView *m_logView;
    
    PingManager *m_pingManager;
    PingModel *m_pingModel;
    PingLogModel *m_logModel;
    DatabaseThread *m_dbThread;
};

#endif // MAINWINDOW_H
