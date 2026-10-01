#ifndef BREAKOVERLAYWIDGET_H
#define BREAKOVERLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QMouseEvent>
#include <QKeyEvent>

class BreakOverlayWidget : public QWidget {
    Q_OBJECT

public:
    enum class DisplayMode {
        FullScreen,
        CenteredPopup
    };
    Q_ENUM(DisplayMode)

    explicit BreakOverlayWidget(DisplayMode mode = DisplayMode::FullScreen, QWidget* parent = nullptr);

    void updateCountdown(int secondsRemaining, int totalSeconds);
    DisplayMode displayMode() const;

signals:
    void skipRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void setupUi();

    DisplayMode m_mode;
    QPoint m_dragPosition;
    QLabel* m_lblTitle;
    QLabel* m_lblSubtitle;
    QLabel* m_lblCountdown;
    QProgressBar* m_progressBar;
    QPushButton* m_btnSkip;
};

#endif // BREAKOVERLAYWIDGET_H
