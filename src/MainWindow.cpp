#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QIcon>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <QIntValidator>
#include <QScrollArea>
#include <QListView>

MainWindow::MainWindow(TimerEngine* timerEngine, SettingsManager* settings, AudioManager* audioManager, QWidget* parent)
    : QMainWindow(parent),
      m_timerEngine(timerEngine),
      m_settings(settings),
      m_audioManager(audioManager),
      m_isUpdatingUi(false) {

    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setWindowTitle("LookAway - 20-20-20 Eye Care");
    setFixedSize(510, 650);

    setupUi();
    applyTheme();

    connect(m_timerEngine, &TimerEngine::stateChanged, this, &MainWindow::updateUiForState);
    connect(m_timerEngine, &TimerEngine::tick, this, &MainWindow::updateCountdown);
    connect(m_settings, &SettingsManager::statsUpdated, this, &MainWindow::updateStatsDisplay);

    loadSettingsToUi();
    updateStatsDisplay();
    updateUiForState(m_timerEngine->state(), TimerEngine::State::Idle);
    updateCountdown(m_timerEngine->secondsRemaining(), m_timerEngine->totalDurationSeconds());
    updateActivePresetHighlight();
}

MainWindow::~MainWindow() {
    qDeleteAll(m_breakOverlays);
    m_breakOverlays.clear();
}

int MainWindow::durationToSeconds(QComboBox* valCombo, QComboBox* unitCombo) const {
    int val = valCombo->currentText().toInt();
    if (val <= 0) val = 1;
    QString unit = unitCombo->currentText();
    if (unit == "Hours") {
        return val * 3600;
    } else if (unit == "Minutes") {
        return val * 60;
    }
    return val; // Seconds
}

void MainWindow::secondsToUi(int totalSeconds, QComboBox* valCombo, QComboBox* unitCombo) {
    if (totalSeconds <= 0) {
        valCombo->setCurrentText("1");
        unitCombo->setCurrentText("Seconds");
        return;
    }

    if (totalSeconds % 3600 == 0) {
        valCombo->setCurrentText(QString::number(totalSeconds / 3600));
        unitCombo->setCurrentText("Hours");
    } else if (totalSeconds % 60 == 0) {
        valCombo->setCurrentText(QString::number(totalSeconds / 60));
        unitCombo->setCurrentText("Minutes");
    } else {
        valCombo->setCurrentText(QString::number(totalSeconds));
        unitCombo->setCurrentText("Seconds");
    }
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(14, 14, 14, 14);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createDashboardTab(), QIcon(":/icons/app_icon.svg"), "Dashboard");
    m_tabWidget->addTab(createSettingsTab(), "Settings");

    mainLayout->addWidget(m_tabWidget);
}

