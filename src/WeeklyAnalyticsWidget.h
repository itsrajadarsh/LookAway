#ifndef WEEKLYANALYTICSWIDGET_H
#define WEEKLYANALYTICSWIDGET_H

#include <QWidget>
#include "SettingsManager.h"

class WeeklyAnalyticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit WeeklyAnalyticsWidget(SettingsManager* settings, QWidget* parent = nullptr);

    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    SettingsManager* m_settings;
    QList<DayStats> m_cachedStats;
    int m_hoveredIndex;

    QRect barRectForIndex(int index, int totalBars, const QRect& chartArea) const;
};

#endif // WEEKLYANALYTICSWIDGET_H
