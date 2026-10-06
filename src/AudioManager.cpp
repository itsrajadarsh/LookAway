#include "AudioManager.h"
#include <QUrl>
#include <QFileInfo>
#include <QDebug>

AudioManager::AudioManager(SettingsManager* settings, TimerEngine* timerEngine, QObject* parent)
    : QObject(parent),
      m_settings(settings),
      m_timerEngine(timerEngine) {

    syncSettings();

    connect(m_settings, &SettingsManager::settingsChanged, this, &AudioManager::syncSettings);
    connect(m_timerEngine, &TimerEngine::workCompleted, this, &AudioManager::playWorkCompleteChime);
    connect(m_timerEngine, &TimerEngine::breakCompleted, this, &AudioManager::playBreakCompleteChime);
}

QUrl AudioManager::resolveSoundUrl(const QString& soundType) const {
    QString pack = m_settings->soundPack();
    bool isWork = (soundType == "work");

    if (pack == "zen") {
        return QUrl(isWork ? "qrc:/sounds/zen_work.wav" : "qrc:/sounds/zen_break.wav");
    } else if (pack == "marimba") {
        return QUrl(isWork ? "qrc:/sounds/marimba_work.wav" : "qrc:/sounds/marimba_break.wav");
    } else if (pack == "bell") {
        return QUrl(isWork ? "qrc:/sounds/bell_work.wav" : "qrc:/sounds/bell_break.wav");
    } else if (pack == "custom") {
        QString customPath = isWork ? m_settings->customWorkSoundPath() : m_settings->customBreakSoundPath();
        if (!customPath.isEmpty() && QFileInfo::exists(customPath)) {
            return QUrl::fromLocalFile(customPath);
        }
        // Fallback to default if custom file missing
        return QUrl(isWork ? "qrc:/sounds/chime_work.wav" : "qrc:/sounds/chime_break.wav");
    }

    // Default "default"
    return QUrl(isWork ? "qrc:/sounds/chime_work.wav" : "qrc:/sounds/chime_break.wav");
}

void AudioManager::syncSettings() {
    float vol = static_cast<float>(m_settings->volume()) / 100.0f;
    m_workSound.setSource(resolveSoundUrl("work"));
    m_breakSound.setSource(resolveSoundUrl("break"));
    m_workSound.setVolume(vol);
    m_breakSound.setVolume(vol);
}

void AudioManager::playWorkCompleteChime() {
    if (m_settings->audioEnabled() && !m_timerEngine->isDndActive()) {
        m_workSound.play();
    }
}

void AudioManager::playBreakCompleteChime() {
    if (m_settings->audioEnabled() && !m_timerEngine->isDndActive()) {
        m_breakSound.play();
    }
}

void AudioManager::playTestChime(const QString& type) {
    float vol = static_cast<float>(m_settings->volume()) / 100.0f;
    if (type == "break") {
        m_breakSound.setVolume(vol);
        m_breakSound.play();
    } else {
        m_workSound.setVolume(vol);
        m_workSound.play();
    }
}