QWidget* MainWindow::createDashboardTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(14);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon(":/icons/app_icon.svg").pixmap(36, 36));
    QLabel* titleLabel = new QLabel("LookAway");
    titleLabel->setObjectName("headerTitle");
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    // Status Badge
    m_statusBadgeLabel = new QLabel("READY TO WORK");
    m_statusBadgeLabel->setObjectName("statusBadge");
    m_statusBadgeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusBadgeLabel);

    // Countdown Display Box
    QFrame* timerCard = new QFrame();
    timerCard->setObjectName("timerCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(timerCard);
    cardLayout->setContentsMargins(16, 18, 16, 18);

    m_countdownLabel = new QLabel("20:00");
    m_countdownLabel->setObjectName("countdownDisplay");
    m_countdownLabel->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_countdownLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setObjectName("sessionProgress");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);
    m_progressBar->setTextVisible(false);
    cardLayout->addWidget(m_progressBar);

    layout->addWidget(timerCard);

    // Controls
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);

    m_btnPlayPause = new QPushButton("Start");
    m_btnPlayPause->setObjectName("btnPrimary");
    m_btnPlayPause->setFixedHeight(40);

    m_btnReset = new QPushButton("Reset");
    m_btnReset->setObjectName("btnSecondary");
    m_btnReset->setFixedHeight(40);

    m_btnSkipBreak = new QPushButton("Skip Break");
    m_btnSkipBreak->setObjectName("btnSecondary");
    m_btnSkipBreak->setFixedHeight(40);
    m_btnSkipBreak->setEnabled(false);

    btnLayout->addWidget(m_btnPlayPause);
    btnLayout->addWidget(m_btnReset);
    btnLayout->addWidget(m_btnSkipBreak);
    layout->addLayout(btnLayout);

    // Presets Row
    QHBoxLayout* presetLayout = new QHBoxLayout();
    presetLayout->setSpacing(8);
    QLabel* lblPreset = new QLabel("Presets:");
    lblPreset->setStyleSheet("font-weight: 700; color: #94a3b8; font-size: 12px;");
    presetLayout->addWidget(lblPreset);

    m_btnPreset20 = new QPushButton("20-20-20");
    m_btnPreset20->setObjectName("btnPreset");
    m_btnPreset20->setCheckable(true);
    m_btnPreset20->setToolTip(
        "<b>20-20-20 Rule (Eye Care)</b><br>"
        "• <b>Work Interval:</b> 20 minutes<br>"
        "• <b>Break Duration:</b> 20 seconds<br>"
        "• <b>Purpose:</b> Clinically recommended to prevent digital eye strain. "
        "Every 20 minutes, look at an object 20 feet away to relax your eye muscles."
    );

    m_btnPreset25 = new QPushButton("25-5 Pomo");
    m_btnPreset25->setObjectName("btnPreset");
    m_btnPreset25->setCheckable(true);
    m_btnPreset25->setToolTip(
        "<b>Pomodoro Technique (Focus)</b><br>"
        "• <b>Work Interval:</b> 25 minutes<br>"
        "• <b>Break Duration:</b> 5 minutes<br>"
        "• <b>Purpose:</b> Boosts productivity through structured intervals, "
        "allowing high mental concentration with regular rest breaks."
    );

    m_btnPreset50 = new QPushButton("50-10 Work");
    m_btnPreset50->setObjectName("btnPreset");
    m_btnPreset50->setCheckable(true);
    m_btnPreset50->setToolTip(
        "<b>Deep Work (Extended Session)</b><br>"
        "• <b>Work Interval:</b> 50 minutes<br>"
        "• <b>Break Duration:</b> 10 minutes<br>"
        "• <b>Purpose:</b> Designed for intensive coding, writing, and deep problem-solving "
        "where long uninterrupted focus is needed."
    );

    m_btnPresetCustom = new QPushButton("Custom ▾");
    m_btnPresetCustom->setObjectName("btnPreset");
    m_btnPresetCustom->setCheckable(true);
    m_btnPresetCustom->setToolTip(
        "<b>Custom Presets & Profiles</b><br>"
        "• Select from your saved custom work/break profiles.<br>"
        "• Click to switch profile or create a new custom preset."
    );

    m_customPresetMenu = new QMenu(this);
    m_btnPresetCustom->setMenu(m_customPresetMenu);
    rebuildCustomPresetMenu();

    connect(m_btnPreset20, &QPushButton::clicked, [this]() {
        applyPreset(20 * 60, 20, "20-20-20");
    });
    connect(m_btnPreset25, &QPushButton::clicked, [this]() {
        applyPreset(25 * 60, 300, "25-5 Pomo");
    });
    connect(m_btnPreset50, &QPushButton::clicked, [this]() {
        applyPreset(50 * 60, 600, "50-10 Work");
    });

    m_btnPreset20->setFixedHeight(34);
    m_btnPreset25->setFixedHeight(34);
    m_btnPreset50->setFixedHeight(34);
    m_btnPresetCustom->setFixedHeight(34);

    presetLayout->addWidget(m_btnPreset20, 1);
    presetLayout->addWidget(m_btnPreset25, 1);
    presetLayout->addWidget(m_btnPreset50, 1);
    presetLayout->addWidget(m_btnPresetCustom, 1);
    layout->addLayout(presetLayout);

    // Daily Stats Box with Reset button
    QGroupBox* statsGroup = new QGroupBox();
    QVBoxLayout* statsOuterLayout = new QVBoxLayout(statsGroup);
    statsOuterLayout->setContentsMargins(14, 12, 14, 12);
    statsOuterLayout->setSpacing(10);

    QHBoxLayout* statsHeaderLayout = new QHBoxLayout();
    QLabel* lblStatsTitle = new QLabel("Daily Eye Care Summary");
    lblStatsTitle->setStyleSheet("font-weight: 700; color: #38bdf8; font-size: 13px;");

    QPushButton* btnResetStats = new QPushButton("↺ Reset Summary");
    btnResetStats->setObjectName("btnSmall");
    btnResetStats->setFixedHeight(26);
    btnResetStats->setToolTip("Reset today's breaks taken, skipped, and rest time back to 0.");
    connect(btnResetStats, &QPushButton::clicked, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Reset Today's Summary",
            "Reset today's eye care summary (breaks taken, skipped, rest time) to zero?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            m_settings->resetDailyStats();
        }
    });

    statsHeaderLayout->addWidget(lblStatsTitle);
    statsHeaderLayout->addStretch();
    statsHeaderLayout->addWidget(btnResetStats);
    statsOuterLayout->addLayout(statsHeaderLayout);

    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(12);

    QVBoxLayout* col1 = new QVBoxLayout();
    m_lblStatCompleted = new QLabel("0");
    m_lblStatCompleted->setStyleSheet("font-size: 20px; font-weight: 700; color: #38bdf8;");
    m_lblStatCompleted->setAlignment(Qt::AlignCenter);
    QLabel* cap1 = new QLabel("Breaks Taken");
    cap1->setStyleSheet("font-size: 11px; color: #94a3b8;");
    cap1->setAlignment(Qt::AlignCenter);
    col1->addWidget(m_lblStatCompleted);
    col1->addWidget(cap1);

    QVBoxLayout* col2 = new QVBoxLayout();
    m_lblStatSkipped = new QLabel("0");
    m_lblStatSkipped->setStyleSheet("font-size: 20px; font-weight: 700; color: #f59e0b;");
    m_lblStatSkipped->setAlignment(Qt::AlignCenter);
    QLabel* cap2 = new QLabel("Skipped");
    cap2->setStyleSheet("font-size: 11px; color: #94a3b8;");
    cap2->setAlignment(Qt::AlignCenter);
    col2->addWidget(m_lblStatSkipped);
    col2->addWidget(cap2);

    QVBoxLayout* col3 = new QVBoxLayout();
    m_lblStatRestTime = new QLabel("0m");
    m_lblStatRestTime->setStyleSheet("font-size: 20px; font-weight: 700; color: #10b981;");
    m_lblStatRestTime->setAlignment(Qt::AlignCenter);
    QLabel* cap3 = new QLabel("Rest Time");
    cap3->setStyleSheet("font-size: 11px; color: #94a3b8;");
    cap3->setAlignment(Qt::AlignCenter);
    col3->addWidget(m_lblStatRestTime);
    col3->addWidget(cap3);

    statsLayout->addLayout(col1);
    statsLayout->addLayout(col2);
    statsLayout->addLayout(col3);
    statsOuterLayout->addLayout(statsLayout);

    layout->addWidget(statsGroup);
    layout->addStretch();

    // Connect Action Buttons
    connect(m_btnPlayPause, &QPushButton::clicked, [this]() {
        if (m_timerEngine->state() == TimerEngine::State::Working || m_timerEngine->state() == TimerEngine::State::Breaking) {
            m_timerEngine->pause();
        } else {
            m_timerEngine->start();
        }
    });

    connect(m_btnReset, &QPushButton::clicked, [this]() {
        m_timerEngine->stop();
    });

    connect(m_btnSkipBreak, &QPushButton::clicked, [this]() {
        m_timerEngine->skipBreak();
    });

    return tab;
}

