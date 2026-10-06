#include "MainWindow.h"
#include "ScreenFlashWidget.h"
#include "LinuxIdleDetector.h"
#include "GlobalHotkeyManager.h"
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
#include <QFileDialog>
#include <functional>

MainWindow::MainWindow(TimerEngine* timerEngine, SettingsManager* settings, AudioManager* audioManager, QWidget* parent)
    : QMainWindow(parent),
      m_timerEngine(timerEngine),
      m_settings(settings),
      m_audioManager(audioManager),
      m_isUpdatingUi(false),
      m_hotkeyManager(nullptr) {

    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setWindowTitle("LookAway - 20-20-20 Eye Care");
    setFixedSize(540, 750);

    m_hotkeyManager = new GlobalHotkeyManager(m_settings, this);
    connect(m_hotkeyManager, &GlobalHotkeyManager::togglePauseResumeRequested, this, [this]() {
        if (m_btnPlayPause) {
            m_btnPlayPause->click();
        }
    });
    connect(m_hotkeyManager, &GlobalHotkeyManager::snoozeBreakRequested, this, [this]() {
        if (m_btnPostponeBreak && m_btnPostponeBreak->isEnabled() && m_timerEngine->state() == TimerEngine::State::Breaking) {
            m_timerEngine->postponeBreak(m_settings->defaultPostponeSeconds());
        }
    });
    connect(m_hotkeyManager, &GlobalHotkeyManager::skipBreakRequested, this, [this]() {
        if (!m_settings->forceDisableSkip() && m_timerEngine->state() == TimerEngine::State::Breaking) {
            m_timerEngine->skipBreak();
        }
    });
    connect(m_hotkeyManager, &GlobalHotkeyManager::toggleDndRequested, this, [this]() {
        if (m_timerEngine->isDndActive()) {
            m_timerEngine->disableDnd();
        } else {
            m_timerEngine->enableDnd(3600);
        }
    });
    connect(m_timerEngine, &TimerEngine::breakCompleted, this, [this]() {
        if (m_settings->screenFlashEnabled()) {
            ScreenFlashWidget::flashAllScreens(m_settings->screenFlashStyle());
        }
    });

    setupUi();
    applyTheme();

    connect(m_timerEngine, &TimerEngine::stateChanged, this, &MainWindow::updateUiForState);
    connect(m_timerEngine, &TimerEngine::tick, this, &MainWindow::updateCountdown);
    connect(m_timerEngine, &TimerEngine::dndStateChanged, this, &MainWindow::handleDndStateChanged);
    connect(m_timerEngine, &TimerEngine::dndExpired, this, [this]() {
        m_statusBadgeLabel->setText("DND ENDED - SCHEDULE RESUMED");
        m_statusBadgeLabel->setStyleSheet("background-color: #0284c7; color: #ffffff;");
    });
    connect(m_timerEngine, &TimerEngine::compoundTick, [this](int pRem, int pTot, int sRem, int sTot) {
        Q_UNUSED(pRem);
        Q_UNUSED(pTot);
        Q_UNUSED(sTot);
        if (m_settings->concurrentPresetsEnabled() && m_timerEngine->state() == TimerEngine::State::Working) {
            m_lblSecondaryTimerStatus->setVisible(true);
            QString timeStr;
            if (sRem >= 3600) {
                int hrs = sRem / 3600;
                int mins = (sRem % 3600) / 60;
                int secs = sRem % 60;
                timeStr = QString("%1:%2:%3")
                              .arg(hrs, 2, 10, QChar('0'))
                              .arg(mins, 2, 10, QChar('0'))
                              .arg(secs, 2, 10, QChar('0'));
            } else {
                int mins = sRem / 60;
                int secs = sRem % 60;
                timeStr = QString("%1:%2")
                              .arg(mins, 2, 10, QChar('0'))
                              .arg(secs, 2, 10, QChar('0'));
            }
            m_lblSecondaryTimerStatus->setText(QString("Macro Break (%1): %2")
                .arg(m_settings->secondaryPresetName())
                .arg(timeStr));
        } else {
            m_lblSecondaryTimerStatus->setVisible(false);
        }
    });
    connect(m_timerEngine, &TimerEngine::breakDeferredForFullscreen, [this]() {
        m_statusBadgeLabel->setText("BREAK POSTPONED (FULL-SCREEN APP ACTIVE)");
        m_statusBadgeLabel->setStyleSheet("background-color: #64748b; color: #ffffff;");
    });
    connect(m_timerEngine, &TimerEngine::preBreakWarning, this, &MainWindow::handlePreBreakWarning);
    connect(m_timerEngine, &TimerEngine::breakPostponed, this, [this](int postponeSecs) {
        int mins = postponeSecs / 60;
        QString text = (mins > 0 && postponeSecs % 60 == 0)
            ? QString("BREAK SNOOZED (+%1 MINS) ⏳").arg(mins)
            : QString("BREAK SNOOZED (+%1 SECS) ⏳").arg(postponeSecs);
        m_statusBadgeLabel->setText(text);
        m_statusBadgeLabel->setStyleSheet("background-color: #d97706; color: #ffffff; font-weight: 800;");
        if (m_btnPostponeBreak) {
            m_btnPostponeBreak->setEnabled(false);
        }
    });
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
    mainLayout->setSpacing(10);

    // Constant Global App Header (LookAway Logo + Title + DND Mode dropdown)
    QHBoxLayout* appHeaderLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel();
    iconLabel->setPixmap(QIcon(":/icons/app_icon.svg").pixmap(32, 32));
    QLabel* titleLabel = new QLabel("LookAway");
    titleLabel->setObjectName("headerTitle");
    appHeaderLayout->addWidget(iconLabel);
    appHeaderLayout->addWidget(titleLabel);
    appHeaderLayout->addStretch();

    m_btnDnd = new QPushButton("🔕 DND Mode ▾");
    m_btnDnd->setObjectName("btnSmall");
    m_btnDnd->setFixedHeight(28);
    m_btnDnd->setToolTip("Suppress all break overlays and audio during meetings or focus sessions.");
    m_dndMenu = new QMenu(this);
    QAction* actDnd30 = m_dndMenu->addAction("30 Minutes");
    QAction* actDnd60 = m_dndMenu->addAction("1 Hour");
    QAction* actDnd120 = m_dndMenu->addAction("2 Hours");
    QAction* actDndInf = m_dndMenu->addAction("Until Turned Off (Indefinite)");
    connect(actDnd30, &QAction::triggered, [this]() { m_timerEngine->enableDnd(30 * 60); });
    connect(actDnd60, &QAction::triggered, [this]() { m_timerEngine->enableDnd(60 * 60); });
    connect(actDnd120, &QAction::triggered, [this]() { m_timerEngine->enableDnd(120 * 60); });
    connect(actDndInf, &QAction::triggered, [this]() { m_timerEngine->enableDnd(0); });
    m_btnDnd->setMenu(m_dndMenu);

    m_btnEndDnd = new QPushButton("Turn Off DND");
    m_btnEndDnd->setObjectName("btnSmall");
    m_btnEndDnd->setFixedHeight(28);
    m_btnEndDnd->setStyleSheet("background-color: #ef4444; color: #ffffff; font-weight: 700;");
    m_btnEndDnd->setVisible(false);
    connect(m_btnEndDnd, &QPushButton::clicked, [this]() { m_timerEngine->disableDnd(); });

    appHeaderLayout->addWidget(m_btnDnd);
    appHeaderLayout->addWidget(m_btnEndDnd);
    mainLayout->addLayout(appHeaderLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createDashboardTab(), "⏱️ Dashboard");
    m_tabWidget->addTab(createAnalyticsTab(), "📈 Analytics");
    m_tabWidget->addTab(createAlertsTab(), "🔔 Alerts");
    m_tabWidget->addTab(createAudioTab(), "🎵 Audio");
    m_tabWidget->addTab(createPreferencesTab(), "⚙️ Preferences");

    mainLayout->addWidget(m_tabWidget);
}

