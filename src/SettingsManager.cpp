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
    emit statsUpdated();
}

void SettingsManager::incrementBreaksSkipped() {
    resetStatsIfNewDay();
    int skipped = breaksSkippedToday() + 1;
    m_settings.setValue("stats/skippedToday", skipped);
    emit statsUpdated();
}

void SettingsManager::resetStatsIfNewDay() {
    QString todayStr = QDate::currentDate().toString(Qt::ISODate);
    QString lastReset = m_settings.value("stats/lastResetDate", "").toString();
    if (lastReset != todayStr) {
        m_settings.setValue("stats/lastResetDate", todayStr);
        m_settings.setValue("stats/completedToday", 0);
        m_settings.setValue("stats/skippedToday", 0);
        m_settings.setValue("stats/eyeRestSecondsToday", 0);
        emit statsUpdated();
    }
}

void SettingsManager::resetDailyStats() {
    m_settings.setValue("stats/completedToday", 0);
    m_settings.setValue("stats/skippedToday", 0);
    m_settings.setValue("stats/eyeRestSecondsToday", 0);
    emit statsUpdated();
}

void SettingsManager::resetAllToDefaults() {
    setWorkDurationSeconds(1200);
    setBreakDurationSeconds(20);
    setAudioEnabled(true);
    setVolume(80);
    setNotificationsEnabled(true);
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
