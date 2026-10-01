#ifndef BREAKOVERLAYWIDGET_H
#define BREAKOVERLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QKeyEvent>
#include <QFrame>
#include <QTimer>

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

signals:
    void skipRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUi();

    DisplayMode m_mode;
    bool m_skipDisabled;

    // Strict enforcement watchdog timer
    QTimer* m_enforceTopTimer = nullptr;

    // Ambient border glow breathing animation
    QTimer* m_glowTimer = nullptr;
    float m_glowPhase = 0.0f;

    QFrame* m_bgFrame = nullptr;
    QLabel* m_lblTitle = nullptr;
    QLabel* m_lblSubtitle = nullptr;
    QLabel* m_lblCountdown = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_btnSkip = nullptr;
};

#endif // BREAKOVERLAYWIDGET_H