QWidget* MainWindow::createDashboardTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

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

    m_lblSecondaryTimerStatus = new QLabel("");
    m_lblSecondaryTimerStatus->setObjectName("secondaryTimerBadge");
    m_lblSecondaryTimerStatus->setAlignment(Qt::AlignCenter);
    m_lblSecondaryTimerStatus->setStyleSheet("color: #38bdf8; font-size: 12px; font-weight: 600; padding: 4px 8px; margin-top: 6px; background: rgba(56, 189, 248, 25); border: 1px solid rgba(56, 189, 248, 80); border-radius: 6px;");
    m_lblSecondaryTimerStatus->setVisible(false);
    cardLayout->addWidget(m_lblSecondaryTimerStatus);

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

    m_btnPostponeBreak = new QPushButton("Snooze Break");
    m_btnPostponeBreak->setObjectName("btnSecondary");
    m_btnPostponeBreak->setFixedHeight(40);
    m_btnPostponeBreak->setEnabled(false);
    m_btnPostponeBreak->setToolTip("Postpone the upcoming or active break by a few minutes without counting as skipped.");
    connect(m_btnPostponeBreak, &QPushButton::clicked, [this]() {
        m_timerEngine->postponeBreak(m_settings->defaultPostponeSeconds());
    });

    m_btnSkipBreak = new QPushButton("Skip Break");
    m_btnSkipBreak->setObjectName("btnSecondary");
    m_btnSkipBreak->setFixedHeight(40);
    m_btnSkipBreak->setEnabled(false);

    btnLayout->addWidget(m_btnPlayPause);
    btnLayout->addWidget(m_btnReset);
    btnLayout->addWidget(m_btnPostponeBreak);
    btnLayout->addWidget(m_btnSkipBreak);
    layout->addLayout(btnLayout);

    // Presets Row Header with Dual Schedule toggle
    QHBoxLayout* presetHeaderLayout = new QHBoxLayout();
    QLabel* lblPreset = new QLabel("Presets:");
    lblPreset->setStyleSheet("font-weight: 700; color: #94a3b8; font-size: 12px;");
    presetHeaderLayout->addWidget(lblPreset);
    presetHeaderLayout->addStretch();

    m_btnToggleConcurrentMode = new QPushButton("Dual Schedule: Off");
    m_btnToggleConcurrentMode->setObjectName("btnSmall");
    m_btnToggleConcurrentMode->setFixedHeight(26);
    m_btnToggleConcurrentMode->setToolTip("Toggle compound scheduling (e.g. running 20-20-20 Micro Eye Break AND 50-10 Macro Break simultaneously).");
    connect(m_btnToggleConcurrentMode, &QPushButton::clicked, [this]() {
        bool enabled = !m_settings->concurrentPresetsEnabled();
        m_settings->setConcurrentPresetsEnabled(enabled);
        if (m_chkConcurrentPresets) {
            m_chkConcurrentPresets->setChecked(enabled);
        }
        if (enabled) {
            m_btnToggleConcurrentMode->setText("Dual Schedule: ON");
            m_btnToggleConcurrentMode->setStyleSheet("background-color: #0284c7; color: #ffffff; border: 1px solid #38bdf8;");
            if (m_settings->secondaryPresetName().isEmpty() || m_settings->secondaryPresetName() == m_settings->activePresetName()) {
                m_settings->setSecondaryPresetName("50-10 Work");
                m_settings->setSecondaryWorkDurationSeconds(50 * 60);
                m_settings->setSecondaryBreakDurationSeconds(600);
            }
        } else {
            m_btnToggleConcurrentMode->setText("Dual Schedule: Off");
            m_btnToggleConcurrentMode->setStyleSheet("");
            m_lblSecondaryTimerStatus->setVisible(false);
        }
        updateActivePresetHighlight();
        m_timerEngine->stop();
        m_timerEngine->start();
    });
    presetHeaderLayout->addWidget(m_btnToggleConcurrentMode);
    layout->addLayout(presetHeaderLayout);

    // Presets Row
    QHBoxLayout* presetLayout = new QHBoxLayout();
    presetLayout->setSpacing(8);

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

    auto onPresetClicked = [this](int workSecs, int breakSecs, const QString& presetName) {
        if (m_settings->concurrentPresetsEnabled()) {
            if (m_settings->workDurationSeconds() == workSecs && m_settings->breakDurationSeconds() == breakSecs) {
                // Clicking primary preset in dual mode toggles dual mode off
                m_settings->setConcurrentPresetsEnabled(false);
                m_btnToggleConcurrentMode->setText("Dual Schedule: Off");
                m_btnToggleConcurrentMode->setStyleSheet("");
                m_lblSecondaryTimerStatus->setVisible(false);
            } else {
                // Set as secondary (Macro) preset
                m_settings->setSecondaryPresetName(presetName);
                m_settings->setSecondaryWorkDurationSeconds(workSecs);
                m_settings->setSecondaryBreakDurationSeconds(breakSecs);
            }
            updateActivePresetHighlight();
            m_timerEngine->stop();
            m_timerEngine->start();
        } else {
            applyPreset(workSecs, breakSecs, presetName);
        }
    };

    connect(m_btnPreset20, &QPushButton::clicked, [onPresetClicked]() {
        onPresetClicked(20 * 60, 20, "20-20-20");
    });
    connect(m_btnPreset25, &QPushButton::clicked, [onPresetClicked]() {
        onPresetClicked(25 * 60, 300, "25-5 Pomo");
    });
    connect(m_btnPreset50, &QPushButton::clicked, [onPresetClicked]() {
        onPresetClicked(50 * 60, 600, "50-10 Work");
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

    // Today's Eye Rest & Habit Overview Card
    QFrame* overviewCard = new QFrame();
    overviewCard->setObjectName("overviewCard");
    overviewCard->setStyleSheet(
        "#overviewCard {"
        "   background-color: #0f172a;"
        "   border: 1px solid #334155;"
        "   border-radius: 10px;"
        "}"
    );
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewCard);
    overviewLayout->setContentsMargins(14, 12, 14, 12);
    overviewLayout->setSpacing(8);

    QHBoxLayout* overviewHeader = new QHBoxLayout();
    QLabel* lblOverviewTitle = new QLabel("Today's Eye Rest Overview");
    lblOverviewTitle->setStyleSheet("font-weight: 700; color: #38bdf8; font-size: 13px;");
    QPushButton* btnViewAnalytics = new QPushButton("View Full Analytics 📈");
    btnViewAnalytics->setObjectName("btnSmall");
    btnViewAnalytics->setFixedHeight(26);
    btnViewAnalytics->setToolTip("Open the dedicated Analytics tab to see your 7-day adherence chart, streaks, and compliance stats.");
    connect(btnViewAnalytics, &QPushButton::clicked, [this]() {
        m_tabWidget->setCurrentIndex(1);
    });
    overviewHeader->addWidget(lblOverviewTitle);
    overviewHeader->addStretch();
    overviewHeader->addWidget(btnViewAnalytics);
    overviewLayout->addLayout(overviewHeader);

    QLabel* lblOverviewDesc = new QLabel(
        "Consistent micro-breaks protect your vision and eliminate digital fatigue. Track weekly trends, habit streaks, and compliance in Analytics."
    );
    lblOverviewDesc->setStyleSheet("color: #94a3b8; font-size: 12px;");
    lblOverviewDesc->setWordWrap(true);
    overviewLayout->addWidget(lblOverviewDesc);

    layout->addWidget(overviewCard);
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

namespace {
void setupDarkCombo(QComboBox* combo, std::function<void()> onSave) {
    if (combo->isEditable()) {
        combo->setCompleter(nullptr);
    }
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

    combo->setView(listView);

    // Style the popup container (QComboBoxPrivateContainer) to eliminate any default frame/white borders
    if (QFrame* frame = qobject_cast<QFrame*>(combo->view()->parentWidget())) {
        frame->setObjectName("comboContainer");
        frame->setFrameShape(QFrame::NoFrame);
        frame->setLineWidth(0);
        frame->setContentsMargins(0, 0, 0, 0);
        frame->setAttribute(Qt::WA_TranslucentBackground, true);
        frame->setStyleSheet("QFrame#comboContainer { background-color: #0f172a; border: 1px solid #38bdf8; border-radius: 6px; }");
    }

    listView->setStyleSheet(R"(
        QListView {
            background-color: transparent;
            color: #f8fafc;
            border: none;
            padding: 4px;
            outline: none;
        }
        QListView::item {
            background-color: transparent;
            color: #f8fafc;
            padding: 6px 12px;
            border-radius: 4px;
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

    QObject::connect(combo, QOverload<int>::of(&QComboBox::activated), [onSave](int) {
        if (onSave) onSave();
    });
    if (combo->isEditable() && combo->lineEdit()) {
        QObject::connect(combo->lineEdit(), &QLineEdit::editingFinished, [onSave]() {
            if (onSave) onSave();
        });
    }
}
} // namespace

QWidget* MainWindow::createAnalyticsTab() {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setObjectName("analyticsScrollArea");
    scrollArea->setStyleSheet("#analyticsScrollArea { background: transparent; border: none; }");

    QWidget* tabContent = new QWidget();
    tabContent->setObjectName("tabAnalyticsContent");
    tabContent->setStyleSheet("#tabAnalyticsContent { background: transparent; }");
    tabContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout* layout = new QVBoxLayout(tabContent);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

    // Section 1: Today's Performance KPI Card
    QFrame* todayCard = new QFrame();
    todayCard->setObjectName("todayPerfCard");
    todayCard->setStyleSheet(
        "#todayPerfCard {"
        "   background-color: #1e293b;"
        "   border: 1px solid #334155;"
        "   border-radius: 12px;"
        "}"
    );
    QVBoxLayout* todayOuterLayout = new QVBoxLayout(todayCard);
    todayOuterLayout->setContentsMargins(14, 12, 14, 14);
    todayOuterLayout->setSpacing(10);

    QHBoxLayout* todayHeaderLayout = new QHBoxLayout();
    QLabel* lblTodayTitle = new QLabel("Today's Performance");
    lblTodayTitle->setStyleSheet("font-size: 13px; font-weight: 700; color: #f8fafc;");

    QLabel* lblStatsHint = new QLabel("Daily session metrics");
    lblStatsHint->setStyleSheet("color: #64748b; font-size: 11px; margin-left: 4px;");

    QPushButton* btnResetStats = new QPushButton("↺ Reset");
    btnResetStats->setObjectName("btnSmall");
    btnResetStats->setFixedHeight(24);
    btnResetStats->setStyleSheet("padding: 2px 10px; font-size: 11px;");
    btnResetStats->setToolTip("Reset today's breaks taken, snoozed, skipped, and rest time back to 0.");
    connect(btnResetStats, &QPushButton::clicked, [this]() {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Reset Today's Summary",
            "Reset today's eye care summary (breaks taken, snoozed, skipped, rest time) to zero?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            m_settings->resetDailyStats();
        }
    });

    todayHeaderLayout->addWidget(lblTodayTitle);
    todayHeaderLayout->addWidget(lblStatsHint);
    todayHeaderLayout->addStretch();
    todayHeaderLayout->addWidget(btnResetStats);
    todayOuterLayout->addLayout(todayHeaderLayout);

    QHBoxLayout* statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(8);

    auto makeStatCard = [](const QString& title, QLabel*& valLabel, const QString& color) -> QFrame* {
        QFrame* card = new QFrame();
        card->setStyleSheet("background: #0f172a; border: 1px solid #243249; border-radius: 8px;");
        QVBoxLayout* cLayout = new QVBoxLayout(card);
        cLayout->setContentsMargins(4, 8, 4, 8);
        cLayout->setSpacing(2);
        valLabel = new QLabel("0");
        valLabel->setStyleSheet(QString("font-size: 19px; font-weight: 700; color: %1;").arg(color));
        valLabel->setAlignment(Qt::AlignCenter);
        QLabel* cap = new QLabel(title);
        cap->setStyleSheet("font-size: 11px; color: #94a3b8;");
        cap->setAlignment(Qt::AlignCenter);
        cLayout->addWidget(valLabel);
        cLayout->addWidget(cap);
        return card;
    };

    QFrame* cardCompleted = makeStatCard("Breaks Taken", m_lblStatCompleted, "#38bdf8");
    QFrame* cardSnoozed = makeStatCard("Postponed", m_lblStatPostponed, "#fbbf24");
    QFrame* cardSkipped = makeStatCard("Skipped", m_lblStatSkipped, "#f43f5e");
    QFrame* cardRestTime = makeStatCard("Total Rest", m_lblStatRestTime, "#10b981");

    statsLayout->addWidget(cardCompleted, 1);
    statsLayout->addWidget(cardSnoozed, 1);
    statsLayout->addWidget(cardSkipped, 1);
    statsLayout->addWidget(cardRestTime, 1);
    todayOuterLayout->addLayout(statsLayout);

    layout->addWidget(todayCard);

    // Section 2: 7-Day Adherence Chart Card (Self-contained with header, badges & legend)
    m_weeklyAnalyticsWidget = new WeeklyAnalyticsWidget(m_settings, tabContent);
    layout->addWidget(m_weeklyAnalyticsWidget);

    // Section 3: Clinical Health Guidance Card
    QFrame* guideCard = new QFrame();
    guideCard->setObjectName("guideCard");
    guideCard->setStyleSheet(
        "#guideCard {"
        "   background-color: #1e293b;"
        "   border: 1px solid #334155;"
        "   border-radius: 12px;"
        "}"
    );
    QVBoxLayout* guideLayout = new QVBoxLayout(guideCard);
    guideLayout->setContentsMargins(14, 12, 14, 12);
    guideLayout->setSpacing(6);

    QLabel* lblGuideTitle = new QLabel("💡 Clinical Eye Care Insights");
    lblGuideTitle->setStyleSheet("font-weight: 700; color: #38bdf8; font-size: 12px;");
    guideLayout->addWidget(lblGuideTitle);

    QLabel* lblTip1 = new QLabel(
        "• <b>The 20-20-20 Rule:</b> Every 20 minutes, gaze 20 feet away for 20 seconds to relax ciliary muscle spasm and alleviate digital strain."
    );
    lblTip1->setStyleSheet("color: #94a3b8; font-size: 11px;");
    lblTip1->setWordWrap(true);
    guideLayout->addWidget(lblTip1);

    QLabel* lblTip2 = new QLabel(
        "• <b>Tear Film Restoration:</b> Screen work cuts blink rate by up to 66%. Consistent micro-rests restore natural ocular hydration and prevent fatigue."
    );
    lblTip2->setStyleSheet("color: #94a3b8; font-size: 11px;");
    lblTip2->setWordWrap(true);
    guideLayout->addWidget(lblTip2);

    layout->addWidget(guideCard);
    layout->addStretch();

    scrollArea->setWidget(tabContent);
    return scrollArea;
}

QWidget* MainWindow::createAlertsTab() {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setObjectName("alertsScrollArea");
    scrollArea->setStyleSheet("#alertsScrollArea { background: transparent; border: none; }");

    QWidget* tabContent = new QWidget();
    tabContent->setObjectName("tabAlertsContent");
    tabContent->setStyleSheet("#tabAlertsContent { background: transparent; }");
    tabContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout* layout = new QVBoxLayout(tabContent);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

    auto saveLambda = [this]() {
        if (!m_isUpdatingUi) {
            saveSettingsFromUi();
        }
    };

    // Group 1: Break Window Display
    QGroupBox* windowGroup = new QGroupBox("Break Window Display");
    QVBoxLayout* windowLayout = new QVBoxLayout(windowGroup);
    windowLayout->setSpacing(10);
    windowLayout->setContentsMargins(12, 14, 12, 14);

    m_chkBreakWindow = new QCheckBox("Show visual break window during rest breaks");
    windowLayout->addWidget(m_chkBreakWindow);

    QHBoxLayout* styleLayout = new QHBoxLayout();
    styleLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblStyle = new QLabel("Window style:");
    lblStyle->setStyleSheet("color: #94a3b8;");
    m_comboBreakStyle = new QComboBox();
    m_comboBreakStyle->addItem("Centered popup window (Rest reminder)", "popup");
    m_comboBreakStyle->addItem("Ambient border glow (Perimeter halo)", "border");
    m_comboBreakStyle->addItem("Full-screen shield (Blocks all displays)", "fullscreen");
    m_comboBreakStyle->setFixedWidth(300);
    styleLayout->addWidget(lblStyle);
    styleLayout->addWidget(m_comboBreakStyle);
    styleLayout->addStretch();
    windowLayout->addLayout(styleLayout);

    m_chkForceDisableSkip = new QCheckBox("Strict Mode: Hide 'Skip Break' button during rest");
    m_chkForceDisableSkip->setToolTip("Prevents skipping breaks via button or Escape key during rest sessions.");
    windowLayout->addWidget(m_chkForceDisableSkip);

    m_chkSuppressOnFullscreen = new QCheckBox("Postpone breaks during full-screen apps");
    m_chkSuppressOnFullscreen->setToolTip("Automatically defers breaks when a full-screen video player or game is active.");
    windowLayout->addWidget(m_chkSuppressOnFullscreen);

    QLabel* lblFsDesc = new QLabel("Defers breaks when games or full-screen video players are active.");
    lblFsDesc->setStyleSheet("color: #64748b; font-size: 11px; margin-left: 24px;");
    lblFsDesc->setWordWrap(true);
    windowLayout->addWidget(lblFsDesc);

    layout->addWidget(windowGroup);

    // Group 2: Screen Flash When Break Ends
    QGroupBox* flashGroup = new QGroupBox("Screen Flash When Break Ends ✨");
    QVBoxLayout* flashLayout = new QVBoxLayout(flashGroup);
    flashLayout->setSpacing(10);
    flashLayout->setContentsMargins(12, 14, 12, 14);

    QLabel* lblFlashDesc = new QLabel("Triggers a momentary, non-intrusive luminous glow across displays when rest duration ends so you know your break is over.");
    lblFlashDesc->setWordWrap(true);
    lblFlashDesc->setStyleSheet("color: #94a3b8; font-size: 12px; margin-bottom: 2px;");
    flashLayout->addWidget(lblFlashDesc);

    m_chkScreenFlash = new QCheckBox("Flash screen when break ends");
    flashLayout->addWidget(m_chkScreenFlash);

    QHBoxLayout* flashStyleLayout = new QHBoxLayout();
    flashStyleLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblFlashStyle = new QLabel("Flash animation style:");
    lblFlashStyle->setStyleSheet("color: #94a3b8;");
    m_comboScreenFlashStyle = new QComboBox();
    m_comboScreenFlashStyle->addItem("Cyan Pulse (Subtle Glow - 400ms)", "cyan");
    m_comboScreenFlashStyle->addItem("Amber Warmth (Soft Eye Care - 500ms)", "amber");
    m_comboScreenFlashStyle->addItem("Pure White (High Visibility - 350ms)", "white");
    m_comboScreenFlashStyle->addItem("Double Ripple (Dual Pulse - 700ms)", "double");
    m_comboScreenFlashStyle->setFixedWidth(275);
    flashStyleLayout->addWidget(lblFlashStyle);
    flashStyleLayout->addWidget(m_comboScreenFlashStyle);
    flashStyleLayout->addStretch();
    flashLayout->addLayout(flashStyleLayout);

    QHBoxLayout* previewLayout = new QHBoxLayout();
    previewLayout->setContentsMargins(24, 0, 0, 0);
    m_btnPreviewFlash = new QPushButton("✨ Preview Screen Flash");
    m_btnPreviewFlash->setObjectName("btnSmall");
    m_btnPreviewFlash->setFixedHeight(30);
    m_btnPreviewFlash->setToolTip("Test and preview the selected screen flash animation immediately.");
    previewLayout->addWidget(m_btnPreviewFlash);
    previewLayout->addStretch();
    flashLayout->addLayout(previewLayout);

    layout->addWidget(flashGroup);

    // Group 3: Advance Warning & Snooze
    QGroupBox* timingGroup = new QGroupBox("Advance Warning && Snooze");
    QVBoxLayout* timingLayout = new QVBoxLayout(timingGroup);
    timingLayout->setSpacing(10);
    timingLayout->setContentsMargins(12, 14, 12, 14);

    m_chkPreBreakWarning = new QCheckBox("Pre-break 'heads-up' advance warning alert");
    m_chkPreBreakWarning->setToolTip("Notifies you before a break starts so you can wrap up typing or calls.");
    timingLayout->addWidget(m_chkPreBreakWarning);

    QHBoxLayout* warnTimeLayout = new QHBoxLayout();
    warnTimeLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblWarnTime = new QLabel("Warning lead time:");
    lblWarnTime->setStyleSheet("color: #94a3b8;");
    m_comboPreBreakWarningSecs = new QComboBox();
    m_comboPreBreakWarningSecs->addItem("15 seconds ahead", 15);
    m_comboPreBreakWarningSecs->addItem("30 seconds ahead (Recommended)", 30);
    m_comboPreBreakWarningSecs->addItem("45 seconds ahead", 45);
    m_comboPreBreakWarningSecs->addItem("60 seconds ahead", 60);
    m_comboPreBreakWarningSecs->setFixedWidth(265);
    warnTimeLayout->addWidget(lblWarnTime);
    warnTimeLayout->addWidget(m_comboPreBreakWarningSecs);
    warnTimeLayout->addStretch();
    timingLayout->addLayout(warnTimeLayout);

    m_chkPostponeBreak = new QCheckBox("Allow break snooze / postpone");
    m_chkPostponeBreak->setToolTip("Allows postponing breaks by a few minutes without counting as skipped.");
    timingLayout->addWidget(m_chkPostponeBreak);

    QHBoxLayout* postTimeLayout = new QHBoxLayout();
    postTimeLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblPostTime = new QLabel("Snooze duration:");
    lblPostTime->setStyleSheet("color: #94a3b8;");
    m_comboPostponeMins = new QComboBox();
    m_comboPostponeMins->addItem("Postpone by 1 minute", 60);
    m_comboPostponeMins->addItem("Postpone by 2 minutes (Default)", 120);
    m_comboPostponeMins->addItem("Postpone by 3 minutes", 180);
    m_comboPostponeMins->addItem("Postpone by 5 minutes", 300);
    m_comboPostponeMins->setFixedWidth(265);
    postTimeLayout->addWidget(lblPostTime);
    postTimeLayout->addWidget(m_comboPostponeMins);
    postTimeLayout->addStretch();
    timingLayout->addLayout(postTimeLayout);

    layout->addWidget(timingGroup);
    layout->addStretch();

    setupDarkCombo(m_comboBreakStyle, saveLambda);
    setupDarkCombo(m_comboScreenFlashStyle, saveLambda);
    setupDarkCombo(m_comboPreBreakWarningSecs, saveLambda);
    setupDarkCombo(m_comboPostponeMins, saveLambda);

    connect(m_chkBreakWindow, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboBreakStyle->setEnabled(checked);
        saveLambda();
    });

    connect(m_chkScreenFlash, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboScreenFlashStyle->setEnabled(checked);
        m_btnPreviewFlash->setEnabled(checked);
        saveLambda();
    });

    connect(m_btnPreviewFlash, &QPushButton::clicked, [this]() {
        QString style = m_comboScreenFlashStyle->currentData().toString();
        ScreenFlashWidget::flashAllScreens(style);
    });

    connect(m_chkPreBreakWarning, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboPreBreakWarningSecs->setEnabled(checked);
        saveLambda();
    });

    connect(m_chkPostponeBreak, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboPostponeMins->setEnabled(checked);
        saveLambda();
    });

    connect(m_chkForceDisableSkip, &QCheckBox::clicked, [this](bool checked) {
        if (checked) {
            QMessageBox::StandardButton reply = QMessageBox::warning(
                this,
                "Enable Strict Eye Rest Mode?",
                "Enabling this option will hide the 'Skip Break' button and disable the Escape key during all breaks.\n\n"
                "You will NOT be able to dismiss or bypass the break until the timer completes.\n\n"
                "Are you sure you want to enable strict enforcement?",
                QMessageBox::Yes | QMessageBox::Cancel
            );
            if (reply != QMessageBox::Yes) {
                m_chkForceDisableSkip->setChecked(false);
                return;
            }
        }
        saveSettingsFromUi();
    });

    connect(m_chkSuppressOnFullscreen, &QCheckBox::toggled, saveLambda);

    scrollArea->setWidget(tabContent);
    return scrollArea;
}