void MainWindow::rebuildCustomPresetMenu() {
    m_customPresetMenu->clear();

    QList<CustomPreset> presets = m_settings->customPresets();
    QString activePreset = m_settings->activePresetName();

    QAction* headerAct = m_customPresetMenu->addAction("Saved Profiles:");
    headerAct->setEnabled(false);

    for (const CustomPreset& p : presets) {
        QString workStr = (p.workDurationSeconds >= 60) ? QString("%1m").arg(p.workDurationSeconds / 60) : QString("%1s").arg(p.workDurationSeconds);
        QString breakStr = (p.breakDurationSeconds >= 60) ? QString("%1m").arg(p.breakDurationSeconds / 60) : QString("%1s").arg(p.breakDurationSeconds);
        QString title = QString("%1 (%2 / %3)").arg(p.name, workStr, breakStr);

        QAction* act = m_customPresetMenu->addAction(title, [this, p]() {
            applyPreset(p.workDurationSeconds, p.breakDurationSeconds, p.name);
        });
        act->setCheckable(true);
        if (activePreset == p.name ||
            (m_settings->workDurationSeconds() == p.workDurationSeconds &&
             m_settings->breakDurationSeconds() == p.breakDurationSeconds)) {
            act->setChecked(true);
        }
    }

    m_customPresetMenu->addSeparator();
    m_customPresetMenu->addAction("➕ New Custom Profile...", this, &MainWindow::openCreateCustomPresetDialog);
    if (!presets.isEmpty()) {
        m_customPresetMenu->addAction("✏️ Edit Current Profile...", this, &MainWindow::openEditCustomPresetDialog);
        m_customPresetMenu->addAction("🗑️ Delete Current Profile", this, &MainWindow::deleteSelectedCustomPreset);
    }
}

void MainWindow::openCreateCustomPresetDialog() {
    // Pre-fill with the exact values currently in the duration boxes
    int currentWork = durationToSeconds(m_comboWorkVal, m_comboWorkUnit);
    int currentBreak = durationToSeconds(m_comboBreakVal, m_comboBreakUnit);

    CustomPreset initial;
    initial.name = "";
    initial.workDurationSeconds = (currentWork > 0) ? currentWork : 1200;
    initial.breakDurationSeconds = (currentBreak > 0) ? currentBreak : 20;

    CustomPresetDialog dlg(&initial, false, this);
    if (dlg.exec() == QDialog::Accepted) {
        CustomPreset p = dlg.preset();
        m_settings->saveCustomPreset(p);
        rebuildCustomPresetMenu();
        applyPreset(p.workDurationSeconds, p.breakDurationSeconds, p.name);
    }
}

void MainWindow::openEditCustomPresetDialog() {
    QList<CustomPreset> presets = m_settings->customPresets();
    if (presets.isEmpty()) return;

    QString active = m_settings->activePresetName();
    const CustomPreset* toEdit = &presets[0];
    for (const CustomPreset& p : presets) {
        if (p.name == active) {
            toEdit = &p;
            break;
        }
    }

    CustomPresetDialog dlg(toEdit, true, this);
    if (dlg.exec() == QDialog::Accepted) {
        CustomPreset p = dlg.preset();
        m_settings->saveCustomPreset(p);
        rebuildCustomPresetMenu();
        applyPreset(p.workDurationSeconds, p.breakDurationSeconds, p.name);
    }
}

void MainWindow::deleteSelectedCustomPreset() {
    QList<CustomPreset> presets = m_settings->customPresets();
    if (presets.isEmpty()) return;

    QString active = m_settings->activePresetName();
    QString toDelete = presets[0].name;
    for (const CustomPreset& p : presets) {
        if (p.name == active) {
            toDelete = p.name;
            break;
        }
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Custom Preset",
        QString("Are you sure you want to delete profile '%1'?").arg(toDelete),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        m_settings->deleteCustomPreset(toDelete);
        rebuildCustomPresetMenu();
        applyPreset(20 * 60, 20, "20-20-20");
    }
}

