#include "ChartWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>

void InteractiveChartView::wheelEvent(QWheelEvent *event)
{
    if (chart()->axes().isEmpty()) return;
    
    auto axisX = qobject_cast<QDateTimeAxis*>(chart()->axes(Qt::Horizontal).first());
    if (!axisX) return;

    qint64 min = axisX->min().toMSecsSinceEpoch();
    qint64 max = axisX->max().toMSecsSinceEpoch();
    qint64 range = max - min;

    if (event->angleDelta().y() > 0) {
        // Zoom in
        range /= 1.5;
    } else {
        // Zoom out
        range *= 1.5;
    }
    
    // Center zoom
    qint64 center = min + (max - min) / 2;
    
    axisX->setRange(QDateTime::fromMSecsSinceEpoch(center - range / 2), 
                    QDateTime::fromMSecsSinceEpoch(center + range / 2));
                    
    event->accept();
}

void InteractiveChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    }
    QChartView::mousePressEvent(event);
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        auto axisX = qobject_cast<QDateTimeAxis*>(chart()->axes(Qt::Horizontal).first());
        if (axisX) {
            qint64 min = axisX->min().toMSecsSinceEpoch();
            qint64 max = axisX->max().toMSecsSinceEpoch();
            qint64 range = max - min;
            
            // Calculate delta in pixels
            int dx = event->pos().x() - m_lastMousePos.x();
            
            // Map pixel delta to time delta
            // Width of plot area
            qreal plotWidth = chart()->plotArea().width();
            if (plotWidth > 0) {
                qint64 timeDelta = -dx * range / plotWidth;
                
                axisX->setRange(QDateTime::fromMSecsSinceEpoch(min + timeDelta), 
                                QDateTime::fromMSecsSinceEpoch(max + timeDelta));
            }
        }
        m_lastMousePos = event->pos();
        event->accept();
    }
    QChartView::mouseMoveEvent(event);
}

void InteractiveChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    }
    QChartView::mouseReleaseEvent(event);
}

ChartWindow::ChartWindow(const QString &target, int timeoutMs, QObject *parent)
    : QMainWindow(nullptr) // Independent window
    , m_target(target)
    , m_timeoutMs(timeoutMs)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QString("Ping Chart - %1").arg(target));
    resize(900, 500);

    setupUi();

    // Load last 1 hour data from DB by default -> User requested manual query only
    // QDateTime end = QDateTime::currentDateTime();
    // QDateTime start = end.addSecs(-3600);
    // loadFromDatabase(start, end);
}

ChartWindow::~ChartWindow()
{
}

void ChartWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Controls
    QHBoxLayout *controlLayout = new QHBoxLayout();
    
    controlLayout->addWidget(new QLabel("Start:"));
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime().addSecs(-3600)); // Default 1 hour ago
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    controlLayout->addWidget(m_startTimeEdit);

    controlLayout->addWidget(new QLabel("End:"));
    m_endTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    controlLayout->addWidget(m_endTimeEdit);

    m_queryBtn = new QPushButton("Query Database");
    controlLayout->addWidget(m_queryBtn);
    
    controlLayout->addSpacing(20);

    m_autoScaleYCheck = new QCheckBox("Auto Y");
    m_autoScaleYCheck->setChecked(true);
    controlLayout->addWidget(m_autoScaleYCheck);

    controlLayout->addWidget(new QLabel("Max Y:"));
    m_yMaxSpin = new QSpinBox();
    m_yMaxSpin->setRange(10, 10000);
    m_yMaxSpin->setValue(100);
    m_yMaxSpin->setEnabled(false); // Disabled when Auto is on
    controlLayout->addWidget(m_yMaxSpin);
    
    controlLayout->addStretch();

    mainLayout->addLayout(controlLayout);

    // Chart
    m_chart = new QChart();
    m_chart->setTitle(QString("RTT for %1").arg(m_target));
    // m_chart->legend()->hide(); // Show legend for Success/Timeout distinction

    m_series = new QLineSeries();
    m_series->setName("Success");
    m_chart->addSeries(m_series);

    m_timeoutSeries = new QLineSeries();
    m_timeoutSeries->setName("Timeout");
    m_timeoutSeries->setColor(Qt::red);
    m_chart->addSeries(m_timeoutSeries);

    m_axisX = new QDateTimeAxis();
    m_axisX->setFormat("HH:mm:ss");
    m_axisX->setTitleText("Time");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);
    m_timeoutSeries->attachAxis(m_axisX);

    m_axisY = new QValueAxis();
    m_axisY->setTitleText("RTT (ms)");
    m_axisY->setRange(0, m_timeoutMs * 1.2); // Initial range based on timeout
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
    m_timeoutSeries->attachAxis(m_axisY);

    InteractiveChartView *chartView = new InteractiveChartView(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    mainLayout->addWidget(chartView);

    connect(m_queryBtn, &QPushButton::clicked, this, &ChartWindow::onQueryClicked);
    connect(m_autoScaleYCheck, &QCheckBox::stateChanged, this, &ChartWindow::onAutoScaleYChanged);
    connect(m_yMaxSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChartWindow::onYMaxChanged);
}

void ChartWindow::onNewResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs)
{
    if (target != m_target) return;

    if (m_endTimeEdit->dateTime() > QDateTime::currentDateTime().addSecs(-10)) {
        int val;
        QLineSeries *seriesToUse;

        if (rtt >= 0) {
            val = rtt;
            seriesToUse = m_series;
        } else {
            // Timeout
            val = timeoutMs;
            seriesToUse = m_timeoutSeries;
        }

        // 4 points: (Start, 0) -> (Start, Val) -> (Return, Val) -> (Return, 0)
        seriesToUse->append(startTime, 0);
        seriesToUse->append(startTime, val);
        seriesToUse->append(returnTime, val);
        seriesToUse->append(returnTime, 0);

        updateAxisRange();
    }
}