QWidget* MainWindow::createAudioTab() {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setObjectName("audioScrollArea");
    scrollArea->setStyleSheet("#audioScrollArea { background: transparent; border: none; }");

    QWidget* tabContent = new QWidget();
    tabContent->setObjectName("tabAudioContent");
    tabContent->setStyleSheet("#tabAudioContent { background: transparent; }");
    tabContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout* layout = new QVBoxLayout(tabContent);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

    auto saveLambda = [this]() {
        if (!m_isUpdatingUi) {
            saveSettingsFromUi();
        }
    };

    // Group 1: Master Audio
    QGroupBox* masterGroup = new QGroupBox("Master Audio");
    QVBoxLayout* masterLayout = new QVBoxLayout(masterGroup);
    masterLayout->setSpacing(10);
    masterLayout->setContentsMargins(12, 14, 12, 14);

    m_chkAudioEnabled = new QCheckBox("Play sound on interval finish");
    masterLayout->addWidget(m_chkAudioEnabled);

    QHBoxLayout* volLayout = new QHBoxLayout();
    volLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblVol = new QLabel("Volume:");
    lblVol->setStyleSheet("color: #94a3b8;");
    m_sliderVolume = new QSlider(Qt::Horizontal);
    m_sliderVolume->setRange(0, 100);
    m_lblVolumeVal = new QLabel("80%");
    m_lblVolumeVal->setFixedWidth(38);
    m_lblVolumeVal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    volLayout->addWidget(lblVol);
    volLayout->addWidget(m_sliderVolume, 1);
    volLayout->addWidget(m_lblVolumeVal);
    masterLayout->addLayout(volLayout);

    layout->addWidget(masterGroup);

    // Group 2: Audio Theme Packs
    QGroupBox* packsGroup = new QGroupBox("Chime Audio Packs");
    QVBoxLayout* packsLayout = new QVBoxLayout(packsGroup);
    packsLayout->setSpacing(10);
    packsLayout->setContentsMargins(12, 14, 12, 14);

    QHBoxLayout* packLayout = new QHBoxLayout();
    QLabel* lblPack = new QLabel("Sound pack:");
    lblPack->setStyleSheet("color: #94a3b8;");
    m_comboSoundPack = new QComboBox();
    m_comboSoundPack->addItem("Modern Digital (Default)", "default");
    m_comboSoundPack->addItem("Zen Singing Bowl (432Hz)", "zen");
    m_comboSoundPack->addItem("Soft Acoustic Marimba", "marimba");
    m_comboSoundPack->addItem("Subtle Crystal Bell", "bell");
    m_comboSoundPack->addItem("Custom Audio Files...", "custom");
    m_comboSoundPack->setFixedWidth(260);
    packLayout->addWidget(lblPack);
    packLayout->addWidget(m_comboSoundPack);
    packLayout->addStretch();
    packsLayout->addLayout(packLayout);

    // Custom sound file widget
    m_customSoundWidget = new QWidget();
    QVBoxLayout* customSoundLayout = new QVBoxLayout(m_customSoundWidget);
    customSoundLayout->setContentsMargins(0, 0, 0, 0);
    customSoundLayout->setSpacing(6);

    QHBoxLayout* workSoundRow = new QHBoxLayout();
    QLabel* lblWorkSound = new QLabel("Work chime:");
    lblWorkSound->setStyleSheet("color: #94a3b8;");
    lblWorkSound->setFixedWidth(90);
    m_editCustomWorkPath = new QLineEdit();
    m_editCustomWorkPath->setPlaceholderText("Select .wav or .mp3 chime...");
    m_btnBrowseCustomWork = new QPushButton("Browse...");
    m_btnBrowseCustomWork->setObjectName("btnSmall");
    m_btnBrowseCustomWork->setFixedHeight(28);
    workSoundRow->addWidget(lblWorkSound);
    workSoundRow->addWidget(m_editCustomWorkPath, 1);
    workSoundRow->addWidget(m_btnBrowseCustomWork);
    customSoundLayout->addLayout(workSoundRow);

    QHBoxLayout* breakSoundRow = new QHBoxLayout();
    QLabel* lblBreakSound = new QLabel("Break chime:");
    lblBreakSound->setStyleSheet("color: #94a3b8;");
    lblBreakSound->setFixedWidth(90);
    m_editCustomBreakPath = new QLineEdit();
    m_editCustomBreakPath->setPlaceholderText("Select .wav or .mp3 chime...");
    m_btnBrowseCustomBreak = new QPushButton("Browse...");
    m_btnBrowseCustomBreak->setObjectName("btnSmall");
    m_btnBrowseCustomBreak->setFixedHeight(28);
    breakSoundRow->addWidget(lblBreakSound);
    breakSoundRow->addWidget(m_editCustomBreakPath, 1);
    breakSoundRow->addWidget(m_btnBrowseCustomBreak);
    customSoundLayout->addLayout(breakSoundRow);

    packsLayout->addWidget(m_customSoundWidget);
    m_customSoundWidget->setVisible(false);

    // Sound test preview buttons row
    QHBoxLayout* soundTestLayout = new QHBoxLayout();
    soundTestLayout->setContentsMargins(0, 4, 0, 0);
    soundTestLayout->setSpacing(10);
    m_btnTestWorkSound = new QPushButton("▶ Test Work Chime");
    m_btnTestWorkSound->setObjectName("btnSmall");
    m_btnTestWorkSound->setFixedHeight(28);
    m_btnTestBreakSound = new QPushButton("▶ Test Break Chime");
    m_btnTestBreakSound->setObjectName("btnSmall");
    m_btnTestBreakSound->setFixedHeight(28);
    soundTestLayout->addWidget(m_btnTestWorkSound);
    soundTestLayout->addWidget(m_btnTestBreakSound);
    soundTestLayout->addStretch();
    packsLayout->addLayout(soundTestLayout);

    layout->addWidget(packsGroup);

    // Group 3: Notifications
    QGroupBox* notifyGroup = new QGroupBox("System Notifications");
    QVBoxLayout* notifyLayout = new QVBoxLayout(notifyGroup);
    notifyLayout->setSpacing(10);
    notifyLayout->setContentsMargins(12, 14, 12, 14);

    m_chkNotificationsEnabled = new QCheckBox("Show system popups (tray notifications)");
    notifyLayout->addWidget(m_chkNotificationsEnabled);

    layout->addWidget(notifyGroup);
    layout->addStretch();

    setupDarkCombo(m_comboSoundPack, saveLambda);

    connect(m_chkAudioEnabled, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_sliderVolume->setEnabled(checked);
        m_comboSoundPack->setEnabled(checked);
        m_customSoundWidget->setEnabled(checked);
        m_btnTestWorkSound->setEnabled(checked);
        m_btnTestBreakSound->setEnabled(checked);
        saveLambda();
    });

    connect(m_sliderVolume, &QSlider::valueChanged, [this, saveLambda](int val) {
        m_lblVolumeVal->setText(QString("%1%").arg(val));
        saveLambda();
    });

    connect(m_comboSoundPack, QOverload<int>::of(&QComboBox::activated), [this, saveLambda](int) {
        bool isCustom = (m_comboSoundPack->currentData().toString() == "custom");
        m_customSoundWidget->setVisible(isCustom);
        saveLambda();
    });

    connect(m_btnBrowseCustomWork, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Work Chime Audio", QString(), "Audio Files (*.wav *.mp3 *.ogg *.flac);;All Files (*.*)");
        if (!path.isEmpty()) {
            m_editCustomWorkPath->setText(path);
            saveSettingsFromUi();
        }
    });

    connect(m_btnBrowseCustomBreak, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Select Break Chime Audio", QString(), "Audio Files (*.wav *.mp3 *.ogg *.flac);;All Files (*.*)");
        if (!path.isEmpty()) {
            m_editCustomBreakPath->setText(path);
            saveSettingsFromUi();
        }
    });

    connect(m_editCustomWorkPath, &QLineEdit::editingFinished, saveLambda);
    connect(m_editCustomBreakPath, &QLineEdit::editingFinished, saveLambda);

    connect(m_btnTestWorkSound, &QPushButton::clicked, [this]() {
        m_audioManager->playTestChime("work");
    });
    connect(m_btnTestBreakSound, &QPushButton::clicked, [this]() {
        m_audioManager->playTestChime("break");
    });

    connect(m_chkNotificationsEnabled, &QCheckBox::toggled, saveLambda);

    scrollArea->setWidget(tabContent);
    return scrollArea;
}

