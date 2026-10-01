#include "SystemTrayManager.h"
#include <QCoreApplication>
#include <QIcon>

SystemTrayManager::SystemTrayManager(TimerEngine* timerEngine, SettingsManager* settings, QObject* parent)
    : QObject(parent),
      m_timerEngine(timerEngine),
      m_settings(settings),
      m_trayIcon(nullptr),
      m_trayMenu(nullptr) {

    m_trayIcon = new QSystemTrayIcon(QIcon(":/icons/tray_work.svg"), this);
    createTrayMenu();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &SystemTrayManager::handleTrayActivated);
    connect(m_timerEngine, &TimerEngine::stateChanged, this, &SystemTrayManager::handleStateChanged);
    connect(m_timerEngine, &TimerEngine::tick, this, &SystemTrayManager::handleTick);
    connect(m_timerEngine, &TimerEngine::workCompleted, this, &SystemTrayManager::handleWorkCompleted);
    connect(m_timerEngine, &TimerEngine::breakCompleted, this, &SystemTrayManager::handleBreakCompleted);

    updateTrayIcon();
    m_trayIcon->show();
}

void SystemTrayManager::createTrayMenu() {
    m_trayMenu = new QMenu();

    m_actionShowDashboard = m_trayMenu->addAction(QIcon(":/icons/app_icon.svg"), "Show Dashboard", this, &SystemTrayManager::showDashboardRequested);
    m_trayMenu->addSeparator();

    m_actionTogglePlayPause = m_trayMenu->addAction("Start Timer", [this]() {
        if (m_timerEngine->state() == TimerEngine::State::Working || m_timerEngine->state() == TimerEngine::State::Breaking) {
            m_timerEngine->pause();
        } else {
            m_timerEngine->start();
        }
    });

    m_actionSkipBreak = m_trayMenu->addAction("Skip Break", [this]() {
        m_timerEngine->skipBreak();
    });

    m_actionSettings = m_trayMenu->addAction("Settings...", this, &SystemTrayManager::showSettingsRequested);
    m_trayMenu->addSeparator();

    m_actionQuit = m_trayMenu->addAction("Quit LookAway", QCoreApplication::instance(), &QCoreApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
}

void SystemTrayManager::updateTrayIcon() {
    switch (m_timerEngine->state()) {
    case TimerEngine::State::Working:
        m_trayIcon->setIcon(QIcon(":/icons/tray_work.svg"));
        m_actionTogglePlayPause->setText("Pause Timer");
        m_actionSkipBreak->setEnabled(false);
        break;
    case TimerEngine::State::Breaking:
        m_trayIcon->setIcon(QIcon(":/icons/tray_break.svg"));
        m_actionTogglePlayPause->setText("Pause Break");
        m_actionSkipBreak->setEnabled(true);
        break;
    case TimerEngine::State::Paused:
        m_trayIcon->setIcon(QIcon(":/icons/tray_paused.svg"));
        m_actionTogglePlayPause->setText("Resume Timer");
        m_actionSkipBreak->setEnabled(true);
        break;
    case TimerEngine::State::Idle:
    default:
        m_trayIcon->setIcon(QIcon(":/icons/tray_work.svg"));
        m_actionTogglePlayPause->setText("Start Timer");
        m_actionSkipBreak->setEnabled(false);
        break;
    }
}

void SystemTrayManager::handleStateChanged(TimerEngine::State newState, TimerEngine::State oldState) {
    Q_UNUSED(newState);
    Q_UNUSED(oldState);
    updateTrayIcon();
}

void SystemTrayManager::handleTick(int secondsRemaining, int totalSeconds) {
    Q_UNUSED(totalSeconds);
    QString stateStr;
    switch (m_timerEngine->state()) {
    case TimerEngine::State::Working:
        stateStr = "Work Session";
        break;
    case TimerEngine::State::Breaking:
        stateStr = "Break Time";
        break;
    case TimerEngine::State::Paused:
        stateStr = "Paused";
        break;
    case TimerEngine::State::Idle:
        stateStr = "Ready";
        break;
    }

    QString timeStr;
    if (secondsRemaining >= 3600) {
        int hrs = secondsRemaining / 3600;
        int mins = (secondsRemaining % 3600) / 60;
        int secs = secondsRemaining % 60;
        timeStr = QString("%1:%2:%3")
                      .arg(hrs, 2, 10, QChar('0'))
                      .arg(mins, 2, 10, QChar('0'))
                      .arg(secs, 2, 10, QChar('0'));
    } else {
        int mins = secondsRemaining / 60;
        int secs = secondsRemaining % 60;
        timeStr = QString("%1:%2")
                      .arg(mins, 2, 10, QChar('0'))
                      .arg(secs, 2, 10, QChar('0'));
    }
    QString tooltip = QString("LookAway - %1\n%2 remaining")
                          .arg(stateStr)
                          .arg(timeStr);
    m_trayIcon->setToolTip(tooltip);
}

void SystemTrayManager::handleWorkCompleted() {
    if (m_settings->notificationsEnabled()) {
        showNotification("Time to Look Away! 👁️",
                         "Take a 20-second break! Look at something 20 feet (6m) away.",
                         QSystemTrayIcon::Information);
    }
}

void SystemTrayManager::handleBreakCompleted() {
    if (m_settings->notificationsEnabled()) {
        showNotification("Break Complete! ✨",
                         "Great job giving your eyes a rest. Resuming work session.",
                         QSystemTrayIcon::Information);
    }
}

void SystemTrayManager::showNotification(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, icon, 5000);
    }
}

void SystemTrayManager::handleTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        emit showDashboardRequested();
    }
}
