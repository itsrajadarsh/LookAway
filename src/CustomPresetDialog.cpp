#include "CustomPresetDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QIcon>
#include <QIntValidator>
#include <QListView>

CustomPresetDialog::CustomPresetDialog(const CustomPreset* initialPreset, bool isEditing, QWidget* parent)
    : QDialog(parent),
      m_isEditing(isEditing),
      m_initialPreset(initialPreset) {

    setWindowTitle(m_isEditing ? "Edit Preset Profile" : "Save as Preset Profile");
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setFixedSize(390, 260);
    setModal(true);

    setupUi();
    applyTheme();

    if (m_initialPreset) {
        if (m_isEditing || !m_initialPreset->name.isEmpty()) {
            m_editName->setText(m_initialPreset->name);
        }
        secondsToUi(m_initialPreset->workDurationSeconds, m_comboWorkVal, m_comboWorkUnit);
        secondsToUi(m_initialPreset->breakDurationSeconds, m_comboBreakVal, m_comboBreakUnit);
    } else {
        m_comboWorkVal->setCurrentText("20");
        m_comboWorkUnit->setCurrentText("Minutes");
        m_comboBreakVal->setCurrentText("20");
        m_comboBreakUnit->setCurrentText("Seconds");
    }

    m_editName->setFocus();
    m_editName->selectAll();
}

void CustomPresetDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(14);

    QLabel* titleLabel = new QLabel(m_isEditing ? "Edit Preset Profile" : "Save as Preset Profile");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #38bdf8;");
    mainLayout->addWidget(titleLabel);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(10);

    m_editName = new QLineEdit();
    m_editName->setPlaceholderText("e.g. Sprint, Reading, Gaming");
    m_editName->setMaxLength(30);
    m_editName->setStyleSheet("padding: 7px 10px; border: 1px solid #334155; border-radius: 6px; background: #0f172a; color: #f8fafc; font-size: 13px;");
    form->addRow("Profile Name:", m_editName);

    auto setupDialogCombo = [](QComboBox* combo) {
        combo->setCompleter(nullptr);
        combo->setMaxVisibleItems(6);
        QListView* lv = new QListView(combo);
        lv->setMouseTracking(true);
        QPalette pal = lv->palette();
        pal.setColor(QPalette::Base, QColor("#0f172a"));
        pal.setColor(QPalette::Window, QColor("#0f172a"));
        pal.setColor(QPalette::Text, QColor("#f8fafc"));
        pal.setColor(QPalette::Highlight, QColor("#0284c7"));
        pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
        lv->setPalette(pal);
        lv->setStyleSheet(R"(
            QListView {
                background-color: #0f172a;
                color: #f8fafc;
                border: 1px solid #38bdf8;
                border-radius: 6px;
                padding: 4px;
                outline: none;
            }
            QListView::item {
                background-color: #0f172a;
                color: #f8fafc;
                padding: 6px 12px;
                border-radius: 4px;
                min-height: 22px;
            }
            QListView::item:selected:!hover {
                background-color: #1e293b;
                color: #38bdf8;
            }
            QListView::item:hover,
            QListView::item:selected:hover {
                background-color: #0284c7;
                color: #ffffff;
            }
        )");
        combo->setView(lv);
    };

    // Work Interval controls
    QHBoxLayout* workLayout = new QHBoxLayout();
    m_comboWorkVal = new QComboBox();
    m_comboWorkVal->setEditable(true);
    m_comboWorkVal->setValidator(new QIntValidator(1, 9999, m_comboWorkVal));
    m_comboWorkVal->addItems({"1", "2", "3", "5", "10", "15", "20", "25", "30", "45", "50", "60"});
    m_comboWorkUnit = new QComboBox();
    m_comboWorkUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboWorkUnit->setFixedWidth(100);
    setupDialogCombo(m_comboWorkVal);
    setupDialogCombo(m_comboWorkUnit);
    workLayout->addWidget(m_comboWorkVal, 1);
    workLayout->addWidget(m_comboWorkUnit);
    form->addRow("Work Duration:", workLayout);

    // Break Duration controls
    QHBoxLayout* breakLayout = new QHBoxLayout();
    m_comboBreakVal = new QComboBox();
    m_comboBreakVal->setEditable(true);
    m_comboBreakVal->setValidator(new QIntValidator(1, 9999, m_comboBreakVal));
    m_comboBreakVal->addItems({"5", "10", "15", "20", "30", "45", "1", "2", "3", "5", "10", "15"});
    m_comboBreakUnit = new QComboBox();
    m_comboBreakUnit->addItems({"Seconds", "Minutes", "Hours"});
    m_comboBreakUnit->setFixedWidth(100);
    setupDialogCombo(m_comboBreakVal);
    setupDialogCombo(m_comboBreakUnit);
    breakLayout->addWidget(m_comboBreakVal, 1);
    breakLayout->addWidget(m_comboBreakUnit);
    form->addRow("Break Duration:", breakLayout);

    mainLayout->addLayout(form);
    mainLayout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch();

    m_btnCancel = new QPushButton("Cancel");
    m_btnCancel->setFixedHeight(36);
    m_btnCancel->setFixedWidth(90);
    m_btnCancel->setStyleSheet("background-color: #334155; color: #f8fafc; border: none; border-radius: 6px; font-weight: 600; font-size: 13px;");
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton(m_isEditing ? "Update" : "Save Profile");
    m_btnSave->setFixedHeight(36);
    m_btnSave->setFixedWidth(110);
    m_btnSave->setStyleSheet("background-color: #0284c7; color: #ffffff; border: none; border-radius: 6px; font-weight: 700; font-size: 13px;");
    m_btnSave->setDefault(true);
    connect(m_btnSave, &QPushButton::clicked, this, &CustomPresetDialog::validateAndAccept);

    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnSave);
    mainLayout->addLayout(btnLayout);
}