QWidget* MainWindow::createPreferencesTab() {
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setObjectName("prefScrollArea");
    scrollArea->setStyleSheet("#prefScrollArea { background: transparent; border: none; }");

    QWidget* tabContent = new QWidget();
    tabContent->setObjectName("tabPrefContent");
    tabContent->setStyleSheet("#tabPrefContent { background: transparent; }");
    tabContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout* layout = new QVBoxLayout(tabContent);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(14);

    auto saveLambda = [this]() {
        if (!m_isUpdatingUi) {
            saveSettingsFromUi();
        }
    };

    // Group 1: Global Keyboard Shortcuts (Feature 7)
    QGroupBox* hotkeyGroup = new QGroupBox("Global Keyboard Shortcuts ⌨️");
    QVBoxLayout* hotkeyLayout = new QVBoxLayout(hotkeyGroup);
    hotkeyLayout->setSpacing(10);
    hotkeyLayout->setContentsMargins(12, 14, 12, 14);

    QLabel* lblHotkeyDesc = new QLabel("Control timer actions instantly from any application using system-wide hotkeys.");
    lblHotkeyDesc->setWordWrap(true);
    lblHotkeyDesc->setStyleSheet("color: #94a3b8; font-size: 12px; margin-bottom: 2px;");
    hotkeyLayout->addWidget(lblHotkeyDesc);

    m_chkGlobalHotkeys = new QCheckBox("Enable global system-wide shortcuts");
    hotkeyLayout->addWidget(m_chkGlobalHotkeys);

    QFormLayout* hotkeyForm = new QFormLayout();
    hotkeyForm->setSpacing(8);
    hotkeyForm->setLabelAlignment(Qt::AlignLeft);
    hotkeyForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    m_editHotkeyPause = new QLineEdit("Ctrl+Alt+P");
    m_editHotkeyPause->setPlaceholderText("Ctrl+Alt+P");
    m_editHotkeyPause->setFixedWidth(130);
    m_editHotkeyPause->setAlignment(Qt::AlignCenter);

    m_editHotkeySnooze = new QLineEdit("Ctrl+Alt+S");
    m_editHotkeySnooze->setPlaceholderText("Ctrl+Alt+S");
    m_editHotkeySnooze->setFixedWidth(130);
    m_editHotkeySnooze->setAlignment(Qt::AlignCenter);

    m_editHotkeySkip = new QLineEdit("Ctrl+Alt+K");
    m_editHotkeySkip->setPlaceholderText("Ctrl+Alt+K");
    m_editHotkeySkip->setFixedWidth(130);
    m_editHotkeySkip->setAlignment(Qt::AlignCenter);

    m_editHotkeyDnd = new QLineEdit("Ctrl+Alt+D");
    m_editHotkeyDnd->setPlaceholderText("Ctrl+Alt+D");
    m_editHotkeyDnd->setFixedWidth(130);
    m_editHotkeyDnd->setAlignment(Qt::AlignCenter);

    QString hkStyle = "font-family: 'Consolas', monospace; font-weight: 600; padding: 4px 8px;";
    m_editHotkeyPause->setStyleSheet(hkStyle);
    m_editHotkeySnooze->setStyleSheet(hkStyle);
    m_editHotkeySkip->setStyleSheet(hkStyle);
    m_editHotkeyDnd->setStyleSheet(hkStyle);

    hotkeyForm->addRow("Pause / Resume Timer:", m_editHotkeyPause);
    hotkeyForm->addRow("Snooze / Postpone Break:", m_editHotkeySnooze);
    hotkeyForm->addRow("Skip Active Break:", m_editHotkeySkip);
    hotkeyForm->addRow("Toggle DND Mode (1h):", m_editHotkeyDnd);
    hotkeyLayout->addLayout(hotkeyForm);

    QHBoxLayout* hotkeyFooterLayout = new QHBoxLayout();
    m_lblHotkeyStatus = new QLabel();
    m_lblHotkeyStatus->setStyleSheet("color: #38bdf8; font-size: 11px; font-weight: 600;");
    m_lblHotkeyStatus->setWordWrap(true);
    m_lblHotkeyStatus->setText("Backend: " + m_hotkeyManager->backendStatus());
    m_btnResetHotkeys = new QPushButton("Restore Defaults");
    m_btnResetHotkeys->setObjectName("btnSmall");
    m_btnResetHotkeys->setFixedHeight(26);
    m_btnResetHotkeys->setFixedWidth(115);
    hotkeyFooterLayout->addWidget(m_lblHotkeyStatus, 1);
    hotkeyFooterLayout->addWidget(m_btnResetHotkeys, 0, Qt::AlignRight);
    hotkeyLayout->addLayout(hotkeyFooterLayout);

#if defined(Q_OS_LINUX)
    QLabel* lblLinuxHint = new QLabel(
        "💡 <b>Linux Wayland Tip:</b> For global hotkeys while other apps are focused, add custom shortcuts in your desktop settings (e.g. GNOME / KDE) invoking: "
        "<code style='color:#38bdf8;'>lookaway --toggle</code>, <code style='color:#38bdf8;'>lookaway --snooze</code>, <code style='color:#38bdf8;'>lookaway --skip</code>, <code style='color:#38bdf8;'>lookaway --dnd</code>."
    );
    lblLinuxHint->setWordWrap(true);
    lblLinuxHint->setStyleSheet("color: #94a3b8; font-size: 11px; padding: 6px 10px; background: rgba(56, 189, 248, 15); border: 1px solid rgba(56, 189, 248, 40); border-radius: 6px;");
    hotkeyLayout->addWidget(lblLinuxHint);
#endif

    layout->addWidget(hotkeyGroup);

    // Group 2: Smart Inactivity Detection (Feature 8)
    QGroupBox* idleGroup = new QGroupBox("Smart Inactivity Detection 🐧");
    QVBoxLayout* idleLayout = new QVBoxLayout(idleGroup);
    idleLayout->setSpacing(10);
    idleLayout->setContentsMargins(12, 14, 12, 14);

    QLabel* lblIdleDesc = new QLabel("Automatically pauses your work timer when you step away from your keyboard and mouse.");
    lblIdleDesc->setWordWrap(true);
    lblIdleDesc->setStyleSheet("color: #94a3b8; font-size: 12px; margin-bottom: 2px;");
    idleLayout->addWidget(lblIdleDesc);

    m_chkIdleDetection = new QCheckBox("Auto-pause timer when system is idle");
    idleLayout->addWidget(m_chkIdleDetection);

    QHBoxLayout* idleThresholdLayout = new QHBoxLayout();
    idleThresholdLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblIdle = new QLabel("Idle threshold:");
    lblIdle->setStyleSheet("color: #94a3b8;");
    m_comboIdleVal = new QComboBox();
    m_comboIdleVal->setEditable(true);
    m_comboIdleVal->setValidator(new QIntValidator(1, 9999, m_comboIdleVal));
    m_comboIdleVal->addItems({"1", "2", "3", "5", "10", "15"});
    m_comboIdleVal->setFixedWidth(75);
    m_comboIdleUnit = new QComboBox();
    m_comboIdleUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboIdleUnit->setFixedWidth(100);
    idleThresholdLayout->addWidget(lblIdle);
    idleThresholdLayout->addWidget(m_comboIdleVal);
    idleThresholdLayout->addWidget(m_comboIdleUnit);
    idleThresholdLayout->addStretch();
    idleLayout->addLayout(idleThresholdLayout);

    m_lblIdleBackendStatus = new QLabel();
    m_lblIdleBackendStatus->setWordWrap(true);
    m_lblIdleBackendStatus->setStyleSheet("color: #10b981; font-size: 11px; font-weight: 600; padding: 4px 8px; background: rgba(16, 185, 129, 20); border: 1px solid rgba(16, 185, 129, 60); border-radius: 6px;");
#if defined(Q_OS_LINUX)
    m_lblIdleBackendStatus->setText("Active Engine: " + LinuxIdleDetector::activeBackendName());
#elif defined(Q_OS_WIN)
    m_lblIdleBackendStatus->setText("Active Engine: Win32 GetLastInputInfo (Windows Native)");
#else
    m_lblIdleBackendStatus->setText("Active Engine: System Input Monitor");
#endif
    idleLayout->addWidget(m_lblIdleBackendStatus);

    layout->addWidget(idleGroup);

    // Group 3: Timer Durations & Custom Schedules
    QGroupBox* timerGroup = new QGroupBox("Timer Durations && Intervals");
    QVBoxLayout* timerGroupLayout = new QVBoxLayout(timerGroup);
    timerGroupLayout->setSpacing(10);
    timerGroupLayout->setContentsMargins(12, 14, 12, 14);

    QFormLayout* timerForm = new QFormLayout();
    timerForm->setSpacing(10);
    timerForm->setLabelAlignment(Qt::AlignLeft);
    timerForm->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    QHBoxLayout* workLayout = new QHBoxLayout();
    m_comboWorkVal = new QComboBox();
    m_comboWorkVal->setEditable(true);
    m_comboWorkVal->setValidator(new QIntValidator(1, 9999, m_comboWorkVal));
    m_comboWorkVal->addItems({"5", "10", "15", "20", "25", "30", "45", "50", "60"});
    m_comboWorkVal->setFixedWidth(75);
    m_comboWorkUnit = new QComboBox();
    m_comboWorkUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboWorkUnit->setFixedWidth(100);
    workLayout->addWidget(m_comboWorkVal);
    workLayout->addWidget(m_comboWorkUnit);
    workLayout->addStretch();

    QHBoxLayout* breakLayout = new QHBoxLayout();
    m_comboBreakVal = new QComboBox();
    m_comboBreakVal->setEditable(true);
    m_comboBreakVal->setValidator(new QIntValidator(1, 9999, m_comboBreakVal));
    m_comboBreakVal->addItems({"5", "10", "15", "20", "25", "30", "45", "50", "60"});
    m_comboBreakVal->setFixedWidth(75);
    m_comboBreakUnit = new QComboBox();
    m_comboBreakUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboBreakUnit->setFixedWidth(100);
    breakLayout->addWidget(m_comboBreakVal);
    breakLayout->addWidget(m_comboBreakUnit);
    breakLayout->addStretch();

    timerForm->addRow("Work Interval:", workLayout);
    timerForm->addRow("Break Duration:", breakLayout);
    timerGroupLayout->addLayout(timerForm);

    QHBoxLayout* profileActionLayout = new QHBoxLayout();
    profileActionLayout->addStretch();
    m_btnSaveCurrentAsProfile = new QPushButton("💾 Save as Profile...");
    m_btnSaveCurrentAsProfile->setObjectName("btnSmall");
    m_btnSaveCurrentAsProfile->setFixedHeight(30);
    m_btnSaveCurrentAsProfile->setToolTip("Save the above Work and Break durations into a named custom profile.");
    profileActionLayout->addWidget(m_btnSaveCurrentAsProfile);
    timerGroupLayout->addLayout(profileActionLayout);

    QFrame* dualSeparator = new QFrame();
    dualSeparator->setFrameShape(QFrame::HLine);
    dualSeparator->setStyleSheet("color: #334155; margin: 4px 0px;");
    timerGroupLayout->addWidget(dualSeparator);

    m_chkConcurrentPresets = new QCheckBox("Enable dual schedule (Compound Micro && Macro breaks)");
    m_chkConcurrentPresets->setToolTip("Run a short eye rest interval (e.g. 20-20-20) and a long rest interval (e.g. 50-10) concurrently.");
    timerGroupLayout->addWidget(m_chkConcurrentPresets);

    QHBoxLayout* secondaryLayout = new QHBoxLayout();
    secondaryLayout->setContentsMargins(24, 0, 0, 0);
    QLabel* lblSecondary = new QLabel("Secondary preset:");
    lblSecondary->setStyleSheet("color: #94a3b8;");
    m_comboSecondaryPreset = new QComboBox();
    m_comboSecondaryPreset->addItem("50-10 Work (50m Work / 10m Break)", "50-10");
    m_comboSecondaryPreset->addItem("25-5 Pomo (25m Work / 5m Break)", "25-5");
    m_comboSecondaryPreset->addItem("20-20-20 (20m Work / 20s Break)", "20-20-20");
    m_comboSecondaryPreset->setFixedWidth(275);
    secondaryLayout->addWidget(lblSecondary);
    secondaryLayout->addWidget(m_comboSecondaryPreset);
    secondaryLayout->addStretch();
    timerGroupLayout->addLayout(secondaryLayout);

    layout->addWidget(timerGroup);

    // Group 4: System & Startup
    QGroupBox* appGroup = new QGroupBox("Application Preferences");
    QVBoxLayout* appLayout = new QVBoxLayout(appGroup);
    appLayout->setSpacing(10);
    appLayout->setContentsMargins(12, 14, 12, 14);

    m_chkCloseToTray = new QCheckBox("Minimize to system tray on close");
    m_chkAutostart = new QCheckBox("Launch automatically at system startup");
    appLayout->addWidget(m_chkCloseToTray);
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
            m_timerEngine->start();
            QMessageBox::information(this, "Settings Reset", "All timer intervals, alert modes, window styles, and preferences have been restored to factory defaults.");
        }
    });
    layout->addWidget(btnResetDefaults);
    layout->addStretch();

    setupDarkCombo(m_comboWorkVal, saveLambda);
    setupDarkCombo(m_comboWorkUnit, saveLambda);
    setupDarkCombo(m_comboBreakVal, saveLambda);
    setupDarkCombo(m_comboBreakUnit, saveLambda);
    setupDarkCombo(m_comboSecondaryPreset, saveLambda);
    setupDarkCombo(m_comboIdleVal, saveLambda);
    setupDarkCombo(m_comboIdleUnit, saveLambda);

    connect(m_chkGlobalHotkeys, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_editHotkeyPause->setEnabled(checked);
        m_editHotkeySnooze->setEnabled(checked);
        m_editHotkeySkip->setEnabled(checked);
        m_editHotkeyDnd->setEnabled(checked);
        m_btnResetHotkeys->setEnabled(checked);
        m_lblHotkeyStatus->setText("Backend: " + m_hotkeyManager->backendStatus() + (checked ? " (Active)" : " (Disabled)"));
        saveLambda();
    });

    connect(m_editHotkeyPause, &QLineEdit::editingFinished, saveLambda);
    connect(m_editHotkeySnooze, &QLineEdit::editingFinished, saveLambda);
    connect(m_editHotkeySkip, &QLineEdit::editingFinished, saveLambda);
    connect(m_editHotkeyDnd, &QLineEdit::editingFinished, saveLambda);

    connect(m_btnResetHotkeys, &QPushButton::clicked, [this, saveLambda]() {
        m_editHotkeyPause->setText("Ctrl+Alt+P");
        m_editHotkeySnooze->setText("Ctrl+Alt+S");
        m_editHotkeySkip->setText("Ctrl+Alt+K");
        m_editHotkeyDnd->setText("Ctrl+Alt+D");
        saveLambda();
    });

    connect(m_chkIdleDetection, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboIdleVal->setEnabled(checked);
        m_comboIdleUnit->setEnabled(checked);
        saveLambda();
    });

    connect(m_chkConcurrentPresets, &QCheckBox::toggled, [this, saveLambda](bool checked) {
        m_comboSecondaryPreset->setEnabled(checked);
        if (m_btnToggleConcurrentMode) {
            m_btnToggleConcurrentMode->setText(checked ? "Dual Schedule: ON" : "Dual Schedule: Off");
            m_btnToggleConcurrentMode->setStyleSheet(checked ? "background-color: #0284c7; color: #ffffff; border: 1px solid #38bdf8;" : "");
        }
        if (!checked && m_lblSecondaryTimerStatus) {
            m_lblSecondaryTimerStatus->setVisible(false);
        }
        saveLambda();
        updateActivePresetHighlight();
    });

    connect(m_comboSecondaryPreset, QOverload<int>::of(&QComboBox::activated), [this, saveLambda](int) {
        saveLambda();
        updateActivePresetHighlight();
    });

    connect(m_btnSaveCurrentAsProfile, &QPushButton::clicked, this, &MainWindow::openCreateCustomPresetDialog);
    connect(m_chkCloseToTray, &QCheckBox::toggled, saveLambda);
    connect(m_chkAutostart, &QCheckBox::toggled, saveLambda);

    scrollArea->setWidget(tabContent);
    return scrollArea;
}

