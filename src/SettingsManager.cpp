#include "SettingsManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDate>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent),
      m_settings("LookAway", "LookAwayApp") {
    resetStatsIfNewDay();
}

int SettingsManager::workDurationSeconds() const {
    return m_settings.value("timer/workDuration", 1200).toInt(); // 20 mins default
}

void SettingsManager::setWorkDurationSeconds(int seconds) {
    if (seconds <= 0) seconds = 1200;
    m_settings.setValue("timer/workDuration", seconds);
    emit settingsChanged();
}

int SettingsManager::breakDurationSeconds() const {
    return m_settings.value("timer/breakDuration", 20).toInt(); // 20 secs default
}

void SettingsManager::setBreakDurationSeconds(int seconds) {
    if (seconds <= 0) seconds = 20;
    m_settings.setValue("timer/breakDuration", seconds);
    emit settingsChanged();
}

bool SettingsManager::audioEnabled() const {
    return m_settings.value("audio/enabled", true).toBool();
}

void SettingsManager::setAudioEnabled(bool enabled) {
    m_settings.setValue("audio/enabled", enabled);
    emit settingsChanged();
}

bool SettingsManager::notificationsEnabled() const {
    return m_settings.value("notifications/enabled", true).toBool();
}

void SettingsManager::setNotificationsEnabled(bool enabled) {
    m_settings.setValue("notifications/enabled", enabled);
    emit settingsChanged();
}

int SettingsManager::volume() const {
    return m_settings.value("audio/volume", 80).toInt();
}

void SettingsManager::setVolume(int volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    m_settings.setValue("audio/volume", volumePercent);
    emit settingsChanged();
}

bool SettingsManager::closeToTray() const {
    return m_settings.value("ui/closeToTray", true).toBool(); // Default true
}

void SettingsManager::setCloseToTray(bool enabled) {
    m_settings.setValue("ui/closeToTray", enabled);
    emit settingsChanged();
}

bool SettingsManager::autostart() const {
    return m_settings.value("system/autostart", false).toBool();
}

void SettingsManager::setAutostart(bool enabled) {
    m_settings.setValue("system/autostart", enabled);
    applyAutostart(enabled);
    emit settingsChanged();
}

bool SettingsManager::breakWindowEnabled() const {
    return m_settings.value("ui/breakWindowEnabled", m_settings.value("ui/strictMode", true)).toBool();
}

void SettingsManager::setBreakWindowEnabled(bool enabled) {
    m_settings.setValue("ui/breakWindowEnabled", enabled);
    m_settings.setValue("ui/strictMode", enabled && (breakWindowStyle() == "fullscreen"));
    emit settingsChanged();
}

QString SettingsManager::breakWindowStyle() const {
    return m_settings.value("ui/breakWindowStyle", "popup").toString();
}

void SettingsManager::setBreakWindowStyle(const QString& style) {
    m_settings.setValue("ui/breakWindowStyle", style);
    m_settings.setValue("ui/strictMode", breakWindowEnabled() && (style == "fullscreen"));
    emit settingsChanged();
}

QRect SettingsManager::popupGeometry() const {
    return m_settings.value("ui/popupGeometry", QRect(-1, -1, 460, 320)).toRect();
}

void SettingsManager::setPopupGeometry(const QRect& geom) {
    m_settings.setValue("ui/popupGeometry", geom);
}

bool SettingsManager::forceDisableSkip() const {
    return m_settings.value("ui/forceDisableSkip", false).toBool();
}

void SettingsManager::setForceDisableSkip(bool disable) {
    m_settings.setValue("ui/forceDisableSkip", disable);
    emit settingsChanged();
}

bool SettingsManager::suppressOnFullscreen() const {
    return m_settings.value("system/suppressOnFullscreen", false).toBool();
}

void SettingsManager::setSuppressOnFullscreen(bool suppress) {
    m_settings.setValue("system/suppressOnFullscreen", suppress);
    emit settingsChanged();
}

bool SettingsManager::preBreakWarningEnabled() const {
    return m_settings.value("notification/preBreakWarningEnabled", true).toBool();
}

void SettingsManager::setPreBreakWarningEnabled(bool enabled) {
    m_settings.setValue("notification/preBreakWarningEnabled", enabled);
    emit settingsChanged();
}

int SettingsManager::preBreakWarningSeconds() const {
    return m_settings.value("notification/preBreakWarningSeconds", 30).toInt();
}

void SettingsManager::setPreBreakWarningSeconds(int seconds) {
    m_settings.setValue("notification/preBreakWarningSeconds", seconds);
    emit settingsChanged();
}

