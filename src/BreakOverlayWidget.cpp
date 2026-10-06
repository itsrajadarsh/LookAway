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
      m_skipDisabled(false),
      m_postponeVisible(true),
      m_postponeSeconds(120),
      m_isMacroBreak(false) {

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
    if (m_mode != DisplayMode::BorderGlow) {
        applyTip(ErgonomicTipCatalog::getRandomTip(false));
    }
}

BreakOverlayWidget::DisplayMode BreakOverlayWidget::displayMode() const {
    return m_mode;
}

void BreakOverlayWidget::setSkipDisabled(bool disabled) {
    m_skipDisabled = disabled;
    if (m_btnSkip) {
        m_btnSkip->setVisible(!disabled);
    }
    if (m_btnPostpone) {
        m_btnPostpone->setVisible(!disabled && m_postponeVisible);
    }
    if (disabled) {
        m_enforceTopTimer->start();
    } else {
        m_enforceTopTimer->stop();
    }
}

void BreakOverlayWidget::setPostponeVisible(bool visible) {
    m_postponeVisible = visible;
    if (m_btnPostpone) {
        m_btnPostpone->setVisible(!m_skipDisabled && m_postponeVisible);
    }
}

void BreakOverlayWidget::setPostponeSeconds(int seconds) {
    m_postponeSeconds = seconds;
    if (m_btnPostpone) {
        int mins = seconds / 60;
        if (mins > 0 && (seconds % 60 == 0)) {
            m_btnPostpone->setText(QString("Snooze %1m (S)").arg(mins));
        } else {
            m_btnPostpone->setText(QString("Snooze %1s (S)").arg(seconds));
        }
    }
}

void BreakOverlayWidget::setBreakContext(bool isMacroBreak) {
    m_isMacroBreak = isMacroBreak;
    applyTip(ErgonomicTipCatalog::getRandomTip(isMacroBreak));
}

void BreakOverlayWidget::cycleNextTip() {
    applyTip(ErgonomicTipCatalog::getNextTip(m_currentTip.title, m_isMacroBreak));
}

void BreakOverlayWidget::applyTip(const ErgonomicTip& tip) {
    m_currentTip = tip;
    if (m_lblCategoryBadge) {
        m_lblCategoryBadge->setText(tip.category);
    }
    if (m_lblTitle) {
        m_lblTitle->setText(tip.title);
    }
    if (m_lblSubtitle) {
        m_lblSubtitle->setText(tip.instruction);
    }
    if (m_lblBenefit) {
        m_lblBenefit->setText(QString("💡 %1").arg(tip.benefit));
    }
}

