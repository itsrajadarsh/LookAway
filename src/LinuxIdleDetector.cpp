#include "LinuxIdleDetector.h"
#include <QGuiApplication>
#include <QDebug>

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#endif

qint64 LinuxIdleDetector::getIdletimeMs() {
#if defined(Q_OS_LINUX)
    if (!QDBusConnection::sessionBus().isConnected()) {
        return 0;
    }

    // 1. Try GNOME Mutter IdleMonitor (Default on GNOME Wayland / X11)
    {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.gnome.Mutter.IdleMonitor",
            "/org/gnome/Mutter/IdleMonitor/Core",
            "org.gnome.Mutter.IdleMonitor",
            "GetIdletime"
        );
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            bool ok = false;
            qulonglong ms = reply.arguments().at(0).toULongLong(&ok);
            if (ok) {
                return static_cast<qint64>(ms);
            }
        }
    }

    // 2. Try KDE Plasma / FreeDesktop ScreenSaver (Default on KDE Wayland / X11)
    {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.freedesktop.ScreenSaver",
            "/org/freedesktop/ScreenSaver",
            "org.freedesktop.ScreenSaver",
            "GetSessionIdleTime"
        );
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            bool ok = false;
            uint ms = reply.arguments().at(0).toUInt(&ok);
            if (ok) {
                return static_cast<qint64>(ms);
            }
        }
    }

    // 3. Try KDE KIdleTime
    {
        QDBusMessage msg = QDBusMessage::createMethodCall(
            "org.kde.KIdleTime",
            "/org/kde/KIdleTime",
            "org.kde.KIdleTime",
            "getIdleTime"
        );
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            bool ok = false;
            int ms = reply.arguments().at(0).toInt(&ok);
            if (ok && ms >= 0) {
                return static_cast<qint64>(ms);
            }
        }
    }
#endif
    return 0;
}

QString LinuxIdleDetector::activeBackendName() {
#if defined(Q_OS_WIN)
    return "Windows Input Hook (Win32)";
#elif defined(Q_OS_LINUX)
    if (!QDBusConnection::sessionBus().isConnected()) {
        return "Linux (D-Bus Not Connected)";
    }

    // Probe Mutter
    QDBusMessage msgMutter = QDBusMessage::createMethodCall(
        "org.gnome.Mutter.IdleMonitor",
        "/org/gnome/Mutter/IdleMonitor/Core",
        "org.gnome.Mutter.IdleMonitor",
        "GetIdletime"
    );
    QDBusMessage replyMutter = QDBusConnection::sessionBus().call(msgMutter);
    if (replyMutter.type() == QDBusMessage::ReplyMessage) {
        return "Wayland GNOME Mutter (D-Bus)";
    }

    // Probe FreeDesktop ScreenSaver
    QDBusMessage msgKde = QDBusMessage::createMethodCall(
        "org.freedesktop.ScreenSaver",
        "/org/freedesktop/ScreenSaver",
        "org.freedesktop.ScreenSaver",
        "GetSessionIdleTime"
    );
    QDBusMessage replyKde = QDBusConnection::sessionBus().call(msgKde);
    if (replyKde.type() == QDBusMessage::ReplyMessage) {
        return "KDE Plasma / FreeDesktop (D-Bus)";
    }

    if (QGuiApplication::platformName() == "xcb") {
        return "Linux X11 Session";
    }

    return "Wayland Desktop Session";
#else
    return "Desktop Session";
#endif
}