void MainWindow::loadSettingsToUi() {
    m_isUpdatingUi = true;
    secondsToUi(m_settings->workDurationSeconds(), m_comboWorkVal, m_comboWorkUnit);
    secondsToUi(m_settings->breakDurationSeconds(), m_comboBreakVal, m_comboBreakUnit);

    m_chkConcurrentPresets->setChecked(m_settings->concurrentPresetsEnabled());
    int secIdx = m_comboSecondaryPreset->findData(m_settings->secondaryPresetName());
    if (secIdx >= 0) m_comboSecondaryPreset->setCurrentIndex(secIdx);
    m_comboSecondaryPreset->setEnabled(m_settings->concurrentPresetsEnabled());

    m_chkBreakWindow->setChecked(m_settings->breakWindowEnabled());
    int styleIdx = m_comboBreakStyle->findData(m_settings->breakWindowStyle());
    if (styleIdx >= 0) m_comboBreakStyle->setCurrentIndex(styleIdx);
    m_comboBreakStyle->setEnabled(m_settings->breakWindowEnabled());

    // Screen Flash
    m_chkScreenFlash->setChecked(m_settings->screenFlashEnabled());
    int flashIdx = m_comboScreenFlashStyle->findData(m_settings->screenFlashStyle());
    if (flashIdx >= 0) m_comboScreenFlashStyle->setCurrentIndex(flashIdx);
    m_comboScreenFlashStyle->setEnabled(m_settings->screenFlashEnabled());
    m_btnPreviewFlash->setEnabled(m_settings->screenFlashEnabled());

    // Hotkeys
    m_chkGlobalHotkeys->setChecked(m_settings->globalHotkeysEnabled());
    m_editHotkeyPause->setText(m_settings->hotkeyPauseResume());
    m_editHotkeySnooze->setText(m_settings->hotkeySnooze());
    m_editHotkeySkip->setText(m_settings->hotkeySkip());
    m_editHotkeyDnd->setText(m_settings->hotkeyDnd());
    bool hkOn = m_settings->globalHotkeysEnabled();
    m_editHotkeyPause->setEnabled(hkOn);
    m_editHotkeySnooze->setEnabled(hkOn);
    m_editHotkeySkip->setEnabled(hkOn);
    m_editHotkeyDnd->setEnabled(hkOn);
    m_btnResetHotkeys->setEnabled(hkOn);
    m_lblHotkeyStatus->setText("Backend: " + m_hotkeyManager->backendStatus() + (hkOn ? " (Active)" : " (Disabled)"));

    m_chkPreBreakWarning->setChecked(m_settings->preBreakWarningEnabled());
    int warnIdx = m_comboPreBreakWarningSecs->findData(m_settings->preBreakWarningSeconds());
    if (warnIdx >= 0) m_comboPreBreakWarningSecs->setCurrentIndex(warnIdx);
    m_comboPreBreakWarningSecs->setEnabled(m_settings->preBreakWarningEnabled());

    m_chkPostponeBreak->setChecked(m_settings->postponeEnabled());
    int postIdx = m_comboPostponeMins->findData(m_settings->defaultPostponeSeconds());
    if (postIdx >= 0) m_comboPostponeMins->setCurrentIndex(postIdx);
    m_comboPostponeMins->setEnabled(m_settings->postponeEnabled());

    m_chkForceDisableSkip->setChecked(m_settings->forceDisableSkip());
    m_chkSuppressOnFullscreen->setChecked(m_settings->suppressOnFullscreen());

    m_chkAudioEnabled->setChecked(m_settings->audioEnabled());
    m_sliderVolume->setValue(m_settings->volume());
    m_lblVolumeVal->setText(QString("%1%").arg(m_settings->volume()));

    int packIdx = m_comboSoundPack->findData(m_settings->soundPack());
    if (packIdx >= 0) m_comboSoundPack->setCurrentIndex(packIdx);
    bool isCustom = (m_settings->soundPack() == "custom");
    m_customSoundWidget->setVisible(isCustom);
    m_editCustomWorkPath->setText(m_settings->customWorkSoundPath());
    m_editCustomBreakPath->setText(m_settings->customBreakSoundPath());

    bool audioOn = m_settings->audioEnabled();
    m_sliderVolume->setEnabled(audioOn);
    m_comboSoundPack->setEnabled(audioOn);
    m_customSoundWidget->setEnabled(audioOn);
    m_btnTestWorkSound->setEnabled(audioOn);
    m_btnTestBreakSound->setEnabled(audioOn);

    m_chkNotificationsEnabled->setChecked(m_settings->notificationsEnabled());
    m_chkCloseToTray->setChecked(m_settings->closeToTray());
    m_chkAutostart->setChecked(m_settings->autostart());
    m_chkIdleDetection->setChecked(m_settings->idleDetectionEnabled());
    secondsToUi(m_settings->idleThresholdSeconds(), m_comboIdleVal, m_comboIdleUnit);
    m_comboIdleVal->setEnabled(m_settings->idleDetectionEnabled());
    m_comboIdleUnit->setEnabled(m_settings->idleDetectionEnabled());

#if defined(Q_OS_LINUX)
    m_lblIdleBackendStatus->setText("Active Engine: " + LinuxIdleDetector::activeBackendName());
#elif defined(Q_OS_WIN)
    m_lblIdleBackendStatus->setText("Active Engine: Win32 GetLastInputInfo (Windows Native)");
#else
    m_lblIdleBackendStatus->setText("Active Engine: Standard System Monitor");
#endif

    m_isUpdatingUi = false;
    updateActivePresetHighlight();
}

