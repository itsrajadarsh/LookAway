#include "BreakOverlayWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QKeyEvent>
#include <QPainter>
#include <QLinearGradient>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

BreakOverlayWidget::BreakOverlayWidget(DisplayMode mode, QWidget* parent)
    : QWidget(parent, [mode]() {
          Qt::WindowFlags f = Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool;
          if (mode == DisplayMode::BorderGlow) {
              f |= Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput;
          }
          return f;
      }()),
      m_mode(mode),
      m_skipDisabled(false) {

    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    if (m_mode == DisplayMode::BorderGlow) {
        setAttribute(Qt::WA_ShowWithoutActivating, true);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setFocusPolicy(Qt::NoFocus);
        setStyleSheet("background: transparent;");

        m_glowTimer = new QTimer(this);
        m_glowTimer->setInterval(33); // ~30 fps smooth pulse
        connect(m_glowTimer, &QTimer::timeout, this, [this]() {
            m_glowPhase += 0.05f;
            update();
        });
        m_glowTimer->start();
    } else {
        setFocusPolicy(Qt::StrongFocus);
    }

#ifdef Q_OS_WIN
    if (m_mode == DisplayMode::BorderGlow) {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        exStyle |= WS_EX_NOACTIVATE | WS_EX_TRANSPARENT;
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
    }
#endif

    m_enforceTopTimer = new QTimer(this);
    m_enforceTopTimer->setInterval(250);
    connect(m_enforceTopTimer, &QTimer::timeout, this, [this]() {
        if (m_skipDisabled) {
            raise();
#ifdef Q_OS_WIN
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_TOPMOST, 0, 0, 0, 0, 
                         SWP_NOMOVE | SWP_NOSIZE | (m_mode == DisplayMode::BorderGlow ? SWP_NOACTIVATE : 0));
#endif
            if (m_mode != DisplayMode::BorderGlow) {
                activateWindow();
            }
        }
    });

    setupUi();
}

BreakOverlayWidget::DisplayMode BreakOverlayWidget::displayMode() const {
    return m_mode;
}

void BreakOverlayWidget::setSkipDisabled(bool disabled) {
    m_skipDisabled = disabled;
    if (m_btnSkip) {
        m_btnSkip->setVisible(!disabled);
    }
    if (disabled) {
        m_enforceTopTimer->start();
    } else {
        m_enforceTopTimer->stop();
    }
}

void BreakOverlayWidget::setupUi() {
    if (m_mode == DisplayMode::BorderGlow) {
        // Pure ambient perimeter highlight: transparent canvas, painted directly in paintEvent
        m_lblTitle = nullptr;
        m_lblSubtitle = nullptr;
        m_lblCountdown = nullptr;
        m_progressBar = nullptr;
        m_btnSkip = nullptr;
        return;
    }

    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    m_bgFrame = new QFrame(this);
    m_bgFrame->setObjectName("breakOverlayCard");

    if (m_mode == DisplayMode::FullScreen) {
        m_bgFrame->setStyleSheet(R"(
            QFrame#breakOverlayCard {
                background-color: rgba(15, 23, 42, 235);
                border: none;
            }
            QLabel {
                background: transparent;
                border: none;
            }
        )");
    } else {
        // Centered popup: fixed 460x300 card, clean border and rounded corners
        setFixedSize(460, 300);
        m_bgFrame->setStyleSheet(R"(
            QFrame#breakOverlayCard {
                background-color: #1e293b;
                border: 2px solid #38bdf8;
                border-radius: 16px;
            }
            QLabel {
                background: transparent;
                border: none;
            }
        )");
    }

    QVBoxLayout* layout = new QVBoxLayout(m_bgFrame);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(m_mode == DisplayMode::FullScreen ? 18 : 10);
    int padH = (m_mode == DisplayMode::FullScreen) ? 40 : 24;
    int padV = (m_mode == DisplayMode::FullScreen) ? 40 : 18;
    layout->setContentsMargins(padH, padV, padH, padV);

    // Eye icon
    QLabel* iconLabel = new QLabel();
    int iconSize = (m_mode == DisplayMode::FullScreen) ? 72 : 44;
    iconLabel->setPixmap(QIcon(":/icons/app_icon.svg").pixmap(iconSize, iconSize));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    m_lblTitle = new QLabel("TIME FOR AN EYE BREAK");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblTitle->setStyleSheet("font-size: 26px; font-weight: 800; color: #38bdf8; letter-spacing: 2px;");
    } else {
        m_lblTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #38bdf8; letter-spacing: 1px;");
    }
    m_lblTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_lblTitle);

    m_lblSubtitle = new QLabel("Look at an object at least 20 feet (6 meters) away to relax your eye muscles.");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblSubtitle->setStyleSheet("font-size: 15px; color: #94a3b8; font-weight: 500;");
    } else {
        m_lblSubtitle->setStyleSheet("font-size: 13px; color: #94a3b8; font-weight: 500;");
    }
    m_lblSubtitle->setAlignment(Qt::AlignCenter);
    m_lblSubtitle->setWordWrap(true);
    layout->addWidget(m_lblSubtitle);

    m_lblCountdown = new QLabel("00:20");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblCountdown->setStyleSheet("font-size: 68px; font-weight: 900; color: #ffffff; font-family: 'Consolas', monospace;");
    } else {
        m_lblCountdown->setStyleSheet("font-size: 44px; font-weight: 900; color: #ffffff; font-family: 'Consolas', monospace;");
    }
    m_lblCountdown->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_lblCountdown);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedWidth(m_mode == DisplayMode::FullScreen ? 360 : 280);
    m_progressBar->setFixedHeight(m_mode == DisplayMode::FullScreen ? 10 : 8);
    m_progressBar->setRange(0, 100);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background-color: #38bdf8;
            border-radius: 3px;
        }
    )");
    layout->addWidget(m_progressBar, 0, Qt::AlignCenter);

    m_btnSkip = new QPushButton("Skip Break (Esc)");
    m_btnSkip->setFixedSize(m_mode == DisplayMode::FullScreen ? 150 : 130, 
                            m_mode == DisplayMode::FullScreen ? 40 : 34);
    m_btnSkip->setCursor(Qt::PointingHandCursor);
    m_btnSkip->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(51, 65, 85, 200);
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 13px;
            font-weight: 600;
        }
        QPushButton:hover {
            background-color: rgba(71, 85, 105, 240);
        }
    )");
    connect(m_btnSkip, &QPushButton::clicked, this, &BreakOverlayWidget::skipRequested);
    layout->addWidget(m_btnSkip, 0, Qt::AlignCenter);

    outerLayout->addWidget(m_bgFrame);
}

