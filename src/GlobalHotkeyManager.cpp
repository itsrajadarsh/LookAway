#include "GlobalHotkeyManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QWidget>
#include <QShortcut>
#include <QSocketNotifier>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>

static bool parseKeySequence(const QKeySequence& seq, UINT& modifiers, UINT& vk) {
    if (seq.isEmpty()) return false;
    modifiers = 0;
    vk = 0;

    int keyCombo = seq[0].toCombined();
    int mods = keyCombo & Qt::KeyboardModifierMask;
    int key = keyCombo & ~Qt::KeyboardModifierMask;

    if (mods & Qt::ControlModifier) modifiers |= MOD_CONTROL;
    if (mods & Qt::AltModifier)     modifiers |= MOD_ALT;
    if (mods & Qt::ShiftModifier)   modifiers |= MOD_SHIFT;
    if (mods & Qt::MetaModifier)    modifiers |= MOD_WIN;

    // Convert Qt key to Win32 Virtual-Key code
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        vk = 'A' + (key - Qt::Key_A);
    } else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        vk = '0' + (key - Qt::Key_0);
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        vk = VK_F1 + (key - Qt::Key_F1);
    } else if (key == Qt::Key_Space) {
        vk = VK_SPACE;
    } else if (key == Qt::Key_Escape) {
        vk = VK_ESCAPE;
    } else if (key == Qt::Key_Tab) {
        vk = VK_TAB;
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        vk = VK_RETURN;
    } else {
        return false;
    }

    return true;
}
#endif

#if defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/keysym.h>

// Undefine conflicting X11 macros
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef CursorShape
#undef CursorShape
#endif

static bool parseX11Sequence(Display* dpy, const QString& keySequenceStr, unsigned int& modifiers, KeyCode& keycode) {
    if (!dpy || keySequenceStr.isEmpty()) return false;
    QKeySequence seq(keySequenceStr, QKeySequence::PortableText);
    if (seq.isEmpty()) return false;

    int keyCombo = seq[0].toCombined();
    int mods = keyCombo & Qt::KeyboardModifierMask;
    int key = keyCombo & ~Qt::KeyboardModifierMask;

    modifiers = 0;
    if (mods & Qt::ControlModifier) modifiers |= ControlMask;
    if (mods & Qt::AltModifier)     modifiers |= Mod1Mask;
    if (mods & Qt::ShiftModifier)   modifiers |= ShiftMask;
    if (mods & Qt::MetaModifier)    modifiers |= Mod4Mask;

    QString keyStr = QKeySequence(key).toString().toLower();
    KeySym sym = XStringToKeysym(keyStr.toUtf8().constData());
    if (sym == NoSymbol) {
        if (key == Qt::Key_Space) sym = XK_space;
        else if (key == Qt::Key_Escape) sym = XK_Escape;
        else if (key == Qt::Key_Return || key == Qt::Key_Enter) sym = XK_Return;
        else if (key == Qt::Key_Tab) sym = XK_Tab;
        else return false;
    }

    keycode = XKeysymToKeycode(dpy, sym);
    return (keycode != 0);
}
#endif

#if defined(Q_OS_LINUX)
static int x11ErrorHandler(Display* /*dpy*/, XErrorEvent* /*ev*/) {
    return 0; // Ignore X11 errors like BadAccess
}
#endif

GlobalHotkeyManager::GlobalHotkeyManager(SettingsManager* settings, QObject* parent)
    : QObject(parent),
      m_settings(settings),
      m_filterInstalled(false)
#if defined(Q_OS_LINUX)
    , m_x11Display(nullptr)
    , m_x11Notifier(nullptr)
    , m_x11CodePause(0)
    , m_x11CodeSnooze(0)
    , m_x11CodeSkip(0)
    , m_x11CodeDnd(0)