QWidget* MainWindow::createSettingsTab() {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    QWidget* tabContent = new QWidget();
    tabContent->setStyleSheet("background: transparent;");

    QVBoxLayout* layout = new QVBoxLayout(tabContent);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(14);

    // Group 1: Timer Durations
    QGroupBox* timerGroup = new QGroupBox("Timer Durations");
    QVBoxLayout* timerGroupLayout = new QVBoxLayout(timerGroup);
    timerGroupLayout->setSpacing(10);
    timerGroupLayout->setContentsMargins(12, 14, 12, 14);

    QFormLayout* timerForm = new QFormLayout();
    timerForm->setSpacing(10);
    timerForm->setLabelAlignment(Qt::AlignLeft);

    // Work Interval controls
    QHBoxLayout* workLayout = new QHBoxLayout();
    m_comboWorkVal = new QComboBox();
    m_comboWorkVal->setEditable(true);
    m_comboWorkVal->setValidator(new QIntValidator(1, 9999, m_comboWorkVal));
    m_comboWorkVal->addItems({"5", "10", "15", "20", "25", "30", "45", "50", "60"});
    m_comboWorkUnit = new QComboBox();
    m_comboWorkUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboWorkUnit->setFixedWidth(105);
    workLayout->addWidget(m_comboWorkVal, 1);
    workLayout->addWidget(m_comboWorkUnit);

    // Break Duration controls
    QHBoxLayout* breakLayout = new QHBoxLayout();
    m_comboBreakVal = new QComboBox();
    m_comboBreakVal->setEditable(true);
    m_comboBreakVal->setValidator(new QIntValidator(1, 9999, m_comboBreakVal));
    m_comboBreakVal->addItems({"5", "10", "15", "20", "25", "30", "45", "50", "60"});
    m_comboBreakUnit = new QComboBox();
    m_comboBreakUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboBreakUnit->setFixedWidth(105);
    breakLayout->addWidget(m_comboBreakVal, 1);
    breakLayout->addWidget(m_comboBreakUnit);

    timerForm->addRow("Work Interval:", workLayout);
    timerForm->addRow("Break Duration:", breakLayout);
    timerGroupLayout->addLayout(timerForm);

    // Save as Profile action row directly below timer inputs
    QHBoxLayout* profileActionLayout = new QHBoxLayout();
    profileActionLayout->addStretch();
    m_btnSaveCurrentAsProfile = new QPushButton("💾 Save as Profile...");
    m_btnSaveCurrentAsProfile->setObjectName("btnSmall");
    m_btnSaveCurrentAsProfile->setFixedHeight(30);
    m_btnSaveCurrentAsProfile->setToolTip("Save the above Work and Break durations into a named custom profile.");
    profileActionLayout->addWidget(m_btnSaveCurrentAsProfile);
    timerGroupLayout->addLayout(profileActionLayout);

    layout->addWidget(timerGroup);

    // Group 2: Break Window && Alerts
    QGroupBox* alertsGroup = new QGroupBox("Break Window && Alerts");
    QVBoxLayout* alertsLayout = new QVBoxLayout(alertsGroup);
    alertsLayout->setSpacing(10);
    alertsLayout->setContentsMargins(12, 14, 12, 14);

    m_chkBreakWindow = new QCheckBox("Show visual break window during rest breaks");
    alertsLayout->addWidget(m_chkBreakWindow);

    QHBoxLayout* styleLayout = new QHBoxLayout();
    styleLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblStyle = new QLabel("Window style:");
    lblStyle->setStyleSheet("color: #94a3b8;");
    m_comboBreakStyle = new QComboBox();
    m_comboBreakStyle->addItem("Centered popup window (Floating alert card)", "popup");
    m_comboBreakStyle->addItem("Full-screen shield (Blocks all displays)", "fullscreen");
    styleLayout->addWidget(lblStyle);
    styleLayout->addWidget(m_comboBreakStyle, 1);
    alertsLayout->addLayout(styleLayout);

    m_chkAudioEnabled = new QCheckBox("Play sound on interval finish");
    alertsLayout->addWidget(m_chkAudioEnabled);

    QHBoxLayout* volLayout = new QHBoxLayout();
    volLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblVol = new QLabel("Volume:");
    lblVol->setStyleSheet("color: #94a3b8;");
    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_lblVolumeVal = new QLabel("80%");
    m_lblVolumeVal->setFixedWidth(38);
    m_lblVolumeVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_btnTestAudio = new QPushButton("Test");
    m_btnTestAudio->setObjectName("btnSmall");
    m_btnTestAudio->setFixedHeight(26);

    volLayout->addWidget(lblVol);
    volLayout->addWidget(m_sliderVolume, 1);
    volLayout->addWidget(m_lblVolumeVal);
    volLayout->addWidget(m_btnTestAudio);
    alertsLayout->addLayout(volLayout);

    m_chkNotificationsEnabled = new QCheckBox("Show system popups (tray notifications)");
    alertsLayout->addWidget(m_chkNotificationsEnabled);

    layout->addWidget(alertsGroup);

    // Group 3: Application Preferences
    QGroupBox* appGroup = new QGroupBox("Application Preferences");
    QVBoxLayout* appLayout = new QVBoxLayout(appGroup);
    appLayout->setSpacing(10);
    appLayout->setContentsMargins(12, 14, 12, 14);

    m_chkCloseToTray = new QCheckBox("Minimize to system tray on close");
    m_chkIdleDetection = new QCheckBox("Auto-pause timer when system is idle");

    QHBoxLayout* idleLayout = new QHBoxLayout();
    idleLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblIdle = new QLabel("Idle threshold:");
    lblIdle->setStyleSheet("color: #94a3b8;");
    m_comboIdleVal = new QComboBox();
    m_comboIdleVal->setEditable(true);
    m_comboIdleVal->setValidator(new QIntValidator(1, 9999, m_comboIdleVal));
    m_comboIdleVal->addItems({"1", "2", "3", "5", "10", "15"});
    m_comboIdleVal->setFixedWidth(75);
    m_comboIdleUnit = new QComboBox();
    m_comboIdleUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboIdleUnit->setFixedWidth(105);
    idleLayout->addWidget(lblIdle);
    idleLayout->addWidget(m_comboIdleVal);
    idleLayout->addWidget(m_comboIdleUnit);
    idleLayout->addStretch();

    m_chkAutostart = new QCheckBox("Launch automatically at system startup");

    appLayout->addWidget(m_chkCloseToTray);
    appLayout->addWidget(m_chkIdleDetection);
    appLayout->addLayout(idleLayout);
    appLayout->addWidget(m_chkAutostart);

    layout->addWidget(appGroup);

    // Reset Defaults Action Button
    QPushButton* btnResetDefaults = new QPushButton("↺ Reset All Settings to Default");
    btnResetDefaults->setObjectName("btnSecondary");
    btnResetDefaults->setFixedHeight(36);
    btnResetDefaults->setToolTip("Restore all timer intervals, audio, notification, and display settings to the factory 20-20-20 default.");
    connect(btnResetDefaults, &QPushButton::clicked, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Reset to Default Settings",
            "Are you sure you want to reset all timer settings, audio preferences, and alert behaviors back to their default values (20-20-20 rule)?\n\nYour custom profiles will be kept.",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            m_settings->resetAllToDefaults();
            loadSettingsToUi();
            m_timerEngine->stop();
            QMessageBox::information(this, "Settings Reset", "All settings have been restored to defaults.");
        }
    });
    layout->addWidget(btnResetDefaults);

    layout->addStretch();

    // Helper to configure each combobox with dark QListView and avoid duplicate hover events
    auto saveLambda = [this]() {
        if (!m_isUpdatingUi) {
            saveSettingsFromUi();
        }
    };

    auto setupCombo = [saveLambda](QComboBox* combo) {
        combo->setCompleter(nullptr);
        combo->setMaxVisibleItems(6);

        QListView* listView = new QListView(combo);
        listView->setMouseTracking(true);

        QPalette pal = listView->palette();
        pal.setColor(QPalette::Base, QColor("#0f172a"));
        pal.setColor(QPalette::Window, QColor("#0f172a"));
        pal.setColor(QPalette::Text, QColor("#f8fafc"));
        pal.setColor(QPalette::Highlight, QColor("#0284c7"));
        pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
        listView->setPalette(pal);

        listView->setStyleSheet(R"(
            QListView {
                background-color: #0f172a;
                color: #f8fafc;
                border: 1px solid #38bdf8;
                border-radius: 6px;
                padding: 4px;
                outline: none;
            }
            QListView::item {
                background-color: #0f172a;
                color: #f8fafc;
                padding: 6px 12px;
                border-radius: 4px;
                min-height: 22px;
            }
            QListView::item:selected:!hover {
                background-color: #1e293b;
                color: #38bdf8;
            }
            QListView::item:hover,
            QListView::item:selected:hover {
                background-color: #0284c7;
                color: #ffffff;
            }
        )");
        combo->setView(listView);

        QObject::connect(combo, QOverload<int>::of(&QComboBox::activated), [saveLambda](int) {
            saveLambda();
        });
        if (combo->isEditable() && combo->lineEdit()) {
            QObject::connect(combo->lineEdit(), &QLineEdit::editingFinished, [saveLambda]() {
                saveLambda();
            });
        }
    };

    setupCombo(m_comboWorkVal);
    setupCombo(m_comboWorkUnit);
    setupCombo(m_comboBreakVal);
    setupCombo(m_comboBreakUnit);
    setupCombo(m_comboBreakStyle);
    setupCombo(m_comboIdleVal);
    setupCombo(m_comboIdleUnit);

    connect(m_chkBreakWindow, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboBreakStyle->setEnabled(checked);
        saveLambda();
    });

    connect(m_chkAudioEnabled, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_sliderVolume->setEnabled(checked);
        m_btnTestAudio->setEnabled(checked);
        saveLambda();
    });
    connect(m_chkNotificationsEnabled, &QCheckBox::toggled, saveLambda);
    connect(m_chkCloseToTray, &QCheckBox::toggled, saveLambda);
    connect(m_chkAutostart, &QCheckBox::toggled, saveLambda);
    connect(m_chkIdleDetection, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboIdleVal->setEnabled(checked);
        m_comboIdleUnit->setEnabled(checked);
        saveLambda();
    });

    connect(m_sliderVolume, &QSlider::valueChanged, [this, saveLambda](int val) {
        m_lblVolumeVal->setText(QString("%1%").arg(val));
        saveLambda();
    });

    connect(m_btnTestAudio, &QPushButton::clicked, [this]() {
        m_audioManager->playTestChime();
    });

    connect(m_btnSaveCurrentAsProfile, &QPushButton::clicked, this, &MainWindow::openCreateCustomPresetDialog);

    scrollArea->setWidget(tabContent);
    return scrollArea;
}

