#include "WeeklyAnalyticsWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <QDate>
#include <algorithm>

WeeklyAnalyticsWidget::WeeklyAnalyticsWidget(SettingsManager* settings, QWidget* parent)
    : QWidget(parent),
      m_settings(settings),
      m_hoveredIndex(-1) {

    setMouseTracking(true);
    setMinimumHeight(200);
    setFixedHeight(215);

    connect(m_settings, &SettingsManager::statsUpdated, this, &WeeklyAnalyticsWidget::refresh);
    connect(m_settings, &SettingsManager::settingsChanged, this, &WeeklyAnalyticsWidget::refresh);

    refresh();
}

void WeeklyAnalyticsWidget::refresh() {
    m_cachedStats = m_settings->recentStats(7);
    update();
}

QRect WeeklyAnalyticsWidget::barRectForIndex(int index, int totalBars, const QRect& chartArea) const {
    if (totalBars <= 0) return QRect();
    int slotWidth = chartArea.width() / totalBars;
    int barWidth = std::min(28, slotWidth - 12);
    int x = chartArea.x() + index * slotWidth + (slotWidth - barWidth) / 2;
    return QRect(x, chartArea.y(), barWidth, chartArea.height());
}

void WeeklyAnalyticsWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    int w = width();
    int h = height();

    // 1. Background Card
    QPainterPath bgPath;
    bgPath.addRoundedRect(QRectF(1, 1, w - 2, h - 2), 12, 12);
    painter.fillPath(bgPath, QColor("#1e293b"));
    painter.setPen(QPen(QColor("#334155"), 1));
    painter.drawPath(bgPath);

    // 2. Header
    painter.setFont(QFont("Segoe UI", 10, QFont::Bold));
    painter.setPen(QColor("#f8fafc"));
    painter.drawText(18, 27, "7-Day Rest Adherence");

    // Streak and Compliance Badges
    int streak = m_settings->currentStreakDays();
    double compRate = m_settings->weeklyComplianceRate();

    QString compText = QString("%1% Rate").arg(static_cast<int>(compRate));
    QString streakText = (streak > 0)
        ? QString("🔥 %1-Day Streak").arg(streak)
        : QString("🌱 Day 1 Active");

    QFont badgeFont("Segoe UI", 8, QFont::DemiBold);
    painter.setFont(badgeFont);

    QFontMetrics fm(badgeFont);
    int streakW = fm.horizontalAdvance(streakText) + 14;
    int compW = fm.horizontalAdvance(compText) + 14;
    int badgeH = 22;
    int rightEdge = w - 18;

    // Compliance Badge (top right)
    QRect compRect(rightEdge - compW, 11, compW, badgeH);
    painter.setBrush(QColor(16, 185, 129, 30));
    painter.setPen(QPen(QColor(16, 185, 129, 110), 1));
    painter.drawRoundedRect(compRect, 6, 6);
    painter.setPen(QColor("#34d399"));
    painter.drawText(compRect, Qt::AlignCenter, compText);

    // Streak Badge (left of compliance badge)
    QRect streakRect(compRect.left() - streakW - 8, 11, streakW, badgeH);
    painter.setBrush(QColor(245, 158, 11, 30));
    painter.setPen(QPen(QColor(245, 158, 11, 110), 1));
    painter.drawRoundedRect(streakRect, 6, 6);
    painter.setPen(QColor("#fbbf24"));
    painter.drawText(streakRect, Qt::AlignCenter, streakText);

    // 3. Bars Chart Area
    int chartTop = 56;
    int chartBottom = h - 48; // Leaves room for day labels and legend
    int chartHeight = chartBottom - chartTop;
    QRect chartArea(18, chartTop, w - 36, chartHeight);

    int totalBars = m_cachedStats.size();
    if (totalBars == 0) return;

    // Find max total count for scale
    int maxBreaks = 6;
    for (const auto& ds : m_cachedStats) {
        int sum = ds.completed + ds.snoozed + ds.skipped;
        if (sum > maxBreaks) maxBreaks = sum;
    }

    // Ceiling with generous headroom so bar tops and count numbers never reach the header
    int ceiling = std::max(maxBreaks + 2, (maxBreaks * 12) / 10 + 1);
    int maxBarHeight = chartHeight - 24; // Guaranteed 24px buffer below chartTop

    QDate today = QDate::currentDate();

    for (int i = 0; i < totalBars; ++i) {
        const auto& ds = m_cachedStats.at(i);
        QRect bRect = barRectForIndex(i, totalBars, chartArea);
        bool isHovered = (i == m_hoveredIndex);
        QDate dayDate = QDate::fromString(ds.date, Qt::ISODate);
        bool isToday = (dayDate == today);

        // Draw track slot background
        QPainterPath trackPath;
        trackPath.addRoundedRect(bRect, 4, 4);
        painter.fillPath(trackPath, isHovered ? QColor("#243249") : QColor("#0f172a"));

        // Compute segment heights
        int totalDayBreaks = ds.completed + ds.snoozed + ds.skipped;
        int activeHeight = (totalDayBreaks > 0)
            ? std::max(6, static_cast<int>((static_cast<double>(totalDayBreaks) / ceiling) * maxBarHeight))
            : 0;

        if (activeHeight > 0) {
            int compH = static_cast<int>((static_cast<double>(ds.completed) / totalDayBreaks) * activeHeight);
            int snoozeH = static_cast<int>((static_cast<double>(ds.snoozed) / totalDayBreaks) * activeHeight);
            int skipH = activeHeight - compH - snoozeH;

            int curY = bRect.bottom();

            // Completed segment (Cyan)
            if (compH > 0) {
                QRect cRect(bRect.x(), curY - compH, bRect.width(), compH);
                QPainterPath cPath;
                cPath.addRoundedRect(cRect, 4, 4);
                painter.fillPath(cPath, isHovered ? QColor("#7dd3fc") : QColor("#38bdf8"));
                curY -= compH;
            }

            // Snoozed segment (Amber)
            if (snoozeH > 0) {
                QRect sRect(bRect.x(), curY - snoozeH, bRect.width(), snoozeH);
                QPainterPath sPath;
                sPath.addRoundedRect(sRect, 4, 4);
                painter.fillPath(sPath, isHovered ? QColor("#fde047") : QColor("#fbbf24"));
                curY -= snoozeH;
            }

            // Skipped segment (Soft Rose)
            if (skipH > 0) {
                QRect skRect(bRect.x(), curY - skipH, bRect.width(), skipH);
                QPainterPath skPath;
                skPath.addRoundedRect(skRect, 4, 4);
                painter.fillPath(skPath, isHovered ? QColor("#fb7185") : QColor("#f43f5e"));
            }
        }

        // Draw count on top of bar if breaks > 0 (well separated from header badges)
        if (totalDayBreaks > 0) {
            painter.setFont(QFont("Segoe UI", 8, QFont::DemiBold));
            painter.setPen(isHovered ? QColor("#f8fafc") : QColor("#94a3b8"));
            int textY = bRect.bottom() - activeHeight - 3;
            painter.drawText(QRect(bRect.x() - 6, textY - 14, bRect.width() + 12, 14), Qt::AlignCenter, QString::number(ds.completed));
        }

        // Day label below bar
        QString dayLabel = isToday ? "Today" : dayDate.toString("ddd");
        painter.setFont(QFont("Segoe UI", 8, isToday ? QFont::Bold : QFont::Normal));
        painter.setPen(isToday ? QColor("#38bdf8") : (isHovered ? QColor("#f8fafc") : QColor("#94a3b8")));
        QRect lblRect(bRect.x() - 10, chartBottom + 4, bRect.width() + 20, 16);
        painter.drawText(lblRect, Qt::AlignCenter, dayLabel);
    }

    // 4. Integrated Legend at Bottom
    int legY = h - 22;
    QFont legFont("Segoe UI", 8);
    painter.setFont(legFont);
    QFontMetrics legFm(legFont);

    struct LegItem { QString label; QColor color; };
    LegItem legItems[] = {
        {"Completed", QColor("#38bdf8")},
        {"Snoozed", QColor("#fbbf24")},
        {"Skipped", QColor("#f43f5e")}
    };

    int totalLegW = 0;
    for (const auto& item : legItems) {
        totalLegW += 8 + 6 + legFm.horizontalAdvance(item.label) + 16;
    }
    totalLegW -= 16;

    int curLegX = (w - totalLegW) / 2;
    for (const auto& item : legItems) {
        painter.setBrush(item.color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(curLegX, legY + 2, 7, 7);
        curLegX += 12;

        painter.setPen(QColor("#94a3b8"));
        int textW = legFm.horizontalAdvance(item.label);
        painter.drawText(curLegX, legY + 9, item.label);
        curLegX += textW + 16;
    }
}

