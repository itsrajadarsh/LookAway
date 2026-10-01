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

public slots:
    void start();
    void pause();
    void resume();
    void stop();
    void skipBreak();

signals:
    void stateChanged(TimerEngine::State newState, TimerEngine::State oldState);
    void tick(int secondsRemaining, int totalDurationSeconds);
    void compoundTick(int primaryRemaining, int primaryTotal, int secondaryRemaining, int secondaryTotal);
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
    ActiveBreakType m_activeBreakType;
    int m_secondsRemaining;
    int m_totalDurationSeconds;
    int m_secondarySecondsRemaining;
    int m_secondaryTotalDurationSeconds;
    bool m_wasPausedForIdle;
};

#endif // TIMERENGINE_H
