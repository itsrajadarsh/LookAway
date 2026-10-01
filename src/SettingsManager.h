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

    // Statistics & Reset
    int breaksCompletedToday() const;
    int breaksSkippedToday() const;
    int eyeRestSecondsToday() const;
    void incrementBreaksCompleted(int breakDurationSecs);
    void incrementBreaksSkipped();
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
