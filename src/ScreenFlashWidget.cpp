#include "ScreenFlashWidget.h"
#include <QPainter>
#include <QGuiApplication>
#include <QScreen>
#include <QEasingCurve>
#include <algorithm>

ScreenFlashWidget::ScreenFlashWidget(QWidget* parent)
    : QWidget(parent),
      m_flashOpacity(0.0),
      m_color(QColor("#38bdf8")) {

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool |
                   Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_ShowWithoutActivating);
}

qreal ScreenFlashWidget::flashOpacity() const {
    return m_flashOpacity;
}

void ScreenFlashWidget::setFlashOpacity(qreal opacity) {
    m_flashOpacity = std::clamp(opacity, 0.0, 1.0);
    update();
}

void ScreenFlashWidget::startFlash(const QString& style) {
    int duration = 400;
    qreal peak = 0.35;

    if (style == "amber") {
        m_color = QColor("#fbbf24");
        duration = 500;
        peak = 0.35;
    } else if (style == "white") {
        m_color = QColor("#ffffff");
        duration = 350;
        peak = 0.40;
    } else if (style == "double") {
        m_color = QColor("#38bdf8");
        duration = 700;
        peak = 0.35;
    } else { // "cyan" default
        m_color = QColor("#38bdf8");
        duration = 400;
        peak = 0.35;
    }

    show();
    raise();

    QPropertyAnimation* anim = new QPropertyAnimation(this, "flashOpacity", this);
    anim->setDuration(duration);
    anim->setStartValue(0.0);

    if (style == "double") {
        anim->setKeyValueAt(0.20, peak);
        anim->setKeyValueAt(0.40, 0.05);
        anim->setKeyValueAt(0.65, peak);
        anim->setKeyValueAt(0.85, 0.05);
    } else {
        anim->setKeyValueAt(0.35, peak);
    }

    anim->setEndValue(0.0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(anim, &QPropertyAnimation::finished, this, [this]() {
        hide();
        deleteLater();
    });

    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ScreenFlashWidget::flashAllScreens(const QString& style) {
    const QList<QScreen*> screens = QGuiApplication::screens();
    for (QScreen* screen : screens) {
        if (!screen) continue;
        ScreenFlashWidget* flashWidget = new ScreenFlashWidget();
        flashWidget->setGeometry(screen->geometry());
        flashWidget->startFlash(style);
    }
}

void ScreenFlashWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    if (m_flashOpacity <= 0.001) return;

    QPainter painter(this);
    QColor c = m_color;
    c.setAlphaF(m_flashOpacity);
    painter.fillRect(rect(), c);
}