void ChartWindow::onQueryClicked()
{
    loadFromDatabase(m_startTimeEdit->dateTime(), m_endTimeEdit->dateTime());
}

void ChartWindow::loadFromDatabase(const QDateTime &start, const QDateTime &end)
{
    m_series->clear();
    m_timeoutSeries->clear();
    
    // Create a separate connection for UI thread
    QString connectionName = "ChartConnection_" + QString::number((quint64)this);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        QString dbPath = QCoreApplication::applicationDirPath() + "/pinglog.db";
        db.setDatabaseName(dbPath);
        
        if (db.open()) {
            QSqlQuery query(db);
            query.prepare("SELECT start_time, return_time, rtt, timeout_val FROM ping_log WHERE target = :target AND timestamp BETWEEN :start AND :end ORDER BY timestamp ASC");
            query.bindValue(":target", m_target);
            query.bindValue(":start", start);
            query.bindValue(":end", end);
            
            if (query.exec()) {
                while (query.next()) {
                    qint64 startTime = query.value(0).toLongLong();
                    qint64 returnTime = query.value(1).toLongLong();
                    int rtt = query.value(2).toInt();
                    int timeoutVal = query.value(3).toInt();
                    
                    // Fallback for old data if columns are null/zero
                    if (startTime == 0) {
                         // Estimate from timestamp (which is return time roughly)
                         // This handles legacy data before schema change
                         // But we can't easily know rtt if it was timeout (-1)
                         // Just skip or approximate?
                         // Let's approximate using current timeout if 0
                         if (timeoutVal == 0) timeoutVal = m_timeoutMs;
                         returnTime = query.value(1).toDateTime().toMSecsSinceEpoch(); // If return_time col is null, this might be 0 too?
                         // Actually if return_time is null, query.value(1) is 0.
                         // We should check if return_time is valid.
                         // If not, use timestamp column?
                         // Wait, I didn't select timestamp column in the new query.
                         // Let's select timestamp as fallback.
                    }

                    int val;
                    QLineSeries *seriesToUse;

                    if (rtt >= 0) {
                        val = rtt;
                        seriesToUse = m_series;
                    } else {
                        // Timeout
                        val = timeoutVal > 0 ? timeoutVal : m_timeoutMs;
                        seriesToUse = m_timeoutSeries;
                    }
                    
                    // If we have valid start/return times, use them.
                    if (startTime > 0 && returnTime > 0) {
                        seriesToUse->append(startTime, 0);
                        seriesToUse->append(startTime, val);
                        seriesToUse->append(returnTime, val);
                        seriesToUse->append(returnTime, 0);
                    }
                }
            } else {
                qWarning() << "Query failed:" << query.lastError().text();
            }
            db.close();
        } else {
             qWarning() << "Failed to open DB for chart:" << db.lastError().text();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
    
    updateAxisRange();
}

void ChartWindow::updateAxisRange()
{
    if (m_series->count() == 0 && m_timeoutSeries->count() == 0) return;

    qint64 firstTime = -1;
    qint64 lastTime = -1;
    double maxY = 0;

    auto processSeries = [&](QLineSeries *s) {
        if (s->count() > 0) {
            qint64 sFirst = s->at(0).x();
            qint64 sLast = s->at(s->count() - 1).x();
            
            if (firstTime == -1 || sFirst < firstTime) firstTime = sFirst;
            if (lastTime == -1 || sLast > lastTime) lastTime = sLast;

            for (const QPointF &p : s->points()) {
                if (p.y() > maxY) maxY = p.y();
            }
        }
    };

    processSeries(m_series);
    processSeries(m_timeoutSeries);

    // Ensure some width
    if (lastTime <= firstTime) {
        if (firstTime == -1) {
             firstTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
             lastTime = firstTime + 1000;
        } else {
             firstTime -= 1000;
             lastTime += 1000;
        }
    }
    
    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(firstTime), QDateTime::fromMSecsSinceEpoch(lastTime));
    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(firstTime), QDateTime::fromMSecsSinceEpoch(lastTime));
    
    if (m_autoScaleYCheck->isChecked()) {
        int newMax = (int)(maxY * 1.2);
        if (newMax < 10) newMax = 10; // Minimum range
        
        // Block signals to prevent feedback loop if we were to update spinbox here
        // Actually, let's update spinbox to reflect current auto scale
        m_yMaxSpin->blockSignals(true);
        m_yMaxSpin->setValue(newMax);
        m_yMaxSpin->blockSignals(false);
        
        m_axisY->setRange(0, newMax);
    } else {
        // Manual mode, use spinbox value
        m_axisY->setRange(0, m_yMaxSpin->value());
    }
}

void ChartWindow::onAutoScaleYChanged(int state)
{
    bool autoScale = (state == Qt::Checked);
    m_yMaxSpin->setEnabled(!autoScale);
    
    if (autoScale) {
        updateAxisRange();
    } else {
        // If switching to manual, keep current value or use spinbox value?
        // Spinbox already has value.
        m_axisY->setRange(0, m_yMaxSpin->value());
    }
}

void ChartWindow::onYMaxChanged(int value)
{
    if (!m_autoScaleYCheck->isChecked()) {
        m_axisY->setRange(0, value);
    }
}
