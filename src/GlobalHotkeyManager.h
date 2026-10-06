#ifndef GLOBALHOTKEYMANAGER_H
#define GLOBALHOTKEYMANAGER_H

#include <QObject>
#include <QString>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>
#include <QList>

class QShortcut;
class QSocketNotifier;
class SettingsManager;

struct _XDisplay;
typedef struct _XDisplay Display;

class GlobalHotkeyManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    enum HotkeyId {
        Hotkey_PauseResume = 1001,
        Hotkey_Snooze      = 1002,
        Hotkey_Skip        = 1003,
        Hotkey_Dnd         = 1004
    };

    explicit GlobalHotkeyManager(SettingsManager* settings, QObject* parent = nullptr);
    ~GlobalHotkeyManager() override;

    void registerAllHotkeys();
    void unregisterAllHotkeys();
    QString backendStatus() const;

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

signals:
    void togglePauseResumeRequested();
    void snoozeBreakRequested();
    void skipBreakRequested();
    void toggleDndRequested();

private slots:
    void handleSettingsChanged();

private:
    void platformRegister(int id, const QString& keySequenceStr);
    void platformUnregister(int id);

    SettingsManager* m_settings;
    bool m_filterInstalled;
    QList<QShortcut*> m_appShortcuts;

#if defined(Q_OS_LINUX)
    Display* m_x11Display;
    QSocketNotifier* m_x11Notifier;
    unsigned int m_x11CodePause;
    unsigned int m_x11CodeSnooze;
    unsigned int m_x11CodeSkip;
    unsigned int m_x11CodeDnd;
#endif
};

#endif // GLOBALHOTKEYMANAGER_H