void MainWindow::saveSettingsFromUi() {
    int workSecs = durationToSeconds(m_comboWorkVal, m_comboWorkUnit);
    int breakSecs = durationToSeconds(m_comboBreakVal, m_comboBreakUnit);

    m_settings->setWorkDurationSeconds(workSecs);
    m_settings->setBreakDurationSeconds(breakSecs);

    m_settings->setConcurrentPresetsEnabled(m_chkConcurrentPresets->isChecked());
    QString secPreset = m_comboSecondaryPreset->currentData().toString();
    m_settings->setSecondaryPresetName(secPreset);
    if (secPreset == "50-10") {
        m_settings->setSecondaryWorkDurationSeconds(50 * 60);
        m_settings->setSecondaryBreakDurationSeconds(600);
    } else if (secPreset == "25-5") {
        m_settings->setSecondaryWorkDurationSeconds(25 * 60);
        m_settings->setSecondaryBreakDurationSeconds(300);
    } else if (secPreset == "20-20-20") {
        m_settings->setSecondaryWorkDurationSeconds(20 * 60);
        m_settings->setSecondaryBreakDurationSeconds(20);
    }

    m_settings->setBreakWindowEnabled(m_chkBreakWindow->isChecked());
    m_settings->setBreakWindowStyle(m_comboBreakStyle->currentData().toString());
    m_settings->setScreenFlashEnabled(m_chkScreenFlash->isChecked());
    m_settings->setScreenFlashStyle(m_comboScreenFlashStyle->currentData().toString());

    m_settings->setGlobalHotkeysEnabled(m_chkGlobalHotkeys->isChecked());
    m_settings->setHotkeyPauseResume(m_editHotkeyPause->text().trimmed());
    m_settings->setHotkeySnooze(m_editHotkeySnooze->text().trimmed());
    m_settings->setHotkeySkip(m_editHotkeySkip->text().trimmed());
    m_settings->setHotkeyDnd(m_editHotkeyDnd->text().trimmed());
    if (m_hotkeyManager) {
        m_hotkeyManager->registerAllHotkeys();
    }

    m_settings->setPreBreakWarningEnabled(m_chkPreBreakWarning->isChecked());
    m_settings->setPreBreakWarningSeconds(m_comboPreBreakWarningSecs->currentData().toInt());
    m_settings->setPostponeEnabled(m_chkPostponeBreak->isChecked());
    m_settings->setDefaultPostponeSeconds(m_comboPostponeMins->currentData().toInt());
    m_settings->setForceDisableSkip(m_chkForceDisableSkip->isChecked());
    m_settings->setSuppressOnFullscreen(m_chkSuppressOnFullscreen->isChecked());
    m_settings->setAudioEnabled(m_chkAudioEnabled->isChecked());
    m_settings->setVolume(m_sliderVolume->value());
    m_settings->setSoundPack(m_comboSoundPack->currentData().toString());
    m_settings->setCustomWorkSoundPath(m_editCustomWorkPath->text());
    m_settings->setCustomBreakSoundPath(m_editCustomBreakPath->text());
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

    if (m_settings->concurrentPresetsEnabled()) {
        if (m_btnToggleConcurrentMode) {
            m_btnToggleConcurrentMode->setText("Dual Schedule: ON");
            m_btnToggleConcurrentMode->setStyleSheet("background-color: #0284c7; color: #ffffff; border: 1px solid #38bdf8;");
        }
        QString sec = m_settings->secondaryPresetName();
        if (sec.contains("50-10")) {
            m_btnPreset50->setChecked(true);
        } else if (sec.contains("25-5") || sec.contains("Pomo")) {
            m_btnPreset25->setChecked(true);
        } else if (sec.contains("20-20-20")) {
            m_btnPreset20->setChecked(true);
        }
    } else {
        if (m_btnToggleConcurrentMode) {
            m_btnToggleConcurrentMode->setText("Dual Schedule: Off");
            m_btnToggleConcurrentMode->setStyleSheet("");
        }
    }
}

