#include <QApplication>
#include <QGuiApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include "SettingsManager.h"
#include "TimerEngine.h"
#include "AudioManager.h"
#include "SystemTrayManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QApplication app(argc, argv);
    QApplication::setApplicationName("LookAway");
    QApplication::setOrganizationName("itsrajadarsh");
    QApplication::setOrganizationDomain("github.com/itsrajadarsh");
    QApplication::setApplicationVersion("2.0.0");
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("LookAway - 20-20-20 Eye Care Background Utility");
    parser.addHelpOption();

    QCommandLineOption startMinimizedOption("minimized", "Start application hidden in system tray.");
    QCommandLineOption toggleOption("toggle", "Pause or resume the active timer.");
    QCommandLineOption snoozeOption("snooze", "Snooze the active or upcoming break.");
    QCommandLineOption skipOption("skip", "Skip the active break.");
    QCommandLineOption dndOption("dnd", "Toggle focus Do Not Disturb (DND) mode.");
    parser.addOption(startMinimizedOption);
    parser.addOption(toggleOption);
    parser.addOption(snoozeOption);
    parser.addOption(skipOption);
    parser.addOption(dndOption);
    parser.process(app);

    // Single-instance enforcement & CLI action routing via QLocalServer / QLocalSocket
    const QString ipcServerName = "LookAway_SingleInstance_IPC_Server";
    QLocalSocket clientSocket;
    clientSocket.connectToServer(ipcServerName);
    if (clientSocket.waitForConnected(300)) {
        if (parser.isSet(toggleOption)) {
            clientSocket.write("TOGGLE\n");
        } else if (parser.isSet(snoozeOption)) {
            clientSocket.write("SNOOZE\n");
        } else if (parser.isSet(skipOption)) {
            clientSocket.write("SKIP\n");
        } else if (parser.isSet(dndOption)) {
            clientSocket.write("DND\n");
        } else {
            clientSocket.write("SHOW_WINDOW\n");
        }
        clientSocket.waitForBytesWritten(500);
        return 0;
    }

    SettingsManager settings;
    TimerEngine timerEngine(&settings);
    AudioManager audioManager(&settings, &timerEngine);
    SystemTrayManager trayManager(&timerEngine, &settings);
    MainWindow mainWindow(&timerEngine, &settings, &audioManager);

    // Listen for incoming requests from secondary launches or global CLI shortcut bindings
    QLocalServer::removeServer(ipcServerName); // Clean up stale sockets from crashes
    QLocalServer ipcServer;
    if (ipcServer.listen(ipcServerName)) {
        QObject::connect(&ipcServer, &QLocalServer::newConnection, [&ipcServer, &mainWindow, &timerEngine, &settings]() {
            QLocalSocket* socket = ipcServer.nextPendingConnection();
            if (socket) {
                QObject::connect(socket, &QLocalSocket::readyRead, [socket, &mainWindow, &timerEngine, &settings]() {
                    QByteArray cmd = socket->readAll();
                    if (cmd.contains("TOGGLE")) {
                        if (timerEngine.state() == TimerEngine::State::Working || timerEngine.state() == TimerEngine::State::Breaking) {
                            timerEngine.pause();
                        } else {
                            timerEngine.start();
                        }
                    } else if (cmd.contains("SNOOZE")) {
                        timerEngine.postponeBreak(settings.defaultPostponeSeconds());
                    } else if (cmd.contains("SKIP")) {
                        timerEngine.skipBreak();
                    } else if (cmd.contains("DND")) {
                        if (timerEngine.isDndActive()) {
                            timerEngine.disableDnd();
                        } else {
                            timerEngine.enableDnd(3600);
                        }
                    } else if (cmd.contains("SHOW_WINDOW")) {
                        mainWindow.show();
                        mainWindow.raise();
                        mainWindow.activateWindow();
                    }
                });
            }
        });
    }

    // Auto-start timer on launch by default 
    timerEngine.start();

    QObject::connect(&trayManager, &SystemTrayManager::showDashboardRequested, [&mainWindow]() {
        mainWindow.show();
        mainWindow.raise();
        mainWindow.activateWindow();
    });

    QObject::connect(&trayManager, &SystemTrayManager::showSettingsRequested, [&mainWindow]() {
        mainWindow.showSettingsTab();
    });

    if (!parser.isSet(startMinimizedOption)) {
        mainWindow.show();
    }

    return app.exec();
}
