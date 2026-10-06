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
    connect(m_timerEngine, &TimerEngine::preBreakWarning, this, &SystemTrayManager::handlePreBreakWarning);
    connect(m_timerEngine, &TimerEngine::breakPostponed, this, [this](int seconds) {
        if (m_settings->notificationsEnabled()) {
            int mins = seconds / 60;
            QString msg = (mins > 0 && seconds % 60 == 0)
                ? QString("Break postponed by %1 minute%2.").arg(mins).arg(mins > 1 ? "s" : "")
                : QString("Break postponed by %1 seconds.").arg(seconds);
            showNotification("Break Postponed ⏳", msg, QSystemTrayIcon::Information);
        }
    });
    connect(m_timerEngine, &TimerEngine::workCompleted, this, &SystemTrayManager::handleWorkCompleted);
    connect(m_timerEngine, &TimerEngine::breakCompleted, this, &SystemTrayManager::handleBreakCompleted);
    connect(m_timerEngine, &TimerEngine::dndStateChanged, this, [this](bool, int) {
        updateTrayIcon();
    });
    connect(m_timerEngine, &TimerEngine::dndExpired, this, [this]() {
        if (m_settings->notificationsEnabled()) {
            showNotification("Do Not Disturb Ended ✨", "Resuming your regular eye care schedule.", QSystemTrayIcon::Information);
        }
    });

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

    m_actionPostponeBreak = m_trayMenu->addAction("Snooze Break (2m)", [this]() {
        m_timerEngine->postponeBreak(m_settings->defaultPostponeSeconds());
    });

    m_actionSkipBreak = m_trayMenu->addAction("Skip Break", [this]() {
        m_timerEngine->skipBreak();
    });

    m_dndMenu = m_trayMenu->addMenu("Do Not Disturb");
    m_actionDnd30m = m_dndMenu->addAction("30 Minutes", [this]() { m_timerEngine->enableDnd(1800); });
    m_actionDnd1h = m_dndMenu->addAction("1 Hour", [this]() { m_timerEngine->enableDnd(3600); });
    m_actionDnd2h = m_dndMenu->addAction("2 Hours", [this]() { m_timerEngine->enableDnd(7200); });
    m_actionDndIndefinite = m_dndMenu->addAction("Until Turned Off", [this]() { m_timerEngine->enableDnd(-1); });
    m_dndMenu->addSeparator();
    m_actionDndDisable = m_dndMenu->addAction("End Do Not Disturb", [this]() { m_timerEngine->disableDnd(); });

    m_actionSettings = m_trayMenu->addAction("Settings...", this, &SystemTrayManager::showSettingsRequested);
    m_trayMenu->addSeparator();

    m_actionQuit = m_trayMenu->addAction("Quit LookAway", QCoreApplication::instance(), &QCoreApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);
}

void SystemTrayManager::updateTrayIcon() {
    bool canSkip = !m_settings->forceDisableSkip();
    bool canPostpone = m_settings->postponeEnabled() && canSkip;
    int snoozeMins = m_settings->defaultPostponeSeconds() / 60;
    m_actionPostponeBreak->setText(QString("Snooze Break (%1m)").arg(snoozeMins > 0 ? snoozeMins : 2));

    if (m_timerEngine->isDndActive()) {
        m_trayIcon->setIcon(QIcon(":/icons/tray_paused.svg"));
        m_actionTogglePlayPause->setEnabled(false);
        m_actionPostponeBreak->setEnabled(false);
        m_actionSkipBreak->setEnabled(false);
        m_actionDndDisable->setEnabled(true);
        return;
    }

    m_actionDndDisable->setEnabled(false);
    m_actionTogglePlayPause->setEnabled(true);

    switch (m_timerEngine->state()) {
    case TimerEngine::State::Working:
        m_trayIcon->setIcon(QIcon(":/icons/tray_work.svg"));
        m_actionTogglePlayPause->setText("Pause Timer");
        m_actionPostponeBreak->setEnabled(canPostpone);
        m_actionSkipBreak->setEnabled(false);
        break;
    case TimerEngine::State::Breaking:
        m_trayIcon->setIcon(QIcon(":/icons/tray_break.svg"));
        m_actionTogglePlayPause->setText("Pause Break");
        m_actionPostponeBreak->setEnabled(canPostpone);
        m_actionSkipBreak->setEnabled(canSkip);
        break;
    case TimerEngine::State::Paused:
        m_trayIcon->setIcon(QIcon(":/icons/tray_paused.svg"));
        m_actionTogglePlayPause->setText("Resume Timer");
        m_actionPostponeBreak->setEnabled(canPostpone);
        m_actionSkipBreak->setEnabled(canSkip);
        break;
    case TimerEngine::State::Idle:
    default:
        m_trayIcon->setIcon(QIcon(":/icons/tray_work.svg"));
        m_actionTogglePlayPause->setText("Start Timer");
        m_actionPostponeBreak->setEnabled(false);
        m_actionSkipBreak->setEnabled(false);
        break;
    }
}

void SystemTrayManager::handlePreBreakWarning(int secondsUntilBreak, TimerEngine::ActiveBreakType breakType) {
    if (m_settings->notificationsEnabled() && m_settings->preBreakWarningEnabled()) {
        QString title = (breakType == TimerEngine::ActiveBreakType::Secondary)
            ? "Upcoming Macro Break ☕"
            : "Upcoming Eye Rest Break 👁️";
        QString msg = QString("Break begins in %1 seconds. Finish your current thought and look away.").arg(secondsUntilBreak);
        showNotification(title, msg, QSystemTrayIcon::Information);
    }
}

void SystemTrayManager::handleStateChanged(TimerEngine::State newState, TimerEngine::State oldState) {
    Q_UNUSED(newState);
    Q_UNUSED(oldState);
    updateTrayIcon();
}

void SystemTrayManager::handleTick(int secondsRemaining, int totalSeconds) {
    Q_UNUSED(totalSeconds);

    if (m_timerEngine->isDndActive()) {
        QString tooltip = QString("LookAway - Do Not Disturb\n%1 remaining")
                              .arg(m_timerEngine->formattedDndTimeRemaining());
        m_trayIcon->setToolTip(tooltip);
        return;
    }

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