void BreakOverlayWidget::setupUi() {
    if (m_mode == DisplayMode::BorderGlow) {
        // Pure ambient perimeter highlight
        m_lblCategoryBadge = nullptr;
        m_lblTitle = nullptr;
        m_lblSubtitle = nullptr;
        m_lblBenefit = nullptr;
        m_lblCountdown = nullptr;
        m_progressBar = nullptr;
        m_btnNextTip = nullptr;
        m_btnPostpone = nullptr;
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
                background-color: rgba(15, 23, 42, 238);
                border: none;
            }
            QLabel {
                background: transparent;
                border: none;
            }
        )");
    } else {
        // Centered popup: comfortable 520x370 card
        setFixedSize(520, 370);
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
    layout->setSpacing(m_mode == DisplayMode::FullScreen ? 12 : 8);
    int padH = (m_mode == DisplayMode::FullScreen) ? 50 : 26;
    int padV = (m_mode == DisplayMode::FullScreen) ? 40 : 20;
    layout->setContentsMargins(padH, padV, padH, padV);

    // Eye icon
    QLabel* iconLabel = new QLabel();
    int iconSize = (m_mode == DisplayMode::FullScreen) ? 56 : 38;
    iconLabel->setPixmap(QIcon(":/icons/app_icon.svg").pixmap(iconSize, iconSize));
    iconLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(iconLabel);

    // Category Pill Badge
    m_lblCategoryBadge = new QLabel("EYE RELIEF");
    m_lblCategoryBadge->setAlignment(Qt::AlignCenter);
    m_lblCategoryBadge->setStyleSheet(R"(
        background-color: rgba(56, 189, 248, 0.16);
        color: #38bdf8;
        border: 1px solid rgba(56, 189, 248, 0.35);
        border-radius: 9px;
        padding: 3px 12px;
        font-size: 11px;
        font-weight: 800;
        letter-spacing: 1px;
    )");
    layout->addWidget(m_lblCategoryBadge, 0, Qt::AlignCenter);

    // Tip Title
    m_lblTitle = new QLabel("The 20-20-20 Rule");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblTitle->setStyleSheet("font-size: 24px; font-weight: 800; color: #f8fafc; letter-spacing: 1px;");
    } else {
        m_lblTitle->setStyleSheet("font-size: 17px; font-weight: 800; color: #f8fafc; letter-spacing: 0.5px;");
    }
    m_lblTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_lblTitle);

    // Tip Instruction / Subtitle
    m_lblSubtitle = new QLabel("Look at an object at least 20 feet away to relax your eyes.");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblSubtitle->setStyleSheet("font-size: 15px; color: #cbd5e1; font-weight: 500; line-height: 1.4;");
        m_lblSubtitle->setMaximumWidth(680);
    } else {
        m_lblSubtitle->setStyleSheet("font-size: 12px; color: #cbd5e1; font-weight: 500;");
        m_lblSubtitle->setMaximumWidth(460);
    }
    m_lblSubtitle->setAlignment(Qt::AlignCenter);
    m_lblSubtitle->setWordWrap(true);
    layout->addWidget(m_lblSubtitle);

    // Clinical Benefit
    m_lblBenefit = new QLabel("💡 Releases ciliary muscle tension and prevents accommodative fatigue.");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblBenefit->setStyleSheet("font-size: 13px; color: #38bdf8; font-weight: 600; font-style: italic;");
        m_lblBenefit->setMaximumWidth(680);
    } else {
        m_lblBenefit->setStyleSheet("font-size: 11px; color: #38bdf8; font-weight: 600; font-style: italic;");
        m_lblBenefit->setMaximumWidth(460);
    }
    m_lblBenefit->setAlignment(Qt::AlignCenter);
    m_lblBenefit->setWordWrap(true);
    layout->addWidget(m_lblBenefit);

    // Countdown Display
    m_lblCountdown = new QLabel("00:20");
    if (m_mode == DisplayMode::FullScreen) {
        m_lblCountdown->setStyleSheet("font-size: 60px; font-weight: 900; color: #ffffff; font-family: 'Consolas', monospace;");
    } else {
        m_lblCountdown->setStyleSheet("font-size: 38px; font-weight: 900; color: #ffffff; font-family: 'Consolas', monospace;");
    }
    m_lblCountdown->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_lblCountdown);

    // Progress Bar
    m_progressBar = new QProgressBar();
    m_progressBar->setFixedWidth(m_mode == DisplayMode::FullScreen ? 360 : 280);
    m_progressBar->setFixedHeight(m_mode == DisplayMode::FullScreen ? 8 : 6);
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

    // Action Buttons Bar
    QHBoxLayout* buttonBar = new QHBoxLayout();
    buttonBar->setSpacing(m_mode == DisplayMode::FullScreen ? 12 : 8);
    buttonBar->setAlignment(Qt::AlignCenter);

    // Cycle Next Routine Button
    m_btnNextTip = new QPushButton("Next Tip ↻");
    m_btnNextTip->setCursor(Qt::PointingHandCursor);
    m_btnNextTip->setToolTip("Cycle to another guided exercise (R or Space)");
    m_btnNextTip->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(30, 41, 59, 220);
            color: #94a3b8;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background-color: rgba(51, 65, 85, 240);
            color: #f8fafc;
            border-color: #64748b;
        }
    )");
    connect(m_btnNextTip, &QPushButton::clicked, this, &BreakOverlayWidget::cycleNextTip);
    buttonBar->addWidget(m_btnNextTip);

    // Postpone / Snooze Button
    m_btnPostpone = new QPushButton(QString("Snooze %1m (S)").arg(m_postponeSeconds / 60));
    m_btnPostpone->setCursor(Qt::PointingHandCursor);
    m_btnPostpone->setToolTip("Postpone this break by a few minutes without counting as skipped (S)");
    m_btnPostpone->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(217, 119, 6, 0.22);
            color: #fbbf24;
            border: 1px solid rgba(245, 158, 11, 0.45);
            border-radius: 6px;
            font-size: 12px;
            font-weight: 700;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: rgba(217, 119, 6, 0.38);
            border-color: #f59e0b;
            color: #fef3c7;
        }
    )");
    connect(m_btnPostpone, &QPushButton::clicked, this, [this]() {
        emit postponeRequested(m_postponeSeconds);
    });
    buttonBar->addWidget(m_btnPostpone);

    // Skip Break Button
    m_btnSkip = new QPushButton("Skip (Esc)");
    m_btnSkip->setCursor(Qt::PointingHandCursor);
    m_btnSkip->setStyleSheet(R"(
        QPushButton {
            background-color: rgba(51, 65, 85, 200);
            color: #f8fafc;
            border: 1px solid #475569;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
            padding: 6px 14px;
        }
        QPushButton:hover {
            background-color: rgba(71, 85, 105, 240);
        }
    )");
    connect(m_btnSkip, &QPushButton::clicked, this, &BreakOverlayWidget::skipRequested);
    buttonBar->addWidget(m_btnSkip);

    layout->addLayout(buttonBar);

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
    } else if (event->key() == Qt::Key_S) {
        if (!m_skipDisabled && m_postponeVisible) {
            emit postponeRequested(m_postponeSeconds);
        }
    } else if (event->key() == Qt::Key_Space || event->key() == Qt::Key_R) {
        cycleNextTip();
    } else {
        QWidget::keyPressEvent(event);
    }
}