bool SettingsManager::postponeEnabled() const {
    return m_settings.value("ui/postponeEnabled", true).toBool();
}

void SettingsManager::setPostponeEnabled(bool enabled) {
    m_settings.setValue("ui/postponeEnabled", enabled);
    emit settingsChanged();
}

int SettingsManager::defaultPostponeSeconds() const {
    return m_settings.value("ui/defaultPostponeSeconds", 120).toInt();
}

void SettingsManager::setDefaultPostponeSeconds(int seconds) {
    m_settings.setValue("ui/defaultPostponeSeconds", seconds);
    emit settingsChanged();
}

bool SettingsManager::dndActive() const {
    return m_settings.value("system/dndActive", false).toBool();
}

void SettingsManager::setDndActive(bool active) {
    m_settings.setValue("system/dndActive", active);
    emit settingsChanged();
}

int SettingsManager::dndDurationSeconds() const {
    return m_settings.value("system/dndDurationSeconds", 1800).toInt();
}

void SettingsManager::setDndDurationSeconds(int seconds) {
    m_settings.setValue("system/dndDurationSeconds", seconds);
    emit settingsChanged();
}

QString SettingsManager::soundPack() const {
    return m_settings.value("audio/soundPack", "default").toString();
}

void SettingsManager::setSoundPack(const QString& pack) {
    m_settings.setValue("audio/soundPack", pack);
    emit settingsChanged();
}

QString SettingsManager::customWorkSoundPath() const {
    return m_settings.value("audio/customWorkSoundPath", "").toString();
}

void SettingsManager::setCustomWorkSoundPath(const QString& path) {
    m_settings.setValue("audio/customWorkSoundPath", path);
    emit settingsChanged();
}

QString SettingsManager::customBreakSoundPath() const {
    return m_settings.value("audio/customBreakSoundPath", "").toString();
}

void SettingsManager::setCustomBreakSoundPath(const QString& path) {
    m_settings.setValue("audio/customBreakSoundPath", path);
    emit settingsChanged();
}

bool SettingsManager::screenFlashEnabled() const {
    return m_settings.value("alerts/screenFlashEnabled", true).toBool();
}

void SettingsManager::setScreenFlashEnabled(bool enabled) {
    m_settings.setValue("alerts/screenFlashEnabled", enabled);
    emit settingsChanged();
}

QString SettingsManager::screenFlashStyle() const {
    return m_settings.value("alerts/screenFlashStyle", "cyan").toString();
}

void SettingsManager::setScreenFlashStyle(const QString& style) {
    m_settings.setValue("alerts/screenFlashStyle", style);
    emit settingsChanged();
}

bool SettingsManager::globalHotkeysEnabled() const {
    return m_settings.value("hotkeys/enabled", true).toBool();
}

void SettingsManager::setGlobalHotkeysEnabled(bool enabled) {
    m_settings.setValue("hotkeys/enabled", enabled);
    emit settingsChanged();
}

QString SettingsManager::hotkeyPauseResume() const {
    return m_settings.value("hotkeys/pauseResume", "Ctrl+Alt+P").toString();
}

void SettingsManager::setHotkeyPauseResume(const QString& seq) {
    m_settings.setValue("hotkeys/pauseResume", seq);
    emit settingsChanged();
}

QString SettingsManager::hotkeySnooze() const {
    return m_settings.value("hotkeys/snooze", "Ctrl+Alt+S").toString();
}

void SettingsManager::setHotkeySnooze(const QString& seq) {
    m_settings.setValue("hotkeys/snooze", seq);
    emit settingsChanged();
}

QString SettingsManager::hotkeySkip() const {
    return m_settings.value("hotkeys/skip", "Ctrl+Alt+K").toString();
}

void SettingsManager::setHotkeySkip(const QString& seq) {
    m_settings.setValue("hotkeys/skip", seq);
    emit settingsChanged();
}

QString SettingsManager::hotkeyDnd() const {
    return m_settings.value("hotkeys/dnd", "Ctrl+Alt+D").toString();
}

void SettingsManager::setHotkeyDnd(const QString& seq) {
    m_settings.setValue("hotkeys/dnd", seq);
    emit settingsChanged();
}

bool SettingsManager::nonStealingFocus() const {
    return m_settings.value("ui/nonStealingFocus", true).toBool();
}

void SettingsManager::setNonStealingFocus(bool nonStealing) {
    m_settings.setValue("ui/nonStealingFocus", nonStealing);
    emit settingsChanged();
}

bool SettingsManager::concurrentPresetsEnabled() const {
    return m_settings.value("timer/concurrentPresetsEnabled", false).toBool();
}