#endif
{
#ifdef Q_OS_WIN
    QCoreApplication::instance()->installNativeEventFilter(this);
    m_filterInstalled = true;
#elif defined(Q_OS_LINUX)
    XSetErrorHandler(x11ErrorHandler);
    m_x11Display = XOpenDisplay(NULL);
    if (m_x11Display) {
        int x11Fd = ConnectionNumber(m_x11Display);
        m_x11Notifier = new QSocketNotifier(x11Fd, QSocketNotifier::Read, this);
        connect(m_x11Notifier, &QSocketNotifier::activated, this, [this]() {
            while (m_x11Display && XPending(m_x11Display) > 0) {
                XEvent ev;
                XNextEvent(m_x11Display, &ev);
                if (ev.type == 2 /* KeyPress */) {
                    unsigned int kc = ev.xkey.keycode;
                    if (kc == m_x11CodePause && m_x11CodePause != 0) {
                        emit togglePauseResumeRequested();
                    } else if (kc == m_x11CodeSnooze && m_x11CodeSnooze != 0) {
                        emit snoozeBreakRequested();
                    } else if (kc == m_x11CodeSkip && m_x11CodeSkip != 0) {
                        emit skipBreakRequested();
                    } else if (kc == m_x11CodeDnd && m_x11CodeDnd != 0) {
                        emit toggleDndRequested();
                    }
                }
            }
        });
        QCoreApplication::instance()->installNativeEventFilter(this);
        m_filterInstalled = true;
    }
#endif

    connect(m_settings, &SettingsManager::settingsChanged, this, &GlobalHotkeyManager::handleSettingsChanged);

    registerAllHotkeys();
}

GlobalHotkeyManager::~GlobalHotkeyManager() {
    unregisterAllHotkeys();
    if (m_filterInstalled && QCoreApplication::instance()) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
#if defined(Q_OS_LINUX)
    if (m_x11Notifier) {
        m_x11Notifier->setEnabled(false);
        delete m_x11Notifier;
        m_x11Notifier = nullptr;
    }
    if (m_x11Display) {
        XCloseDisplay(m_x11Display);
        m_x11Display = nullptr;
    }
#endif
}

void GlobalHotkeyManager::registerAllHotkeys() {
    unregisterAllHotkeys();

    // 1. Setup in-app application-wide QShortcuts on parent window (works across Linux Wayland/X11 and Windows)
    QWidget* parentWidget = qobject_cast<QWidget*>(parent());
    if (parentWidget && m_settings->globalHotkeysEnabled()) {
        auto addAppShortcut = [this, parentWidget](const QString& seqStr, auto signalMethod) {
            if (seqStr.trimmed().isEmpty()) return;
            QShortcut* sc = new QShortcut(QKeySequence(seqStr), parentWidget);
            sc->setContext(Qt::ApplicationShortcut);
            connect(sc, &QShortcut::activated, this, signalMethod);
            m_appShortcuts.append(sc);
        };

        addAppShortcut(m_settings->hotkeyPauseResume(), &GlobalHotkeyManager::togglePauseResumeRequested);
        addAppShortcut(m_settings->hotkeySnooze(), &GlobalHotkeyManager::snoozeBreakRequested);
        addAppShortcut(m_settings->hotkeySkip(), &GlobalHotkeyManager::skipBreakRequested);
        addAppShortcut(m_settings->hotkeyDnd(), &GlobalHotkeyManager::toggleDndRequested);
    }

    if (!m_settings->globalHotkeysEnabled()) {
        return;
    }

    // 2. Global OS registration
    platformRegister(Hotkey_PauseResume, m_settings->hotkeyPauseResume());
    platformRegister(Hotkey_Snooze, m_settings->hotkeySnooze());
    platformRegister(Hotkey_Skip, m_settings->hotkeySkip());
    platformRegister(Hotkey_Dnd, m_settings->hotkeyDnd());
}

void GlobalHotkeyManager::unregisterAllHotkeys() {
    qDeleteAll(m_appShortcuts);
    m_appShortcuts.clear();

    platformUnregister(Hotkey_PauseResume);
    platformUnregister(Hotkey_Snooze);
    platformUnregister(Hotkey_Skip);
    platformUnregister(Hotkey_Dnd);
}

void GlobalHotkeyManager::platformRegister(int id, const QString& keySequenceStr) {
#ifdef Q_OS_WIN
    QKeySequence seq(keySequenceStr, QKeySequence::PortableText);
    UINT mods = 0;
    UINT vk = 0;
    if (parseKeySequence(seq, mods, vk)) {
        RegisterHotKey(NULL, id, mods | 0x4000 /* MOD_NOREPEAT */, vk);
    }
#elif defined(Q_OS_LINUX)
    if (m_x11Display) {
        unsigned int mods = 0;
        KeyCode kc = 0;
        if (parseX11Sequence(m_x11Display, keySequenceStr, mods, kc)) {
            Window root = DefaultRootWindow(m_x11Display);
            XGrabKey(m_x11Display, kc, mods, root, 1, GrabModeAsync, GrabModeAsync);
            XGrabKey(m_x11Display, kc, mods | Mod2Mask /* NumLock */, root, 1, GrabModeAsync, GrabModeAsync);
            XGrabKey(m_x11Display, kc, mods | LockMask /* CapsLock */, root, 1, GrabModeAsync, GrabModeAsync);
            XGrabKey(m_x11Display, kc, mods | Mod2Mask | LockMask, root, 1, GrabModeAsync, GrabModeAsync);
            XFlush(m_x11Display);

            if (id == Hotkey_PauseResume) m_x11CodePause = kc;
            else if (id == Hotkey_Snooze) m_x11CodeSnooze = kc;
            else if (id == Hotkey_Skip) m_x11CodeSkip = kc;
            else if (id == Hotkey_Dnd) m_x11CodeDnd = kc;
        }
    }
#else
    Q_UNUSED(id);
    Q_UNUSED(keySequenceStr);
#endif
}