void MainWindow::loadSettingsToUi() {
    m_isUpdatingUi = true;
    secondsToUi(m_settings->workDurationSeconds(), m_comboWorkVal, m_comboWorkUnit);
    secondsToUi(m_settings->breakDurationSeconds(), m_comboBreakVal, m_comboBreakUnit);

    m_chkBreakWindow->setChecked(m_settings->breakWindowEnabled());
    int styleIdx = m_comboBreakStyle->findData(m_settings->breakWindowStyle());
    if (styleIdx >= 0) m_comboBreakStyle->setCurrentIndex(styleIdx);
    m_comboBreakStyle->setEnabled(m_settings->breakWindowEnabled());

    m_chkAudioEnabled->setChecked(m_settings->audioEnabled());
    m_sliderVolume->setValue(m_settings->volume());
    m_sliderVolume->setEnabled(m_settings->audioEnabled());
    m_btnTestAudio->setEnabled(m_settings->audioEnabled());
    m_lblVolumeVal->setText(QString("%1%").arg(m_settings->volume()));

    m_chkNotificationsEnabled->setChecked(m_settings->notificationsEnabled());
    m_chkCloseToTray->setChecked(m_settings->closeToTray());
    m_chkAutostart->setChecked(m_settings->autostart());
    m_chkIdleDetection->setChecked(m_settings->idleDetectionEnabled());
    secondsToUi(m_settings->idleThresholdSeconds(), m_comboIdleVal, m_comboIdleUnit);
    m_comboIdleVal->setEnabled(m_settings->idleDetectionEnabled());
    m_comboIdleUnit->setEnabled(m_settings->idleDetectionEnabled());

    m_isUpdatingUi = false;
    updateActivePresetHighlight();
}

