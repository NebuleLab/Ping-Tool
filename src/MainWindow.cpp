#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QStatusBar>
#include <QSettings>
#include "ChartWindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_pingManager(new PingManager(this))
    , m_pingModel(new PingModel(this))
    , m_logModel(new PingLogModel(this))
    , m_dbThread(new DatabaseThread(this))
{
    setupUi();

    // Start DB thread
    m_dbThread->start();

    // Connect Manager to UI and DB
    connect(m_pingManager, &PingManager::newResult, this, &MainWindow::onNewResult);
    
    // Database connection is done in onNewResult to ensure thread safety if needed, 
    // or we can connect directly if we use Qt::QueuedConnection (default for threads).
    // However, onNewResult also updates UI models, so we can chain it there.
    // Actually, let's connect directly for DB to ensure it gets data even if UI is busy?
    // But we need to format data for DB. 
    // PingManager emits (target, rtt, ttl, seq).
    // DatabaseThread::saveResult takes same args.
    connect(m_pingManager, &PingManager::newResult, m_dbThread, &DatabaseThread::saveResult);
    
    // Connect DB status
    connect(m_dbThread, &DatabaseThread::statusUpdated, this, &MainWindow::updateDbStatus);
}

MainWindow::~MainWindow()
{
    m_dbThread->stop();
    m_dbThread->wait();
}

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Top Control Bar
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    controlLayout->addWidget(new QLabel(QString::fromUtf8("Target:")));
    m_targetInput = new QLineEdit();
    m_targetInput->setPlaceholderText("IP or Hostname");
    controlLayout->addWidget(m_targetInput);

    m_addBtn = new QPushButton(QString::fromUtf8("Add"));
    controlLayout->addWidget(m_addBtn);

    m_removeBtn = new QPushButton(QString::fromUtf8("Remove"));
    controlLayout->addWidget(m_removeBtn);

    controlLayout->addWidget(new QLabel(QString::fromUtf8("Timeout (ms):")));
    m_timeoutSpin = new QSpinBox();
    m_timeoutSpin->setRange(100, 10000);
    m_timeoutSpin->setValue(1000);
    controlLayout->addWidget(m_timeoutSpin);

    m_startBtn = new QPushButton(QString::fromUtf8("Start All"));
    controlLayout->addWidget(m_startBtn);

    m_stopBtn = new QPushButton(QString::fromUtf8("Stop"));
    controlLayout->addWidget(m_stopBtn);

    m_stopAllBtn = new QPushButton(QString::fromUtf8("Stop All"));
    controlLayout->addWidget(m_stopAllBtn);

    mainLayout->addLayout(controlLayout);

    // Splitter for Views
    QSplitter *splitter = new QSplitter(Qt::Vertical);

    // Summary View
    m_summaryView = new QTableView();
    m_summaryView->setModel(m_pingModel);
    m_summaryView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_summaryView->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(m_summaryView);

    // Log View
    m_logView = new QTableView();
    m_logView->setModel(m_logModel);
    m_logView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    splitter->addWidget(m_logView);

    mainLayout->addWidget(splitter);

    // Connect Buttons
    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddClicked);
    connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemoveClicked);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_stopAllBtn, &QPushButton::clicked, this, &MainWindow::onStopAllClicked);
    
    // Double click on summary view
    connect(m_summaryView, &QTableView::doubleClicked, this, &MainWindow::onTargetDoubleClicked);
    
    // Status Bar
    statusBar()->showMessage(QString::fromUtf8("Ready"));

    resize(800, 600);
    setWindowTitle(QString::fromUtf8("Qt6 Multithreaded Ping Tool"));
    
    // Load targets
    QSettings settings("MyCompany", "PingTool");
    QStringList targets = settings.value("targets").toStringList();
    for (const QString &t : targets) {
        m_pingModel->addTarget(t);
    }
}

void MainWindow::onAddClicked()
{
    QString targetInput = m_targetInput->text().trimmed();
    if (!targetInput.isEmpty()) {
        m_pingModel->addTarget(targetInput);
        m_targetInput->clear();
        
        // Save targets
        QSettings settings("MyCompany", "PingTool");
        settings.setValue("targets", m_pingModel->getTargets());
    }
}

void MainWindow::onRemoveClicked()
{
    QModelIndex index = m_summaryView->currentIndex();
    if (index.isValid()) {
        QString target = m_pingModel->data(m_pingModel->index(index.row(), 0)).toString();
        
        // Stop ping first
        m_pingManager->stopPing(target);
        
        // Remove from model
        m_pingModel->removeTarget(target);
        
        // Save targets
        QSettings settings("MyCompany", "PingTool");
        settings.setValue("targets", m_pingModel->getTargets());
    } else {
        QMessageBox::information(this, "Info", "Please select a target to remove.");
    }
}

void MainWindow::onTargetDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    
    QString target = m_pingModel->data(m_pingModel->index(index.row(), 0)).toString();
    int timeout = m_timeoutSpin->value();
    
    ChartWindow *chartWin = new ChartWindow(target, timeout, this);
    connect(m_pingManager, &PingManager::newResult, chartWin, &ChartWindow::onNewResult);
    chartWin->show();
}

void MainWindow::onStartClicked()
{
    int timeout = m_timeoutSpin->value();
    QStringList targets = m_pingModel->getTargets();
    
    for (const QString &target : targets) {
        m_pingManager->startPing(target, timeout);
    }
}

void MainWindow::onStopClicked()
{
    // Stop selected target
    QModelIndex index = m_summaryView->currentIndex();
    if (index.isValid()) {
        QString target = m_pingModel->data(m_pingModel->index(index.row(), 0)).toString();
        m_pingManager->stopPing(target);
        // Update status manually? PingManager doesn't emit stopped.
        // We can assume it stops.
        // But PingModel status is updated by newResult. 
        // Maybe we should add setStatus to PingModel.
    }
}

void MainWindow::onStopAllClicked()
{
    m_pingManager->stopAll();
}

void MainWindow::onNewResult(QString target, int rtt, int ttl, int seq)
{
    // Update Summary Model
    m_pingModel->updateResult(target, rtt, ttl, seq);

    // Update Log Model
    m_logModel->addEntry(target, rtt, ttl, seq);
}

void MainWindow::updateDbStatus(long long generated, long long written, QString lastAction)
{
    QString msg = QString::fromUtf8("DB Status: Generated %1 | Written %2 | Action: %3")
                      .arg(generated)
                      .arg(written)
                      .arg(lastAction);
    statusBar()->showMessage(msg);
}