void GlobalHotkeyManager::platformUnregister(int id) {
#ifdef Q_OS_WIN
    UnregisterHotKey(NULL, id);
#elif defined(Q_OS_LINUX)
    if (m_x11Display) {
        Window root = DefaultRootWindow(m_x11Display);
        KeyCode kc = 0;
        if (id == Hotkey_PauseResume) { kc = m_x11CodePause; m_x11CodePause = 0; }
        else if (id == Hotkey_Snooze) { kc = m_x11CodeSnooze; m_x11CodeSnooze = 0; }
        else if (id == Hotkey_Skip) { kc = m_x11CodeSkip; m_x11CodeSkip = 0; }
        else if (id == Hotkey_Dnd) { kc = m_x11CodeDnd; m_x11CodeDnd = 0; }

        if (kc != 0) {
            XUngrabKey(m_x11Display, kc, AnyModifier, root);
            XFlush(m_x11Display);
        }
    }
#else
    Q_UNUSED(id);
#endif
}

QString GlobalHotkeyManager::backendStatus() const {
#ifdef Q_OS_WIN
    return m_settings->globalHotkeysEnabled()
        ? "Windows Win32 Global Hotkeys (Active)"
        : "Disabled in Preferences";
#elif defined(Q_OS_LINUX)
    if (m_settings->globalHotkeysEnabled()) {
        if (QGuiApplication::platformName() == "wayland") {
            return "Linux Wayland: In-App & IPC Active (use 'lookaway --toggle')";
        }
        return "Linux X11 Global Hotkeys (Active)";
    }
    return "Disabled in Preferences";
#else
    return "System Hotkeys Active";
#endif
}

void GlobalHotkeyManager::handleSettingsChanged() {
    registerAllHotkeys();
}

bool GlobalHotkeyManager::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(result);
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_HOTKEY) {
            int id = static_cast<int>(msg->wParam);
            if (id == Hotkey_PauseResume) {
                emit togglePauseResumeRequested();
                return true;
            } else if (id == Hotkey_Snooze) {
                emit snoozeBreakRequested();
                return true;
            } else if (id == Hotkey_Skip) {
                emit skipBreakRequested();
                return true;
            } else if (id == Hotkey_Dnd) {
                emit toggleDndRequested();
                return true;
            }
        }
    }
#elif defined(Q_OS_LINUX)
    if (eventType == "xcb_generic_event_t" && message) {
        struct xcb_generic_event_t {
            uint8_t   response_type;
            uint8_t   pad0;
            uint16_t  sequence;
            uint32_t  pad[7];
            uint32_t  full_sequence;
        };
        struct xcb_key_press_event_t {
            uint8_t         response_type;
            uint8_t         detail; // keycode
            uint16_t        sequence;
            uint32_t        time;
            uint32_t        root;
            uint32_t        event;
            uint32_t        child;
            int16_t         root_x;
            int16_t         root_y;
            int16_t         event_x;
            int16_t         event_y;
            uint16_t        state;
            uint8_t         same_screen;
            uint8_t         pad0;
        };

        auto* event = static_cast<xcb_generic_event_t*>(message);
        uint8_t resp = event->response_type & ~0x80;
        if (resp == 2 /* XCB_KEY_PRESS */) {
            auto* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(event);
            uint8_t kc = keyEvent->detail;
            if (kc == m_x11CodePause && m_x11CodePause != 0) {
                emit togglePauseResumeRequested();
                return true;
            } else if (kc == m_x11CodeSnooze && m_x11CodeSnooze != 0) {
                emit snoozeBreakRequested();
                return true;
            } else if (kc == m_x11CodeSkip && m_x11CodeSkip != 0) {
                emit skipBreakRequested();
                return true;
            } else if (kc == m_x11CodeDnd && m_x11CodeDnd != 0) {
                emit toggleDndRequested();
                return true;
            }
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif
    return false;
}
