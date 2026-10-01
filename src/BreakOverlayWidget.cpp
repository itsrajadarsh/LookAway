#include "BreakOverlayWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QIcon>
#include <QKeyEvent>

BreakOverlayWidget::BreakOverlayWidget(DisplayMode mode, QWidget* parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool),
      m_mode(mode) {

    setAttribute(Qt::WA_TranslucentBackground);
    setFocusPolicy(Qt::StrongFocus);

    setupUi();
}

BreakOverlayWidget::DisplayMode BreakOverlayWidget::displayMode() const {
    return m_mode;
}

void BreakOverlayWidget::setupUi() {
    QVBoxLayout* outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    QFrame* bgFrame = new QFrame(this);
    bgFrame->setObjectName("breakOverlayCard");

    if (m_mode == DisplayMode::FullScreen) {
        bgFrame->setStyleSheet(R"(
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
        bgFrame->setStyleSheet(R"(
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

    QVBoxLayout* layout = new QVBoxLayout(bgFrame);
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

    outerLayout->addWidget(bgFrame);
}

void BreakOverlayWidget::updateCountdown(int secondsRemaining, int totalSeconds) {
    int mins = secondsRemaining / 60;
    int secs = secondsRemaining % 60;
    m_lblCountdown->setText(QString("%1:%2")
                                .arg(mins, 2, 10, QChar('0'))
                                .arg(secs, 2, 10, QChar('0')));

    if (totalSeconds > 0) {
        int pct = static_cast<int>((static_cast<double>(secondsRemaining) / totalSeconds) * 100.0);
        m_progressBar->setValue(pct);
    }
}

void BreakOverlayWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        emit skipRequested();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void BreakOverlayWidget::mousePressEvent(QMouseEvent* event) {
    if (m_mode == DisplayMode::CenteredPopup && event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void BreakOverlayWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_mode == DisplayMode::CenteredPopup && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}