void BreakOverlayWidget::paintEvent(QPaintEvent* event) {
    if (m_mode == DisplayMode::BorderGlow) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        int w = width();
        int h = height();

        float pulse = (std::sin(m_glowPhase) + 1.0f) * 0.5f; // 0.0 to 1.0
        int glowDepth = 28 + static_cast<int>(14 * pulse);   // 28px to 42px breathing depth
        int alpha = 110 + static_cast<int>(55 * pulse);       // 110 to 165
        int rimAlpha = 190 + static_cast<int>(55 * pulse);    // 190 to 245

        // Top edge inner gradient fade
        QLinearGradient topGrad(0, 0, 0, glowDepth);
        topGrad.setColorAt(0.0, QColor(56, 189, 248, alpha));
        topGrad.setColorAt(0.25, QColor(14, 165, 233, static_cast<int>(alpha * 0.65)));
        topGrad.setColorAt(0.7, QColor(2, 132, 199, static_cast<int>(alpha * 0.2)));
        topGrad.setColorAt(1.0, QColor(56, 189, 248, 0));
        painter.fillRect(0, 0, w, glowDepth, topGrad);

        // Bottom edge inner gradient fade
        QLinearGradient botGrad(0, h, 0, h - glowDepth);
        botGrad.setColorAt(0.0, QColor(56, 189, 248, alpha));
        botGrad.setColorAt(0.25, QColor(14, 165, 233, static_cast<int>(alpha * 0.65)));
        botGrad.setColorAt(0.7, QColor(2, 132, 199, static_cast<int>(alpha * 0.2)));
        botGrad.setColorAt(1.0, QColor(56, 189, 248, 0));
        painter.fillRect(0, h - glowDepth, w, glowDepth, botGrad);

        // Left edge inner gradient fade
        QLinearGradient leftGrad(0, 0, glowDepth, 0);
        leftGrad.setColorAt(0.0, QColor(56, 189, 248, alpha));
        leftGrad.setColorAt(0.25, QColor(14, 165, 233, static_cast<int>(alpha * 0.65)));
        leftGrad.setColorAt(0.7, QColor(2, 132, 199, static_cast<int>(alpha * 0.2)));
        leftGrad.setColorAt(1.0, QColor(56, 189, 248, 0));
        painter.fillRect(0, 0, glowDepth, h, leftGrad);

        // Right edge inner gradient fade
        QLinearGradient rightGrad(w, 0, w - glowDepth, 0);
        rightGrad.setColorAt(0.0, QColor(56, 189, 248, alpha));
        rightGrad.setColorAt(0.25, QColor(14, 165, 233, static_cast<int>(alpha * 0.65)));
        rightGrad.setColorAt(0.7, QColor(2, 132, 199, static_cast<int>(alpha * 0.2)));
        rightGrad.setColorAt(1.0, QColor(56, 189, 248, 0));
        painter.fillRect(w - glowDepth, 0, glowDepth, h, rightGrad);

        // Crisp neon outer rim (2px)
        QPen rimPen(QColor(56, 189, 248, rimAlpha), 2);
        painter.setPen(rimPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(1, 1, w - 2, h - 2);

        return;
    }
    QWidget::paintEvent(event);
}

void BreakOverlayWidget::updateCountdown(int secondsRemaining, int totalSeconds) {
    if (m_mode == DisplayMode::BorderGlow) return;
    if (!m_lblCountdown || !m_progressBar) return;

    QString timeText;
    if (secondsRemaining >= 3600) {
        int hrs = secondsRemaining / 3600;
        int mins = (secondsRemaining % 3600) / 60;
        int secs = secondsRemaining % 60;
        timeText = QString("%1:%2:%3")
                       .arg(hrs, 2, 10, QChar('0'))
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));
    } else {
        int mins = secondsRemaining / 60;
        int secs = secondsRemaining % 60;
        timeText = QString("%1:%2")
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));
    }
    m_lblCountdown->setText(timeText);

    if (totalSeconds > 0) {
        int pct = static_cast<int>((static_cast<double>(secondsRemaining) / totalSeconds) * 100.0);
        m_progressBar->setValue(pct);
    }
}

void BreakOverlayWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (!m_skipDisabled) {
            emit skipRequested();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}
