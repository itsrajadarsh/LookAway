#include "TimerEngine.h"
#include "FullscreenDetector.h"
#include <QTime>

#ifdef Q_OS_WIN
#include <windows.h>

static qint64 getSystemIdleTimeMs() {
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        DWORD tickCount = GetTickCount();
        return static_cast<qint64>(tickCount - lii.dwTime);
    }
    return 0;
}
#endif

TimerEngine::TimerEngine(SettingsManager* settings, QObject* parent)
    : QObject(parent),
      m_settings(settings),
      m_state(State::Idle),
      m_previousState(State::Idle),
      m_activeBreakType(ActiveBreakType::None),
      m_secondsRemaining(0),
      m_totalDurationSeconds(0),
      m_secondarySecondsRemaining(0),
      m_secondaryTotalDurationSeconds(0),
      m_wasPausedForIdle(false) {

    connect(&m_timer, &QTimer::timeout, this, &TimerEngine::handleOneSecondTick);
    connect(m_settings, &SettingsManager::settingsChanged, this, &TimerEngine::handleSettingsChanged);

    m_secondsRemaining = m_settings->workDurationSeconds();
    m_totalDurationSeconds = m_secondsRemaining;
    m_secondarySecondsRemaining = m_settings->secondaryWorkDurationSeconds();
    m_secondaryTotalDurationSeconds = m_secondarySecondsRemaining;
}

TimerEngine::State TimerEngine::state() const {
    return m_state;
}

TimerEngine::ActiveBreakType TimerEngine::activeBreakType() const {
    return m_activeBreakType;
}

int TimerEngine::secondsRemaining() const {
    return m_secondsRemaining;
}

int TimerEngine::totalDurationSeconds() const {
    return m_totalDurationSeconds;
}

int TimerEngine::secondarySecondsRemaining() const {
    return m_secondarySecondsRemaining;
}

int TimerEngine::secondaryTotalDurationSeconds() const {
    return m_secondaryTotalDurationSeconds;
}

bool TimerEngine::isPausedForIdle() const {
    return m_wasPausedForIdle;
}