void MainWindow::updateStatsDisplay() {
    if (m_lblStatCompleted) m_lblStatCompleted->setText(QString::number(m_settings->breaksCompletedToday()));
    if (m_lblStatPostponed) m_lblStatPostponed->setText(QString::number(m_settings->breaksPostponedToday()));
    if (m_lblStatSkipped) m_lblStatSkipped->setText(QString::number(m_settings->breaksSkippedToday()));
    double restMins = static_cast<double>(m_settings->eyeRestSecondsToday()) / 60.0;
    if (m_lblStatRestTime) m_lblStatRestTime->setText(QString("%1m").arg(restMins, 0, 'f', 1));

    if (m_weeklyAnalyticsWidget) {
        m_weeklyAnalyticsWidget->refresh();
    }
}

void MainWindow::showSettingsTab() {
    show();
    raise();
    activateWindow();
    m_tabWidget->setCurrentIndex(4);
}

void MainWindow::updateUiForState(TimerEngine::State newState, TimerEngine::State oldState) {
    if (m_timerEngine->isDndActive()) {
        handleDndStateChanged(true, m_timerEngine->dndSecondsRemaining());
        return;
    }

    switch (newState) {
    case TimerEngine::State::Working:
        m_statusBadgeLabel->setText("WORKING SESSION");
        m_statusBadgeLabel->setStyleSheet("background-color: #0284c7; color: #ffffff;");
        m_btnPlayPause->setText("Pause");
        m_btnSkipBreak->setEnabled(false);
        m_btnPostponeBreak->setEnabled(false);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;

    case TimerEngine::State::Breaking: {
        bool isSecondary = (m_timerEngine->activeBreakType() == TimerEngine::ActiveBreakType::Secondary);
        if (isSecondary) {
            m_statusBadgeLabel->setText("LONG REST BREAK! ☕");
        } else {
            m_statusBadgeLabel->setText("LOOK 20 FEET AWAY! 👁️");
        }
        m_statusBadgeLabel->setStyleSheet("background-color: #d97706; color: #ffffff;");
        m_btnPlayPause->setText("Pause");

        bool canSkip = !m_settings->forceDisableSkip();
        bool canPostpone = canSkip && m_settings->postponeEnabled();
        m_btnSkipBreak->setEnabled(canSkip);
        m_btnPostponeBreak->setEnabled(canPostpone);
        if (!canSkip) {
            m_btnSkipBreak->setToolTip("Skip break is disabled in settings (Strict Enforcement mode).");
            m_btnPostponeBreak->setToolTip("Snooze is disabled in strict enforcement mode.");
        } else {
            m_btnSkipBreak->setToolTip("Skip the current break session.");
            m_btnPostponeBreak->setToolTip("Snooze current break by 2 minutes.");
        }

        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();

        if (m_settings->breakWindowEnabled()) {
            QString style = m_settings->breakWindowStyle();

            if (style == "popup") {
                QScreen* screen = QGuiApplication::primaryScreen();
                BreakOverlayWidget* popup = new BreakOverlayWidget(BreakOverlayWidget::DisplayMode::CenteredPopup);
                popup->setSkipDisabled(!canSkip);
                popup->setPostponeVisible(canPostpone);
                popup->setPostponeSeconds(m_settings->defaultPostponeSeconds());
                popup->setBreakContext(isSecondary);
                m_breakOverlays.append(popup);
                connect(popup, &BreakOverlayWidget::skipRequested, m_timerEngine, &TimerEngine::skipBreak);
                connect(popup, &BreakOverlayWidget::postponeRequested, m_timerEngine, &TimerEngine::postponeBreak);

                QRect screenGeom = screen ? screen->geometry() : QRect(0, 0, 1920, 1080);
                int w = 520;
                int h = 370;
                int x = screenGeom.x() + (screenGeom.width() - w) / 2;
                int y = screenGeom.y() + (screenGeom.height() - h) / 2;
                popup->setGeometry(x, y, w, h);
                popup->show();
                popup->raise();
                popup->activateWindow();
            } else if (style == "border") {
                // Ambient border glow: transparent center, non-blocking perimeter halo
                const QList<QScreen*> screens = QGuiApplication::screens();
                for (QScreen* screen : screens) {
                    BreakOverlayWidget* borderOverlay = new BreakOverlayWidget(BreakOverlayWidget::DisplayMode::BorderGlow);
                    m_breakOverlays.append(borderOverlay);
                    borderOverlay->setGeometry(screen->geometry());
                    borderOverlay->show();
                }
            } else {
                // Fullscreen shield
                const QList<QScreen*> screens = QGuiApplication::screens();
                for (QScreen* screen : screens) {
                    BreakOverlayWidget* overlay = new BreakOverlayWidget(BreakOverlayWidget::DisplayMode::FullScreen);
                    overlay->setSkipDisabled(!canSkip);
                    overlay->setPostponeVisible(canPostpone);
                    overlay->setPostponeSeconds(m_settings->defaultPostponeSeconds());
                    overlay->setBreakContext(isSecondary);
                    m_breakOverlays.append(overlay);
                    connect(overlay, &BreakOverlayWidget::skipRequested, m_timerEngine, &TimerEngine::skipBreak);
                    connect(overlay, &BreakOverlayWidget::postponeRequested, m_timerEngine, &TimerEngine::postponeBreak);
                    overlay->setGeometry(screen->geometry());
                    overlay->show();
                    overlay->raise();
                    overlay->activateWindow();
                }
            }
        }
        break;
    }

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
        m_btnPostponeBreak->setEnabled(m_btnSkipBreak->isEnabled() && m_settings->postponeEnabled());
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;

    case TimerEngine::State::Idle:
    default:
        m_statusBadgeLabel->setText("READY TO WORK");
        m_statusBadgeLabel->setStyleSheet("background-color: #334155; color: #94a3b8;");
        m_btnPlayPause->setText("Start");
        m_btnSkipBreak->setEnabled(false);
        m_btnPostponeBreak->setEnabled(false);
        qDeleteAll(m_breakOverlays);
        m_breakOverlays.clear();
        break;
    }
}

