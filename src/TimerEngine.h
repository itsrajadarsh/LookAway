#ifndef TIMERENGINE_H
#define TIMERENGINE_H

#include <QObject>
#include <QTimer>
#include "SettingsManager.h"

class TimerEngine : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Working,
        Breaking,
        Paused
    };
    Q_ENUM(State)

    enum class ActiveBreakType {
        None,
        Primary,
        Secondary
    };
    Q_ENUM(ActiveBreakType)

    explicit TimerEngine(SettingsManager* settings, QObject* parent = nullptr);

    State state() const;
    ActiveBreakType activeBreakType() const;
    int secondsRemaining() const;
    int totalDurationSeconds() const;
    QString formattedTimeRemaining() const;

    int secondarySecondsRemaining() const;
    int secondaryTotalDurationSeconds() const;
    QString formattedSecondaryTimeRemaining() const;

    bool isPausedForIdle() const;
    bool isDndActive() const;
    int dndSecondsRemaining() const;
    QString formattedDndTimeRemaining() const;

public slots:
    void start();
    void pause();
    void resume();
    void stop();
    void skipBreak();
    void postponeBreak(int postponeSeconds = 120);
    void enableDnd(int durationSeconds);
    void disableDnd();

signals:
    void stateChanged(TimerEngine::State newState, TimerEngine::State oldState);
    void tick(int secondsRemaining, int totalDurationSeconds);
    void compoundTick(int primaryRemaining, int primaryTotal, int secondaryRemaining, int secondaryTotal);
    void preBreakWarning(int secondsUntilBreak, TimerEngine::ActiveBreakType breakType);
    void breakPostponed(int postponeSeconds);
    void dndStateChanged(bool active, int secondsRemaining);
    void dndExpired();
    void workCompleted();
    void breakCompleted();
    void idlePauseTriggered();
    void idleResumeTriggered();
    void breakDeferredForFullscreen();

private slots:
    void handleOneSecondTick();
    void handleSettingsChanged();

private:
    void setState(State newState);
    void checkIdleDetection();

    SettingsManager* m_settings;
    QTimer m_timer;
    State m_state;
    State m_previousState;
    State m_stateBeforeDnd;
    ActiveBreakType m_activeBreakType;
    int m_secondsRemaining;
    int m_totalDurationSeconds;
    int m_secondarySecondsRemaining;
    int m_secondaryTotalDurationSeconds;
    bool m_wasPausedForIdle;
    bool m_preBreakWarningFired;
    bool m_secondaryPreBreakWarningFired;
    bool m_dndActive;
    int m_dndSecondsRemaining;
};

#endif // TIMERENGINE_H
