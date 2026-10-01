#include <QApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include "SettingsManager.h"
#include "TimerEngine.h"
#include "AudioManager.h"
#include "SystemTrayManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("LookAway");
    QApplication::setOrganizationName("LookAway");
    QApplication::setQuitOnLastWindowClosed(false);

    // Single-instance enforcement via QLocalServer / QLocalSocket
    const QString ipcServerName = "LookAway_SingleInstance_IPC_Server";
    QLocalSocket clientSocket;
    clientSocket.connectToServer(ipcServerName);
    if (clientSocket.waitForConnected(300)) {
        // Another instance is already running! Inform it to bring its window to the foreground and exit.
        clientSocket.write("SHOW_WINDOW\n");
        clientSocket.waitForBytesWritten(500);
        return 0;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("LookAway - 20-20-20 Eye Care Background Utility");
    parser.addHelpOption();

    QCommandLineOption startMinimizedOption("minimized", "Start application hidden in system tray.");
    parser.addOption(startMinimizedOption);
    parser.process(app);

    SettingsManager settings;
    TimerEngine timerEngine(&settings);
    AudioManager audioManager(&settings, &timerEngine);
    SystemTrayManager trayManager(&timerEngine, &settings);
    MainWindow mainWindow(&timerEngine, &settings, &audioManager);

    // Listen for incoming requests from secondary launches to bring LookAway to the front
    QLocalServer::removeServer(ipcServerName); // Clean up stale sockets from crashes
    QLocalServer ipcServer;
    if (ipcServer.listen(ipcServerName)) {
        QObject::connect(&ipcServer, &QLocalServer::newConnection, [&ipcServer, &mainWindow]() {
            QLocalSocket* socket = ipcServer.nextPendingConnection();
            if (socket) {
                QObject::connect(socket, &QLocalSocket::readyRead, [socket, &mainWindow]() {
                    QByteArray cmd = socket->readAll();
                    if (cmd.contains("SHOW_WINDOW")) {
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