QString TimerEngine::formattedTimeRemaining() const {
    if (m_secondsRemaining >= 3600) {
        int hrs = m_secondsRemaining / 3600;
        int mins = (m_secondsRemaining % 3600) / 60;
        int secs = m_secondsRemaining % 60;
        return QString("%1:%2:%3")
            .arg(hrs, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    int mins = m_secondsRemaining / 60;
    int secs = m_secondsRemaining % 60;
    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

QString TimerEngine::formattedSecondaryTimeRemaining() const {
    if (m_secondarySecondsRemaining >= 3600) {
        int hrs = m_secondarySecondsRemaining / 3600;
        int mins = (m_secondarySecondsRemaining % 3600) / 60;
        int secs = m_secondarySecondsRemaining % 60;
        return QString("%1:%2:%3")
            .arg(hrs, 2, 10, QChar('0'))
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    int mins = m_secondarySecondsRemaining / 60;
    int secs = m_secondarySecondsRemaining % 60;
    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

void TimerEngine::start() {
    m_wasPausedForIdle = false;
    if (m_state == State::Idle || m_state == State::Paused) {
        if (m_state == State::Idle) {
            m_totalDurationSeconds = m_settings->workDurationSeconds();
            m_secondsRemaining = m_totalDurationSeconds;
            m_secondaryTotalDurationSeconds = m_settings->secondaryWorkDurationSeconds();
            m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;
            m_activeBreakType = ActiveBreakType::None;
            setState(State::Working);
        } else {
            setState(m_previousState == State::Idle ? State::Working : m_previousState);
        }
        m_timer.start(1000);
        emit tick(m_secondsRemaining, m_totalDurationSeconds);
        emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
    }
}

void TimerEngine::pause() {
    m_wasPausedForIdle = false;
    if (m_state == State::Working || m_state == State::Breaking) {
        m_timer.stop();
        m_previousState = m_state;
        setState(State::Paused);
    }
}

void TimerEngine::resume() {
    m_wasPausedForIdle = false;
    if (m_state == State::Paused) {
        setState(m_previousState == State::Idle ? State::Working : m_previousState);
        m_timer.start(1000);
        int rem = (m_activeBreakType == ActiveBreakType::Secondary) ? m_secondarySecondsRemaining : m_secondsRemaining;
        int tot = (m_activeBreakType == ActiveBreakType::Secondary) ? m_secondaryTotalDurationSeconds : m_totalDurationSeconds;
        emit tick(rem, tot);
        emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
    }
}

void TimerEngine::stop() {
    m_wasPausedForIdle = false;
    m_timer.stop();
    m_previousState = State::Idle;
    m_activeBreakType = ActiveBreakType::None;
    m_totalDurationSeconds = m_settings->workDurationSeconds();
    m_secondsRemaining = m_totalDurationSeconds;
    m_secondaryTotalDurationSeconds = m_settings->secondaryWorkDurationSeconds();
    m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;
    setState(State::Idle);
    emit tick(m_secondsRemaining, m_totalDurationSeconds);
    emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
}

void TimerEngine::skipBreak() {
    if (m_settings->forceDisableSkip()) {
        return; // Skip break is strictly disabled
    }

    if (m_state == State::Breaking || (m_state == State::Paused && m_previousState == State::Breaking)) {
        m_settings->incrementBreaksSkipped();
        m_wasPausedForIdle = false;
        m_timer.stop();

        if (m_activeBreakType == ActiveBreakType::Secondary) {
            m_secondaryTotalDurationSeconds = m_settings->secondaryWorkDurationSeconds();
            m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;
            m_totalDurationSeconds = m_settings->workDurationSeconds();
            m_secondsRemaining = m_totalDurationSeconds;
        } else {
            m_totalDurationSeconds = m_settings->workDurationSeconds();
            m_secondsRemaining = m_totalDurationSeconds;
        }

        m_activeBreakType = ActiveBreakType::None;
        setState(State::Working);
        m_timer.start(1000);
        emit tick(m_secondsRemaining, m_totalDurationSeconds);
        emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
    }
}

void TimerEngine::setState(State newState) {
    if (m_state != newState) {
        State oldState = m_state;
        m_state = newState;
        emit stateChanged(m_state, oldState);
    }
}

void TimerEngine::handleOneSecondTick() {
    checkIdleDetection();

    if (m_state == State::Paused) {
        return;
    }

    bool concurrent = m_settings->concurrentPresetsEnabled();

    if (m_state == State::Working) {
        if (m_secondsRemaining > 0) {
            m_secondsRemaining--;
        }

        if (concurrent && m_secondarySecondsRemaining > 0) {
            m_secondarySecondsRemaining--;
        }

        emit tick(m_secondsRemaining, m_totalDurationSeconds);
        emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);

        // Check if secondary (Macro) break expires first or simultaneously
        if (concurrent && m_secondarySecondsRemaining <= 0) {
            if (m_settings->suppressOnFullscreen() && FullscreenDetector::isFullscreenAppActive()) {
                m_secondarySecondsRemaining = 120; // Defer macro break by 2 minutes
                emit breakDeferredForFullscreen();
                return;
            }

            m_activeBreakType = ActiveBreakType::Secondary;
            m_settings->incrementBreaksCompleted(m_settings->secondaryBreakDurationSeconds());
            emit workCompleted();

            m_secondaryTotalDurationSeconds = m_settings->secondaryBreakDurationSeconds();
            m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;
            setState(State::Breaking);
            emit tick(m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            return;
        }

        // Primary (Micro) break expires
        if (m_secondsRemaining <= 0) {
            if (m_settings->suppressOnFullscreen() && FullscreenDetector::isFullscreenAppActive()) {
                m_secondsRemaining = 120; // Defer micro break by 2 minutes
                emit breakDeferredForFullscreen();
                return;
            }

            m_activeBreakType = ActiveBreakType::Primary;
            m_settings->incrementBreaksCompleted(m_settings->breakDurationSeconds());
            emit workCompleted();

            m_totalDurationSeconds = m_settings->breakDurationSeconds();
            m_secondsRemaining = m_totalDurationSeconds;
            setState(State::Breaking);
            emit tick(m_secondsRemaining, m_totalDurationSeconds);
            emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            return;
        }
    } else if (m_state == State::Breaking) {
        if (m_activeBreakType == ActiveBreakType::Secondary) {
            if (m_secondarySecondsRemaining > 0) {
                m_secondarySecondsRemaining--;
                emit tick(m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
                emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            }

            if (m_secondarySecondsRemaining <= 0) {
                emit breakCompleted();
                m_secondaryTotalDurationSeconds = m_settings->secondaryWorkDurationSeconds();
                m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;

                // Macro break resets the micro timer as well
                m_totalDurationSeconds = m_settings->workDurationSeconds();
                m_secondsRemaining = m_totalDurationSeconds;

                m_activeBreakType = ActiveBreakType::None;
                setState(State::Working);
                emit tick(m_secondsRemaining, m_totalDurationSeconds);
                emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            }
        } else {
            if (m_secondsRemaining > 0) {
                m_secondsRemaining--;
                emit tick(m_secondsRemaining, m_totalDurationSeconds);
                emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            }

            if (m_secondsRemaining <= 0) {
                emit breakCompleted();
                m_totalDurationSeconds = m_settings->workDurationSeconds();
                m_secondsRemaining = m_totalDurationSeconds;

                m_activeBreakType = ActiveBreakType::None;
                setState(State::Working);
                emit tick(m_secondsRemaining, m_totalDurationSeconds);
                emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
            }
        }
    }
}

void TimerEngine::checkIdleDetection() {
#ifdef Q_OS_WIN
    if (!m_settings->idleDetectionEnabled()) {
        return;
    }

    qint64 idleMs = getSystemIdleTimeMs();
    qint64 thresholdMs = static_cast<qint64>(m_settings->idleThresholdSeconds()) * 1000;

    if (m_state == State::Working && idleMs >= thresholdMs) {
        m_wasPausedForIdle = true;
        m_previousState = m_state;
        setState(State::Paused);
        emit idlePauseTriggered();
    } else if (m_state == State::Paused && m_wasPausedForIdle && idleMs < 2000) {
        m_wasPausedForIdle = false;
        setState(m_previousState == State::Idle ? State::Working : m_previousState);
        emit idleResumeTriggered();
    }
#endif
}

void TimerEngine::handleSettingsChanged() {
    if (m_state == State::Idle) {
        m_totalDurationSeconds = m_settings->workDurationSeconds();
        m_secondsRemaining = m_totalDurationSeconds;
        m_secondaryTotalDurationSeconds = m_settings->secondaryWorkDurationSeconds();
        m_secondarySecondsRemaining = m_secondaryTotalDurationSeconds;
        emit tick(m_secondsRemaining, m_totalDurationSeconds);
        emit compoundTick(m_secondsRemaining, m_totalDurationSeconds, m_secondarySecondsRemaining, m_secondaryTotalDurationSeconds);
    }
}
