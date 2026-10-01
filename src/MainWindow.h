#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QSpinBox>
#include <QCheckBox>
#include <QSlider>
#include <QComboBox>
#include <QTabWidget>
#include <QCloseEvent>
#include <QMenu>
#include "TimerEngine.h"
#include "SettingsManager.h"
#include "AudioManager.h"
#include "BreakOverlayWidget.h"
#include "CustomPresetDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(TimerEngine* timerEngine, SettingsManager* settings, AudioManager* audioManager, QWidget* parent = nullptr);
    ~MainWindow();

    void showSettingsTab();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void updateUiForState(TimerEngine::State newState, TimerEngine::State oldState);
    void updateCountdown(int secondsRemaining, int totalSeconds);
    void updateStatsDisplay();
    void saveSettingsFromUi();
    void loadSettingsToUi();
    void applyPreset(int workSecs, int breakSecs, const QString& presetName);
    void updateActivePresetHighlight();
    void rebuildCustomPresetMenu();
    void openCreateCustomPresetDialog();
    void openEditCustomPresetDialog();
    void deleteSelectedCustomPreset();

private:
    void setupUi();
    QWidget* createDashboardTab();
    QWidget* createSettingsTab();
    void applyTheme();
    int durationToSeconds(QComboBox* valCombo, QComboBox* unitCombo) const;
    void secondsToUi(int totalSeconds, QComboBox* valCombo, QComboBox* unitCombo);

    TimerEngine* m_timerEngine;
    SettingsManager* m_settings;
    AudioManager* m_audioManager;
    QList<BreakOverlayWidget*> m_breakOverlays;

    QTabWidget* m_tabWidget;
    bool m_isUpdatingUi;

    // Dashboard UI
    QLabel* m_statusBadgeLabel;
    QLabel* m_countdownLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_btnPlayPause;
    QPushButton* m_btnReset;
    QPushButton* m_btnSkipBreak;

    // Presets UI
    QPushButton* m_btnPreset20;
    QPushButton* m_btnPreset25;
    QPushButton* m_btnPreset50;
    QPushButton* m_btnPresetCustom;
    QMenu* m_customPresetMenu;

    // Stats UI
    QLabel* m_lblStatCompleted;
    QLabel* m_lblStatSkipped;
    QLabel* m_lblStatRestTime;

    // Settings UI
    QComboBox* m_comboWorkVal;
    QComboBox* m_comboWorkUnit;
    QComboBox* m_comboBreakVal;
    QComboBox* m_comboBreakUnit;
    QPushButton* m_btnSaveCurrentAsProfile;
    QCheckBox* m_chkBreakWindow;
    QComboBox* m_comboBreakStyle;
    QCheckBox* m_chkAudioEnabled;
    QSlider* m_sliderVolume;
    QLabel* m_lblVolumeVal;
    QPushButton* m_btnTestAudio;
    QCheckBox* m_chkNotificationsEnabled;
    QCheckBox* m_chkCloseToTray;
    QCheckBox* m_chkAutostart;
    QCheckBox* m_chkIdleDetection;
    QComboBox* m_comboIdleVal;
    QComboBox* m_comboIdleUnit;
};

#endif // MAINWINDOW_H
