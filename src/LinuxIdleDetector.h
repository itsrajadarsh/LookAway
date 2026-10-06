#ifndef LINUXIDLEDETECTOR_H
#define LINUXIDLEDETECTOR_H

#include <QtGlobal>
#include <QString>

class LinuxIdleDetector {
public:
    static qint64 getIdletimeMs();
    static QString activeBackendName();
};

#endif // LINUXIDLEDETECTOR_H
