#ifndef ERGONOMICTIPCATALOG_H
#define ERGONOMICTIPCATALOG_H

#include <QString>
#include <QList>

struct ErgonomicTip {
    QString category;    // "EYE RELIEF", "DRY EYE RESET", "FOCUS MUSCLES", "POSTURE & SPINE"
    QString title;       // Short routine title
    QString instruction; // Actionable clinical instruction
    QString benefit;     // Clinical ergonomic benefit
    bool isMacro;        // True if primarily intended for longer breaks (>= 1m)
};

class ErgonomicTipCatalog {
public:
    static const QList<ErgonomicTip>& allTips();
    static ErgonomicTip getRandomTip(bool preferMacro = false);
    static ErgonomicTip getNextTip(const QString& currentTitle, bool preferMacro = false);
};

#endif // ERGONOMICTIPCATALOG_H
