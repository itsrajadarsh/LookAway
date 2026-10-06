#ifndef SCREENFLASHWIDGET_H
#define SCREENFLASHWIDGET_H

#include <QWidget>
#include <QColor>
#include <QPropertyAnimation>

class ScreenFlashWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal flashOpacity READ flashOpacity WRITE setFlashOpacity)

public:
    explicit ScreenFlashWidget(QWidget* parent = nullptr);

    qreal flashOpacity() const;
    void setFlashOpacity(qreal opacity);

    void startFlash(const QString& style = "cyan");

    static void flashAllScreens(const QString& style = "cyan");

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qreal m_flashOpacity;
    QColor m_color;
};

#endif // SCREENFLASHWIDGET_H
