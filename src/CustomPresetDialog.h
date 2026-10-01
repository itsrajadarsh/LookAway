#ifndef CUSTOMPRESETDIALOG_H
#define CUSTOMPRESETDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include "SettingsManager.h"

class CustomPresetDialog : public QDialog {
    Q_OBJECT

public:
    explicit CustomPresetDialog(const CustomPreset* initialPreset = nullptr, bool isEditing = false, QWidget* parent = nullptr);

    CustomPreset preset() const;

private slots:
    void validateAndAccept();

private:
    void setupUi();
    void applyTheme();
    int durationToSeconds(QComboBox* valCombo, QComboBox* unitCombo) const;
    void secondsToUi(int totalSeconds, QComboBox* valCombo, QComboBox* unitCombo);

    bool m_isEditing;
    const CustomPreset* m_initialPreset;
    QLineEdit* m_editName;
    QComboBox* m_comboWorkVal;
    QComboBox* m_comboWorkUnit;
    QComboBox* m_comboBreakVal;
    QComboBox* m_comboBreakUnit;
    QPushButton* m_btnSave;
    QPushButton* m_btnCancel;
};

#endif // CUSTOMPRESETDIALOG_H