void WeeklyAnalyticsWidget::mouseMoveEvent(QMouseEvent* event) {
    int w = width();
    int h = height();
    QRect chartArea(18, 56, w - 36, h - 104);
    int totalBars = m_cachedStats.size();

    int foundIdx = -1;
    for (int i = 0; i < totalBars; ++i) {
        QRect bRect = barRectForIndex(i, totalBars, chartArea);
        QRect hitRect(bRect.x() - 4, chartArea.y(), bRect.width() + 8, chartArea.height() + 20);
        if (hitRect.contains(event->pos())) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx != m_hoveredIndex) {
        m_hoveredIndex = foundIdx;
        update();

        if (m_hoveredIndex >= 0 && m_hoveredIndex < m_cachedStats.size()) {
            const auto& ds = m_cachedStats.at(m_hoveredIndex);
            QDate d = QDate::fromString(ds.date, Qt::ISODate);
            QString dayStr = (d == QDate::currentDate()) ? "Today" : d.toString("dddd, MMM d");
            double restMins = static_cast<double>(ds.restSeconds) / 60.0;

            QString tip = QString(
                "<b>%1</b> (%2)<br>"
                "• <span style='color:#38bdf8'>✓ Completed:</span> %3<br>"
                "• <span style='color:#fbbf24'>⏳ Snoozed:</span> %4<br>"
                "• <span style='color:#f43f5e'>✕ Skipped:</span> %5<br>"
                "• <span style='color:#10b981'>👁️ Rest Time:</span> %6m"
            ).arg(dayStr)
             .arg(ds.date)
             .arg(ds.completed)
             .arg(ds.snoozed)
             .arg(ds.skipped)
             .arg(restMins, 0, 'f', 1);

            QToolTip::showText(event->globalPosition().toPoint(), tip, this);
        } else {
            QToolTip::hideText();
        }
    }
}

void WeeklyAnalyticsWidget::leaveEvent(QEvent* event) {
    Q_UNUSED(event);
    m_hoveredIndex = -1;
    QToolTip::hideText();
    update();
}
