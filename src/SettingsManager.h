#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QDate>
#include <QList>
#include <QRect>

struct CustomPreset {
    QString name;
    int workDurationSeconds;
    int breakDurationSeconds;
};

struct DayStats {
    QString date;
    int completed = 0;
    int snoozed = 0;
    int skipped = 0;
    int restSeconds = 0;
};

class SettingsManager : public QObject {
    Q_OBJECT

public:
    explicit SettingsManager(QObject* parent = nullptr);

    int workDurationSeconds() const;
    void setWorkDurationSeconds(int seconds);

    int breakDurationSeconds() const;
    void setBreakDurationSeconds(int seconds);

    bool audioEnabled() const;
    void setAudioEnabled(bool enabled);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    int volume() const;
    void setVolume(int volumePercent);

    bool closeToTray() const;
    void setCloseToTray(bool enabled);

    bool autostart() const;
    void setAutostart(bool enabled);

    bool strictModeEnabled() const;
    void setStrictModeEnabled(bool enabled);

    bool breakWindowEnabled() const;
    void setBreakWindowEnabled(bool enabled);

    QString breakWindowStyle() const;
    void setBreakWindowStyle(const QString& style);

    QRect popupGeometry() const;
    void setPopupGeometry(const QRect& geom);

    bool forceDisableSkip() const;
    void setForceDisableSkip(bool disable);

    bool suppressOnFullscreen() const;
    void setSuppressOnFullscreen(bool suppress);

    // Pre-Break Warning
    bool preBreakWarningEnabled() const;
    void setPreBreakWarningEnabled(bool enabled);
    int preBreakWarningSeconds() const;
    void setPreBreakWarningSeconds(int seconds);

    // Postpone / Snooze Break
    bool postponeEnabled() const;
    void setPostponeEnabled(bool enabled);
    int defaultPostponeSeconds() const;
    void setDefaultPostponeSeconds(int seconds);
    int breaksPostponedToday() const;
    void incrementBreaksPostponed();

    // Meeting / Presentation Do Not Disturb (DND)
    bool dndActive() const;
    void setDndActive(bool active);
    int dndDurationSeconds() const;
    void setDndDurationSeconds(int seconds);

    // Audio Sound Pack & Custom Chimes
    QString soundPack() const;
    void setSoundPack(const QString& pack);
    QString customWorkSoundPath() const;
    void setCustomWorkSoundPath(const QString& path);
    QString customBreakSoundPath() const;
    void setCustomBreakSoundPath(const QString& path);

    // End-of-Break Screen Flash
    bool screenFlashEnabled() const;
    void setScreenFlashEnabled(bool enabled);
    QString screenFlashStyle() const;
    void setScreenFlashStyle(const QString& style);

    // Global Hotkeys
    bool globalHotkeysEnabled() const;
    void setGlobalHotkeysEnabled(bool enabled);
    QString hotkeyPauseResume() const;
    void setHotkeyPauseResume(const QString& seq);
    QString hotkeySnooze() const;
    void setHotkeySnooze(const QString& seq);
    QString hotkeySkip() const;
    void setHotkeySkip(const QString& seq);
    QString hotkeyDnd() const;
    void setHotkeyDnd(const QString& seq);

    bool nonStealingFocus() const;
    void setNonStealingFocus(bool nonStealing);

    bool idleDetectionEnabled() const;
    void setIdleDetectionEnabled(bool enabled);

    int idleThresholdSeconds() const;
    void setIdleThresholdSeconds(int seconds);

    // Multi/Concurrent Presets
    bool concurrentPresetsEnabled() const;
    void setConcurrentPresetsEnabled(bool enabled);

    QString secondaryPresetName() const;
    void setSecondaryPresetName(const QString& name);

    int secondaryWorkDurationSeconds() const;
    void setSecondaryWorkDurationSeconds(int seconds);

    int secondaryBreakDurationSeconds() const;
    void setSecondaryBreakDurationSeconds(int seconds);

    // Custom Presets
    QList<CustomPreset> customPresets() const;
    void setCustomPresets(const QList<CustomPreset>& presets);
    void saveCustomPreset(const CustomPreset& preset);
    void deleteCustomPreset(const QString& name);

    QString activePresetName() const;
    void setActivePresetName(const QString& name);

    // Statistics, Analytics & Habit Streaks
    int breaksCompletedToday() const;
    int breaksSkippedToday() const;
    int eyeRestSecondsToday() const;
    void incrementBreaksCompleted(int breakDurationSecs);
    void incrementBreaksSkipped();
    void recordTodayStats();
    QList<DayStats> recentStats(int days = 7) const;
    int currentStreakDays() const;
    double weeklyComplianceRate() const;
    void sync();
    void resetStatsIfNewDay();
    void resetDailyStats();
    void resetAllToDefaults();

signals:
    void settingsChanged();
    void statsUpdated();

private:
    QSettings m_settings;
    void applyAutostart(bool enabled);
};

#endif // SETTINGSMANAGER_H
