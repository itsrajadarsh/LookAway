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
#include <QLineEdit>
#include "TimerEngine.h"
#include "SettingsManager.h"
#include "AudioManager.h"
#include "BreakOverlayWidget.h"
#include "CustomPresetDialog.h"
#include "WeeklyAnalyticsWidget.h"
#include "ScreenFlashWidget.h"
#include "GlobalHotkeyManager.h"
#include "LinuxIdleDetector.h"

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
    void handlePreBreakWarning(int secondsUntilBreak, TimerEngine::ActiveBreakType breakType);
    void handleDndStateChanged(bool active, int secondsRemaining);

private:
    void setupUi();
    QWidget* createDashboardTab();
    QWidget* createAnalyticsTab();
    QWidget* createAlertsTab();
    QWidget* createAudioTab();
    QWidget* createPreferencesTab();
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
    QPushButton* m_btnDnd;
    QMenu* m_dndMenu;
    QPushButton* m_btnEndDnd;
    QLabel* m_countdownLabel;
    QProgressBar* m_progressBar;
    QLabel* m_lblSecondaryTimerStatus;
    QPushButton* m_btnPlayPause;
    QPushButton* m_btnReset;
    QPushButton* m_btnPostponeBreak;
    QPushButton* m_btnSkipBreak;

    // Presets UI
    QPushButton* m_btnPreset20;
    QPushButton* m_btnPreset25;
    QPushButton* m_btnPreset50;
    QPushButton* m_btnPresetCustom;
    QPushButton* m_btnToggleConcurrentMode;
    QMenu* m_customPresetMenu;

    // Stats UI
    QLabel* m_lblStatCompleted;
    QLabel* m_lblStatPostponed;
    QLabel* m_lblStatSkipped;
    QLabel* m_lblStatRestTime;
    WeeklyAnalyticsWidget* m_weeklyAnalyticsWidget;

    // Settings UI
    QComboBox* m_comboWorkVal;
    QComboBox* m_comboWorkUnit;
    QComboBox* m_comboBreakVal;
    QComboBox* m_comboBreakUnit;
    QPushButton* m_btnSaveCurrentAsProfile;
    QCheckBox* m_chkConcurrentPresets;
    QComboBox* m_comboSecondaryPreset;
    QCheckBox* m_chkBreakWindow;
    QComboBox* m_comboBreakStyle;
    QCheckBox* m_chkPreBreakWarning;
    QComboBox* m_comboPreBreakWarningSecs;
    QCheckBox* m_chkPostponeBreak;
    QComboBox* m_comboPostponeMins;
    QCheckBox* m_chkForceDisableSkip;
    QCheckBox* m_chkSuppressOnFullscreen;
    QCheckBox* m_chkAudioEnabled;
    QSlider* m_sliderVolume;
    QLabel* m_lblVolumeVal;
    QComboBox* m_comboSoundPack;
    QWidget* m_customSoundWidget;
    QLineEdit* m_editCustomWorkPath;
    QLineEdit* m_editCustomBreakPath;
    QPushButton* m_btnBrowseCustomWork;
    QPushButton* m_btnBrowseCustomBreak;
    QPushButton* m_btnTestWorkSound;
    QPushButton* m_btnTestBreakSound;
    QCheckBox* m_chkNotificationsEnabled;
    QCheckBox* m_chkCloseToTray;
    QCheckBox* m_chkAutostart;
    QCheckBox* m_chkIdleDetection;
    QComboBox* m_comboIdleVal;
    QComboBox* m_comboIdleUnit;
    QLabel* m_lblIdleBackendStatus;

    // Screen Flash UI
    QCheckBox* m_chkScreenFlash;
    QComboBox* m_comboScreenFlashStyle;
    QPushButton* m_btnPreviewFlash;

    // Hotkeys UI & Manager
    GlobalHotkeyManager* m_hotkeyManager;
    QCheckBox* m_chkGlobalHotkeys;
    QLineEdit* m_editHotkeyPause;
    QLineEdit* m_editHotkeySnooze;
    QLineEdit* m_editHotkeySkip;
    QLineEdit* m_editHotkeyDnd;
    QPushButton* m_btnResetHotkeys;
    QLabel* m_lblHotkeyStatus;
};

#endif // MAINWINDOW_H