void SettingsManager::setConcurrentPresetsEnabled(bool enabled) {
    m_settings.setValue("timer/concurrentPresetsEnabled", enabled);
    emit settingsChanged();
}

QString SettingsManager::secondaryPresetName() const {
    return m_settings.value("timer/secondaryPresetName", "50-10").toString();
}

void SettingsManager::setSecondaryPresetName(const QString& name) {
    m_settings.setValue("timer/secondaryPresetName", name);
    emit settingsChanged();
}

int SettingsManager::secondaryWorkDurationSeconds() const {
    return m_settings.value("timer/secondaryWorkDuration", 3000).toInt(); // 50 mins default
}

void SettingsManager::setSecondaryWorkDurationSeconds(int seconds) {
    if (seconds <= 0) seconds = 3000;
    m_settings.setValue("timer/secondaryWorkDuration", seconds);
    emit settingsChanged();
}

int SettingsManager::secondaryBreakDurationSeconds() const {
    return m_settings.value("timer/secondaryBreakDuration", 600).toInt(); // 10 mins default
}

void SettingsManager::setSecondaryBreakDurationSeconds(int seconds) {
    if (seconds <= 0) seconds = 600;
    m_settings.setValue("timer/secondaryBreakDuration", seconds);
    emit settingsChanged();
}

bool SettingsManager::strictModeEnabled() const {
    return breakWindowEnabled() && (breakWindowStyle() == "fullscreen");
}

void SettingsManager::setStrictModeEnabled(bool enabled) {
    setBreakWindowEnabled(enabled);
    if (enabled && breakWindowStyle().isEmpty()) {
        setBreakWindowStyle("fullscreen");
    }
}

bool SettingsManager::idleDetectionEnabled() const {
    return m_settings.value("system/idleDetection", true).toBool();
}

void SettingsManager::setIdleDetectionEnabled(bool enabled) {
    m_settings.setValue("system/idleDetection", enabled);
    emit settingsChanged();
}

int SettingsManager::idleThresholdSeconds() const {
    return m_settings.value("system/idleThreshold", 180).toInt(); // 3 mins default
}

void SettingsManager::setIdleThresholdSeconds(int seconds) {
    if (seconds < 30) seconds = 30;
    m_settings.setValue("system/idleThreshold", seconds);
    emit settingsChanged();
}

QList<CustomPreset> SettingsManager::customPresets() const {
    QList<CustomPreset> list;
    int count = m_settings.value("customPresets/count", -1).toInt();
    if (count == -1) {
        // Default out-of-the-box custom presets
        list.append({"Sprint (15m / 2m)", 900, 120});
        list.append({"Eye Strain Relief (10m / 30s)", 600, 30});
        return list;
    }

    for (int i = 0; i < count; ++i) {
        QString prefix = QString("customPresets/%1/").arg(i);
        CustomPreset p;
        p.name = m_settings.value(prefix + "name").toString();
        p.workDurationSeconds = m_settings.value(prefix + "work", 1200).toInt();
        p.breakDurationSeconds = m_settings.value(prefix + "break", 20).toInt();
        if (!p.name.isEmpty()) {
            list.append(p);
        }
    }
    return list;
}

void SettingsManager::setCustomPresets(const QList<CustomPreset>& presets) {
    int oldCount = m_settings.value("customPresets/count", 0).toInt();
    for (int i = 0; i < oldCount; ++i) {
        m_settings.remove(QString("customPresets/%1").arg(i));
    }
    m_settings.setValue("customPresets/count", presets.size());
    for (int i = 0; i < presets.size(); ++i) {
        QString prefix = QString("customPresets/%1/").arg(i);
        m_settings.setValue(prefix + "name", presets[i].name);
        m_settings.setValue(prefix + "work", presets[i].workDurationSeconds);
        m_settings.setValue(prefix + "break", presets[i].breakDurationSeconds);
    }
    emit settingsChanged();
}

void SettingsManager::saveCustomPreset(const CustomPreset& preset) {
    QList<CustomPreset> list = customPresets();
    bool found = false;
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].name.compare(preset.name, Qt::CaseInsensitive) == 0) {
            list[i] = preset;
            found = true;
            break;
        }
    }
    if (!found) {
        list.append(preset);
    }
    setCustomPresets(list);
}

void SettingsManager::deleteCustomPreset(const QString& name) {
    QList<CustomPreset> list = customPresets();
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].name.compare(name, Qt::CaseInsensitive) == 0) {
            list.removeAt(i);
            break;
        }
    }
    setCustomPresets(list);
}

QString SettingsManager::activePresetName() const {
    return m_settings.value("timer/activePreset", "20-20-20").toString();
}

