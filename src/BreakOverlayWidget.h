#ifndef BREAKOVERLAYWIDGET_H
#define BREAKOVERLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QKeyEvent>
#include <QFrame>
#include <QTimer>
#include "ErgonomicTipCatalog.h"

class BreakOverlayWidget : public QWidget {
    Q_OBJECT

public:
    enum class DisplayMode {
        FullScreen,
        CenteredPopup,
        BorderGlow
    };
    Q_ENUM(DisplayMode)

    explicit BreakOverlayWidget(DisplayMode mode = DisplayMode::FullScreen, QWidget* parent = nullptr);

    void updateCountdown(int secondsRemaining, int totalSeconds);
    DisplayMode displayMode() const;
    void setSkipDisabled(bool disabled);
    void setPostponeVisible(bool visible);
    void setPostponeSeconds(int seconds);
    void setBreakContext(bool isMacroBreak);

signals:
    void skipRequested();
    void postponeRequested(int seconds);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private slots:
    void cycleNextTip();

private:
    void setupUi();
    void applyTip(const ErgonomicTip& tip);

    DisplayMode m_mode;
    bool m_skipDisabled;
    bool m_postponeVisible;
    int m_postponeSeconds;
    bool m_isMacroBreak;
    ErgonomicTip m_currentTip;

    // Strict enforcement watchdog timer
    QTimer* m_enforceTopTimer = nullptr;

    // Ambient border glow breathing animation
    QTimer* m_glowTimer = nullptr;
    float m_glowPhase = 0.0f;

    QFrame* m_bgFrame = nullptr;
    QLabel* m_lblCategoryBadge = nullptr;
    QLabel* m_lblTitle = nullptr;
    QLabel* m_lblSubtitle = nullptr;
    QLabel* m_lblBenefit = nullptr;
    QLabel* m_lblCountdown = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_btnNextTip = nullptr;
    QPushButton* m_btnPostpone = nullptr;
    QPushButton* m_btnSkip = nullptr;
};

#endif // BREAKOVERLAYWIDGET_H