void MainWindow::handlePreBreakWarning(int secondsUntilBreak, TimerEngine::ActiveBreakType breakType) {
    QString breakName = (breakType == TimerEngine::ActiveBreakType::Secondary) ? "MACRO REST BREAK" : "EYE REST BREAK";
    m_statusBadgeLabel->setText(QString("%1 IN %2s ⏳").arg(breakName).arg(secondsUntilBreak));
    m_statusBadgeLabel->setStyleSheet("background-color: #f59e0b; color: #0f172a; font-weight: 800;");
    if (m_btnPostponeBreak && m_settings->postponeEnabled() && !m_settings->forceDisableSkip()) {
        m_btnPostponeBreak->setEnabled(true);
        int mins = m_settings->defaultPostponeSeconds() / 60;
        m_btnPostponeBreak->setToolTip(QString("Snooze upcoming break by %1 minutes.").arg(mins > 0 ? mins : 2));
    }
}

void MainWindow::handleDndStateChanged(bool active, int secondsRemaining) {
    if (active) {
        m_btnDnd->setVisible(false);
        m_btnEndDnd->setVisible(true);
        if (secondsRemaining > 0) {
            m_statusBadgeLabel->setText(QString("DO NOT DISTURB (%1) 🔕").arg(m_timerEngine->formattedDndTimeRemaining()));
        } else {
            m_statusBadgeLabel->setText("DO NOT DISTURB (INDEFINITE) 🔕");
        }
        m_statusBadgeLabel->setStyleSheet("background-color: #8b5cf6; color: #ffffff; font-weight: 800;");
        m_btnPlayPause->setEnabled(false);
        m_btnSkipBreak->setEnabled(false);
        m_btnPostponeBreak->setEnabled(false);
    } else {
        m_btnDnd->setVisible(true);
        m_btnEndDnd->setVisible(false);
        m_btnPlayPause->setEnabled(true);
        updateUiForState(m_timerEngine->state(), TimerEngine::State::Idle);
    }
}

void MainWindow::updateCountdown(int secondsRemaining, int totalSeconds) {
    if (m_timerEngine->isDndActive()) {
        int dndRem = m_timerEngine->dndSecondsRemaining();
        if (dndRem > 0) {
            m_statusBadgeLabel->setText(QString("DO NOT DISTURB (%1) 🔕").arg(m_timerEngine->formattedDndTimeRemaining()));
        }
    }

    QString timeText;
    if (secondsRemaining >= 3600) {
        int hrs = secondsRemaining / 3600;
        int mins = (secondsRemaining % 3600) / 60;
        int secs = secondsRemaining % 60;
        timeText = QString("%1:%2:%3")
                       .arg(hrs, 2, 10, QChar('0'))
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));
    } else {
        int mins = secondsRemaining / 60;
        int secs = secondsRemaining % 60;
        timeText = QString("%1:%2")
                       .arg(mins, 2, 10, QChar('0'))
                       .arg(secs, 2, 10, QChar('0'));
    }
    m_countdownLabel->setText(timeText);

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
    QPalette appPal = qApp->palette();
    appPal.setColor(QPalette::ToolTipBase, QColor("#0f172a"));
    appPal.setColor(QPalette::ToolTipText, QColor("#f8fafc"));
    qApp->setPalette(appPal);

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
            padding: 8px 10px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: 600;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            background-color: #1e293b;
            color: #38bdf8;
            border-bottom: 2px solid #38bdf8;
        }
        QTabBar::tab:hover:!selected {
            background-color: #162032;
            color: #e2e8f0;
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
        QToolTip, QTipLabel {
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
        QSpinBox, QComboBox, QLineEdit {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 4px 8px;
            color: #f8fafc;
        }
        QSpinBox:hover, QComboBox:hover {
            border-color: #475569;
        }
        QSpinBox:focus, QComboBox:focus, QLineEdit:focus {
            border: 1px solid #38bdf8;
        }
        QFrame#comboContainer,
        QComboBoxPrivateContainer {
            background-color: #0f172a;
            border: 1px solid #38bdf8;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView,
        QComboBox QListView {
            background-color: transparent;
            color: #f8fafc;
            border: none;
            padding: 4px;
            outline: none;
        }
        QComboBox QAbstractItemView::item,
        QComboBox QListView::item {
            background-color: transparent;
            color: #f8fafc;
            padding: 6px 12px;
            border-radius: 4px;
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
            background-color: transparent;
            width: 8px;
            margin: 10px 4px 10px 0px;
        }
        QScrollBar::handle:vertical {
            background-color: #334155;
            min-height: 28px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #38bdf8;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
            width: 0px;
        }
    )";
    qApp->setStyleSheet(qss);
}