void SettingsManager::setActivePresetName(const QString& name) {
    m_settings.setValue("timer/activePreset", name);
    emit settingsChanged();
}

int SettingsManager::breaksCompletedToday() const {
    return m_settings.value("stats/completedToday", 0).toInt();
}

int SettingsManager::breaksSkippedToday() const {
    return m_settings.value("stats/skippedToday", 0).toInt();
}

int SettingsManager::eyeRestSecondsToday() const {
    return m_settings.value("stats/eyeRestSecondsToday", 0).toInt();
}

void SettingsManager::incrementBreaksCompleted(int breakDurationSecs) {
    resetStatsIfNewDay();
    int completed = breaksCompletedToday() + 1;
    int restSecs = eyeRestSecondsToday() + breakDurationSecs;
    m_settings.setValue("stats/completedToday", completed);
    m_settings.setValue("stats/eyeRestSecondsToday", restSecs);
    recordTodayStats();
    emit statsUpdated();
}

int SettingsManager::breaksPostponedToday() const {
    return m_settings.value("stats/postponedToday", 0).toInt();
}

void SettingsManager::incrementBreaksPostponed() {
    resetStatsIfNewDay();
    int postponed = breaksPostponedToday() + 1;
    m_settings.setValue("stats/postponedToday", postponed);
    recordTodayStats();
    emit statsUpdated();
}

void SettingsManager::incrementBreaksSkipped() {
    resetStatsIfNewDay();
    int skipped = breaksSkippedToday() + 1;
    m_settings.setValue("stats/skippedToday", skipped);
    recordTodayStats();
    emit statsUpdated();
}

void SettingsManager::recordTodayStats() {
    QString todayStr = QDate::currentDate().toString(Qt::ISODate);
    m_settings.setValue(QString("history/%1/completed").arg(todayStr), breaksCompletedToday());
    m_settings.setValue(QString("history/%1/snoozed").arg(todayStr), breaksPostponedToday());
    m_settings.setValue(QString("history/%1/skipped").arg(todayStr), breaksSkippedToday());
    m_settings.setValue(QString("history/%1/restSeconds").arg(todayStr), eyeRestSecondsToday());
}

QList<DayStats> SettingsManager::recentStats(int days) const {
    QList<DayStats> list;
    QDate today = QDate::currentDate();
    for (int i = days - 1; i >= 0; --i) {
        QDate d = today.addDays(-i);
        QString dateStr = d.toString(Qt::ISODate);
        DayStats ds;
        ds.date = dateStr;
        if (dateStr == today.toString(Qt::ISODate)) {
            ds.completed = breaksCompletedToday();
            ds.snoozed = breaksPostponedToday();
            ds.skipped = breaksSkippedToday();
            ds.restSeconds = eyeRestSecondsToday();
        } else {
            ds.completed = m_settings.value(QString("history/%1/completed").arg(dateStr), 0).toInt();
            ds.snoozed = m_settings.value(QString("history/%1/snoozed").arg(dateStr), 0).toInt();
            ds.skipped = m_settings.value(QString("history/%1/skipped").arg(dateStr), 0).toInt();
            ds.restSeconds = m_settings.value(QString("history/%1/restSeconds").arg(dateStr), 0).toInt();
        }
        list.append(ds);
    }
    return list;
}

int SettingsManager::currentStreakDays() const {
    int streak = 0;
    QDate today = QDate::currentDate();
    if (breaksCompletedToday() > 0) {
        streak++;
    }
    QDate checkDate = today.addDays(-1);
    while (true) {
        QString dateStr = checkDate.toString(Qt::ISODate);
        int completed = m_settings.value(QString("history/%1/completed").arg(dateStr), 0).toInt();
        if (completed > 0) {
            streak++;
            checkDate = checkDate.addDays(-1);
        } else {
            break;
        }
    }
    return streak;
}

double SettingsManager::weeklyComplianceRate() const {
    auto stats = recentStats(7);
    int totalCompleted = 0;
    int totalSkipped = 0;
    for (const auto& ds : stats) {
        totalCompleted += ds.completed;
        totalSkipped += ds.skipped;
    }
    if (totalCompleted + totalSkipped == 0) {
        return 100.0;
    }
    return (static_cast<double>(totalCompleted) / (totalCompleted + totalSkipped)) * 100.0;
}

void SettingsManager::sync() {
    m_settings.sync();
}