void MainWindow::saveSettingsFromUi() {
    int workSecs = durationToSeconds(m_comboWorkVal, m_comboWorkUnit);
    int breakSecs = durationToSeconds(m_comboBreakVal, m_comboBreakUnit);

    m_settings->setWorkDurationSeconds(workSecs);
    m_settings->setBreakDurationSeconds(breakSecs);
    m_settings->setBreakWindowEnabled(m_chkBreakWindow->isChecked());
    m_settings->setBreakWindowStyle(m_comboBreakStyle->currentData().toString());
    m_settings->setAudioEnabled(m_chkAudioEnabled->isChecked());
    m_settings->setVolume(m_sliderVolume->value());
    m_settings->setNotificationsEnabled(m_chkNotificationsEnabled->isChecked());
    m_settings->setCloseToTray(m_chkCloseToTray->isChecked());
    m_settings->setAutostart(m_chkAutostart->isChecked());
    m_settings->setIdleDetectionEnabled(m_chkIdleDetection->isChecked());
    m_settings->setIdleThresholdSeconds(durationToSeconds(m_comboIdleVal, m_comboIdleUnit));

    updateActivePresetHighlight();
}

void MainWindow::applyPreset(int workSecs, int breakSecs, const QString& presetName) {
    m_isUpdatingUi = true;
    secondsToUi(workSecs, m_comboWorkVal, m_comboWorkUnit);
    secondsToUi(breakSecs, m_comboBreakVal, m_comboBreakUnit);
    m_isUpdatingUi = false;

    m_settings->setWorkDurationSeconds(workSecs);
    m_settings->setBreakDurationSeconds(breakSecs);
    m_settings->setActivePresetName(presetName);

    if (m_timerEngine->state() == TimerEngine::State::Idle) {
        m_timerEngine->stop();
    }

    updateActivePresetHighlight();
    rebuildCustomPresetMenu();
}

void MainWindow::updateActivePresetHighlight() {
    int workSecs = m_settings->workDurationSeconds();
    int breakSecs = m_settings->breakDurationSeconds();

    m_btnPreset20->setChecked(false);
    m_btnPreset25->setChecked(false);
    m_btnPreset50->setChecked(false);
    m_btnPresetCustom->setChecked(false);

    if (workSecs == 1200 && breakSecs == 20) {
        m_btnPreset20->setChecked(true);
        m_btnPresetCustom->setText("Custom ▾");
    } else if (workSecs == 1500 && breakSecs == 300) {
        m_btnPreset25->setChecked(true);
        m_btnPresetCustom->setText("Custom ▾");
    } else if (workSecs == 3000 && breakSecs == 600) {
        m_btnPreset50->setChecked(true);
        m_btnPresetCustom->setText("Custom ▾");
    } else {
        m_btnPresetCustom->setChecked(true);
        QList<CustomPreset> presets = m_settings->customPresets();
        QString matchName;
        for (const CustomPreset& p : presets) {
            if (p.workDurationSeconds == workSecs && p.breakDurationSeconds == breakSecs) {
                matchName = p.name;
                break;
            }
        }
        if (!matchName.isEmpty()) {
            QString shortName = matchName;
            if (shortName.length() > 9) shortName = shortName.left(8) + "..";
            m_btnPresetCustom->setText(shortName + " ▾");
        } else {
            m_btnPresetCustom->setText("Custom ▾");
        }
    }
}

