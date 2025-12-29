#ifndef CHARTWINDOW_H
#define CHARTWINDOW_H

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QChartGlobal>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QDateTimeEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QtCharts/QValueAxis>

// using namespace QtCharts; // Namespace issue, trying global or macro handling

class InteractiveChartView : public QChartView
{
public:
    InteractiveChartView(QChart *chart, QWidget *parent = nullptr)
        : QChartView(chart, parent)
        , m_isDragging(false)
    {}

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_isDragging;
    QPoint m_lastMousePos;
};

class ChartWindow : public QMainWindow
{
    Q_OBJECT

public:
public:
    explicit ChartWindow(const QString &target, int timeoutMs, QObject *parent = nullptr);
    ~ChartWindow();

public slots:
    void onNewResult(QString target, int rtt, int ttl, int seq, qint64 startTime, qint64 returnTime, int timeoutMs);
    void onQueryClicked();

private:
    void setupUi();
    void updateAxisRange();
    void loadFromDatabase(const QDateTime &start, const QDateTime &end);

    QString m_target;
    int m_timeoutMs;
    QChart *m_chart;
    QLineSeries *m_series;        // Success pings
    QLineSeries *m_timeoutSeries; // Timeout pings
    QDateTimeAxis *m_axisX;
    QValueAxis *m_axisY;
    
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QPushButton *m_queryBtn;
    
    QCheckBox *m_autoScaleYCheck;
    QSpinBox *m_yMaxSpin;

private slots:
    void onYMaxChanged(int value);
    void onAutoScaleYChanged(int state);
};

#endif // CHARTWINDOW_H