void SettingsManager::resetStatsIfNewDay() {
    QString todayStr = QDate::currentDate().toString(Qt::ISODate);
    QString lastReset = m_settings.value("stats/lastResetDate", "").toString();
    if (lastReset != todayStr) {
        if (!lastReset.isEmpty()) {
            m_settings.setValue(QString("history/%1/completed").arg(lastReset), breaksCompletedToday());
            m_settings.setValue(QString("history/%1/snoozed").arg(lastReset), breaksPostponedToday());
            m_settings.setValue(QString("history/%1/skipped").arg(lastReset), breaksSkippedToday());
            m_settings.setValue(QString("history/%1/restSeconds").arg(lastReset), eyeRestSecondsToday());
        }

        // Prune history older than 30 days
        QDate cutoff = QDate::currentDate().addDays(-30);
        m_settings.beginGroup("history");
        const QStringList childGroups = m_settings.childGroups();
        for (const QString& dStr : childGroups) {
            QDate d = QDate::fromString(dStr, Qt::ISODate);
            if (d.isValid() && d < cutoff) {
                m_settings.remove(dStr);
            }
        }
        m_settings.endGroup();

        m_settings.setValue("stats/lastResetDate", todayStr);
        m_settings.setValue("stats/completedToday", 0);
        m_settings.setValue("stats/skippedToday", 0);
        m_settings.setValue("stats/postponedToday", 0);
        m_settings.setValue("stats/eyeRestSecondsToday", 0);
        recordTodayStats();
        emit statsUpdated();
    }
}

void SettingsManager::resetDailyStats() {
    m_settings.setValue("stats/completedToday", 0);
    m_settings.setValue("stats/skippedToday", 0);
    m_settings.setValue("stats/postponedToday", 0);
    m_settings.setValue("stats/eyeRestSecondsToday", 0);
    emit statsUpdated();
}

void SettingsManager::resetAllToDefaults() {
    setWorkDurationSeconds(1200);
    setBreakDurationSeconds(20);
    setAudioEnabled(true);
    setVolume(80);
    setNotificationsEnabled(true);
    setPreBreakWarningEnabled(true);
    setPreBreakWarningSeconds(30);
    setPostponeEnabled(true);
    setDefaultPostponeSeconds(120);
    setBreakWindowEnabled(true);
    setBreakWindowStyle("popup");
    setPopupGeometry(QRect(-1, -1, 460, 320));
    setForceDisableSkip(false);
    setSuppressOnFullscreen(false);
    setNonStealingFocus(true);
    setConcurrentPresetsEnabled(false);
    setSecondaryPresetName("50-10");
    setSecondaryWorkDurationSeconds(3000);
    setSecondaryBreakDurationSeconds(600);
    setCloseToTray(true);
    setAutostart(false);
    setIdleDetectionEnabled(true);
    setIdleThresholdSeconds(180);
    setActivePresetName("20-20-20");
    setDndActive(false);
    setDndDurationSeconds(1800);
    setSoundPack("default");
    setCustomWorkSoundPath("");
    setCustomBreakSoundPath("");
    setScreenFlashEnabled(true);
    setScreenFlashStyle("cyan");
    setGlobalHotkeysEnabled(true);
    setHotkeyPauseResume("Ctrl+Alt+P");
    setHotkeySnooze("Ctrl+Alt+S");
    setHotkeySkip("Ctrl+Alt+K");
    setHotkeyDnd("Ctrl+Alt+D");
    resetDailyStats();
    emit settingsChanged();
}

void SettingsManager::applyAutostart(bool enabled) {
#ifdef Q_OS_WIN
    QSettings autoRunSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    if (enabled) {
        autoRunSettings.setValue("LookAway", "\"" + appPath + "\" --minimized");
    } else {
        autoRunSettings.remove("LookAway");
    }
#elif defined(Q_OS_LINUX)
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString autostartDir = configDir + "/autostart";
    QString desktopFilePath = autostartDir + "/lookaway.desktop";

    if (enabled) {
        QDir dir;
        if (!dir.exists(autostartDir)) {
            dir.mkpath(autostartDir);
        }

        QString appPath = qEnvironmentVariableIsSet("APPIMAGE")
                              ? qEnvironmentVariable("APPIMAGE")
                              : QCoreApplication::applicationFilePath();

        QFile file(desktopFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Version=1.0\n";
            out << "Name=LookAway\n";
            out << "GenericName=Eye Care Utility\n";
            out << "Comment=20-20-20 Eye Care Background Utility\n";
            out << "Exec=\"" << appPath << "\" --minimized\n";
            out << "Icon=lookaway\n";
            out << "Terminal=false\n";
            out << "Categories=Utility;Health;\n";
            out << "StartupNotify=false\n";
            out << "X-GNOME-Autostart-enabled=true\n";
            file.close();
        }
    } else {
        QFile::remove(desktopFilePath);
    }
#endif
}