void MainWindow::updateStatsDisplay() {
    m_lblStatCompleted->setText(QString::number(m_settings->breaksCompletedToday()));
    m_lblStatSkipped->setText(QString::number(m_settings->breaksSkippedToday()));
    double restMins = static_cast<double>(m_settings->eyeRestSecondsToday()) / 60.0;
    m_lblStatRestTime->setText(QString("%1m").arg(restMins, 0, 'f', 1));
}

void MainWindow::showSettingsTab() {
    show();
    raise();
    activateWindow();
    m_tabWidget->setCurrentIndex(1);
}

void MainWindow::updateUiForState(TimerEngine::State newState, TimerEngine::State oldState) {
    Q_UNUSED(oldState);
    switch (newState) {
    case TimerEngine::State::Working:
        m_statusBadgeLabel->setText("WORKING SESSION");
        m_statusBadgeLabel->setStyleSheet("background-color: #0284c7; color: #ffffff;");
        m_btnPlayPause->setText("Pause");
        m_btnSkipBreak->setEnabled(false);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;

    case TimerEngine::State::Breaking:
        m_statusBadgeLabel->setText("LOOK 20 FEET AWAY! 👁️");
        m_statusBadgeLabel->setStyleSheet("background-color: #d97706; color: #ffffff;");
        m_btnPlayPause->setText("Pause");
        m_btnSkipBreak->setEnabled(true);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();

        if (m_settings->breakWindowEnabled()) {
            if (m_settings->breakWindowStyle() == "popup") {
                QScreen* screen = QGuiApplication::primaryScreen();
                BreakOverlayWidget* popup = new BreakOverlayWidget(BreakOverlayWidget::DisplayMode::CenteredPopup);
                m_breakOverlays.append(popup);
                connect(popup, &BreakOverlayWidget::skipRequested, m_timerEngine, &TimerEngine::skipBreak);

                QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
                int w = 460;
                int h = 320;
                int x = screenGeom.x() + (screenGeom.width() - w) / 2;
                int y = screenGeom.y() + (screenGeom.height() - h) / 2;
                popup->setGeometry(x, y, w, h);
                popup->show();
                popup->raise();
                popup->activateWindow();
            } else {
                const QList<QScreen*> screens = QGuiApplication::screens();
                for (QScreen* screen : screens) {
                    BreakOverlayWidget* overlay = new BreakOverlayWidget(BreakOverlayWidget::DisplayMode::FullScreen);
                    m_breakOverlays.append(overlay);
                    connect(overlay, &BreakOverlayWidget::skipRequested, m_timerEngine, &TimerEngine::skipBreak);
                    overlay->setGeometry(screen->geometry());
                    overlay->showFullScreen();
                    overlay->raise();
                    overlay->activateWindow();
                }
            }
        }
        break;

    case TimerEngine::State::Paused:
        if (m_timerEngine->isPausedForIdle()) {
            m_statusBadgeLabel->setText("PAUSED (SYSTEM IDLE)");
            m_statusBadgeLabel->setStyleSheet("background-color: #64748b; color: #ffffff;");
        } else {
            m_statusBadgeLabel->setText("PAUSED");
            m_statusBadgeLabel->setStyleSheet("background-color: #475569; color: #ffffff;");
        }
        m_btnPlayPause->setText("Resume");
        m_btnSkipBreak->setEnabled(m_timerEngine->secondsRemaining() == m_settings->breakDurationSeconds() || oldState == TimerEngine::State::Breaking);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;

    case TimerEngine::State::Idle:
    default:
        m_statusBadgeLabel->setText("READY TO WORK");
        m_statusBadgeLabel->setStyleSheet("background-color: #334155; color: #94a3b8;");
        m_btnPlayPause->setText("Start");
        m_btnSkipBreak->setEnabled(false);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;
    }
}

