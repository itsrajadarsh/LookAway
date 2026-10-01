#include "FullscreenDetector.h"
#include <QProcess>
#include <QStringList>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

bool FullscreenDetector::isFullscreenAppActive() {
#ifdef Q_OS_WIN
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    char className[256];
    if (GetClassNameA(fg, className, sizeof(className))) {
        if (strcmp(className, "Progman") == 0 ||
            strcmp(className, "WorkerW") == 0 ||
            strcmp(className, "Shell_TrayWnd") == 0) {
            return false;
        }
    }

    RECT appRect;
    if (!GetWindowRect(fg, &appRect)) return false;

    HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMon, &mi)) {
        if (appRect.left <= mi.rcMonitor.left &&
            appRect.top <= mi.rcMonitor.top &&
            appRect.right >= mi.rcMonitor.right &&
            appRect.bottom >= mi.rcMonitor.bottom) {
            return true;
        }
    }
    return false;
#elif defined(Q_OS_LINUX)
    // 1. Check active window via xprop on X11 / XWayland
    QProcess proc;
    proc.start("xprop", QStringList() << "-root" << "_NET_ACTIVE_WINDOW");
    if (proc.waitForFinished(150)) {
        QString out = QString::fromUtf8(proc.readAllStandardOutput());
        QRegularExpression rx("0x[0-9a-fA-F]+");
        QRegularExpressionMatch match = rx.match(out);
        if (match.hasMatch()) {
            QString winId = match.captured(0);
            QProcess stateProc;
            stateProc.start("xprop", QStringList() << "-id" << winId << "_NET_WM_STATE");
            if (stateProc.waitForFinished(150)) {
                QString stateOut = QString::fromUtf8(stateProc.readAllStandardOutput());
                if (stateOut.contains("_NET_WM_STATE_FULLSCREEN")) {
                    return true;
                }
            }
        }
    }

    // 2. Check D-Bus screensaver/idle inhibition (used by media players and browsers during fullscreen playback)
    QProcess dbusProc;
    dbusProc.start("gdbus", QStringList() << "call" << "--session"
                                         << "--dest" << "org.freedesktop.ScreenSaver"
                                         << "--object-path" << "/ScreenSaver"
                                         << "--method" << "org.freedesktop.ScreenSaver.GetActive");
    if (dbusProc.waitForFinished(150)) {
        QString dbusOut = QString::fromUtf8(dbusProc.readAllStandardOutput());
        if (dbusOut.contains("true")) {
            return true;
        }
    }

    return false;
#else
    return false;
#endif
}