void CustomPresetDialog::validateAndAccept() {
    QString name = m_editName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Profile Name Required", "Please enter a descriptive name for your preset profile.");
        m_editName->setFocus();
        return;
    }

    int workSecs = durationToSeconds(m_comboWorkVal, m_comboWorkUnit);
    int breakSecs = durationToSeconds(m_comboBreakVal, m_comboBreakUnit);

    if (workSecs <= 0 || breakSecs <= 0) {
        QMessageBox::warning(this, "Invalid Durations", "Work and break durations must both be greater than zero.");
        return;
    }

    accept();
}

CustomPreset CustomPresetDialog::preset() const {
    CustomPreset p;
    p.name = m_editName->text().trimmed();
    p.workDurationSeconds = durationToSeconds(m_comboWorkVal, m_comboWorkUnit);
    p.breakDurationSeconds = durationToSeconds(m_comboBreakVal, m_comboBreakUnit);
    return p;
}

int CustomPresetDialog::durationToSeconds(QComboBox* valCombo, QComboBox* unitCombo) const {
    int val = valCombo->currentText().toInt();
    if (val <= 0) val = 1;
    QString unit = unitCombo->currentText();
    if (unit == "Hours") {
        return val * 3600;
    } else if (unit == "Minutes") {
        return val * 60;
    }
    return val;
}

void CustomPresetDialog::secondsToUi(int totalSeconds, QComboBox* valCombo, QComboBox* unitCombo) {
    if (totalSeconds <= 0) {
        valCombo->setCurrentText("1");
        unitCombo->setCurrentText("Seconds");
        return;
    }

    if (totalSeconds % 3600 == 0) {
        valCombo->setCurrentText(QString::number(totalSeconds / 3600));
        unitCombo->setCurrentText("Hours");
    } else if (totalSeconds % 60 == 0) {
        valCombo->setCurrentText(QString::number(totalSeconds / 60));
        unitCombo->setCurrentText("Minutes");
    } else {
        valCombo->setCurrentText(QString::number(totalSeconds));
        unitCombo->setCurrentText("Seconds");
    }
}

void CustomPresetDialog::applyTheme() {
    setStyleSheet(R"(
        QDialog {
            background-color: #0f172a;
        }
        QLabel {
            color: #f8fafc;
            font-size: 13px;
        }
        QComboBox {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 5px;
            color: #f8fafc;
        }
        QComboBox QAbstractItemView {
            background-color: #0f172a;
            color: #f8fafc;
            border: 1px solid #334155;
            selection-background-color: #0284c7;
        }
        QPushButton:hover {
            opacity: 0.9;
        }
    )");
}