void MainWindow::updateCountdown(int secondsRemaining, int totalSeconds) {
    int mins = secondsRemaining / 60;
    int secs = secondsRemaining % 60;
    m_countdownLabel->setText(QString("%1:%2")
                                   .arg(mins, 2, 10, QChar('0'))
                                   .arg(secs, 2, 10, QChar('0')));

    if (totalSeconds > 0) {
        int pct = static_cast<int>((static_cast<double>(secondsRemaining) / totalSeconds) * 100.0);
        m_progressBar->setValue(pct);
    } else {
        m_progressBar->setValue(100);
    }

    if (m_timerEngine->state() == TimerEngine::State::Breaking) {
        for (BreakOverlayWidget* overlay : m_breakOverlays) {
            overlay->updateCountdown(secondsRemaining, totalSeconds);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_settings->closeToTray()) {
        event->ignore();
        hide();
    } else {
        event->accept();
        QApplication::quit();
    }
}

void MainWindow::applyTheme() {
    QString qss = R"(
        QMainWindow {
            background-color: #0f172a;
        }
        QWidget {
            color: #f8fafc;
            font-family: 'Segoe UI', system-ui, sans-serif;
            font-size: 13px;
        }
        QTabWidget::pane {
            border: 1px solid #334155;
            background-color: #1e293b;
            border-radius: 8px;
        }
        QTabBar::tab {
            background-color: #0f172a;
            color: #94a3b8;
            padding: 8px 18px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            background-color: #1e293b;
            color: #38bdf8;
            border-bottom: 2px solid #38bdf8;
        }
        #headerTitle {
            font-size: 20px;
            font-weight: 700;
            color: #f8fafc;
        }
        #statusBadge {
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 700;
            letter-spacing: 1px;
            font-size: 12px;
        }
        #timerCard {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 12px;
        }
        #countdownDisplay {
            font-size: 50px;
            font-weight: 700;
            color: #38bdf8;
            font-family: 'Consolas', 'Courier New', monospace;
        }
        QProgressBar#sessionProgress {
            background-color: #1e293b;
            border: 1px solid #334155;
            border-radius: 4px;
            height: 8px;
        }
        QProgressBar#sessionProgress::chunk {
            background-color: #38bdf8;
            border-radius: 3px;
        }
        QPushButton#btnPrimary {
            background-color: #0284c7;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            font-weight: 700;
            font-size: 14px;
        }
        QPushButton#btnPrimary:hover {
            background-color: #0369a1;
        }
        QPushButton#btnPrimary:pressed {
            background-color: #075985;
        }
        QPushButton#btnSecondary {
            background-color: #334155;
            color: #f8fafc;
            border: none;
            border-radius: 6px;
            font-weight: 600;
            font-size: 13px;
        }
        QPushButton#btnSecondary:hover {
            background-color: #475569;
        }
        QPushButton#btnSmall {
            background-color: #334155;
            color: #38bdf8;
            border: 1px solid #475569;
            border-radius: 6px;
            padding: 4px 10px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton#btnSmall:hover {
            background-color: #0284c7;
            color: #ffffff;
            border-color: #38bdf8;
        }
        QPushButton#btnPreset {
            background-color: #1e293b;
            color: #94a3b8;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 6px 12px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton#btnPreset:hover {
            background-color: #334155;
            color: #38bdf8;
            border: 1px solid #475569;
        }
        QPushButton#btnPreset:checked {
            background-color: #0284c7;
            color: #ffffff;
            border: 1.5px solid #38bdf8;
            font-weight: 700;
        }
        QPushButton#btnPreset::menu-indicator {
            image: none;
            width: 0px;
        }
        QMenu {
            background-color: #1e293b;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 4px;
            color: #f8fafc;
            font-size: 13px;
        }
        QMenu::item {
            padding: 6px 14px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background-color: #0284c7;
            color: #ffffff;
        }
        QMenu::separator {
            height: 1px;
            background-color: #334155;
            margin: 4px 8px;
        }
        QToolTip {
            background-color: #0f172a;
            color: #f8fafc;
            border: 1px solid #38bdf8;
            border-radius: 6px;
            padding: 8px 12px;
            font-size: 12px;
        }
        QGroupBox {
            font-weight: 700;
            border: 1px solid #334155;
            border-radius: 8px;
            margin-top: 10px;
            padding-top: 14px;
            background-color: transparent;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
            color: #38bdf8;
        }
        QSpinBox, QComboBox {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 4px 8px;
            color: #f8fafc;
        }
        QComboBox QAbstractItemView,
        QComboBox QListView {
            background-color: #0f172a;
            color: #f8fafc;
            border: 1px solid #38bdf8;
            border-radius: 6px;
            padding: 4px;
            outline: none;
        }
        QComboBox QAbstractItemView::item,
        QComboBox QListView::item {
            background-color: #0f172a;
            color: #f8fafc;
            padding: 6px 12px;
            border-radius: 4px;
            min-height: 22px;
        }
        QComboBox QAbstractItemView::item:selected:!hover,
        QComboBox QListView::item:selected:!hover {
            background-color: #1e293b;
            color: #38bdf8;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QAbstractItemView::item:selected:hover,
        QComboBox QListView::item:hover,
        QComboBox QListView::item:selected:hover {
            background-color: #0284c7;
            color: #ffffff;
        }
        QSlider {
            background: transparent;
            border: none;
            height: 24px;
        }
        QSlider::groove:horizontal {
            border: none;
            height: 6px;
            background-color: #334155;
            border-radius: 3px;
        }
        QSlider::sub-page:horizontal {
            background-color: #0284c7;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background-color: #38bdf8;
            border: 2px solid #ffffff;
            width: 16px;
            height: 16px;
            margin: -5px 0;
            border-radius: 8px;
        }
        QSlider::handle:horizontal:hover {
            background-color: #ffffff;
            border-color: #38bdf8;
        }
        QCheckBox {
            spacing: 8px;
            color: #f8fafc;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 1px solid #475569;
            background-color: #0f172a;
        }
        QCheckBox::indicator:checked {
            background-color: #0284c7;
            border-color: #38bdf8;
        }
        QScrollBar:horizontal {
            height: 0px;
            background: transparent;
        }
        QScrollBar:vertical {
            background-color: #0f172a;
            width: 8px;
            margin: 0px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background-color: #334155;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #475569;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
            width: 0px;
        }
    )";
    qApp->setStyleSheet(qss);
}
