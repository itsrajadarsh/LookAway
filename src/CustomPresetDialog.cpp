#include "CustomPresetDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QIcon>
#include <QListView>
#include <QStyledItemDelegate>

CustomPresetDialog::CustomPresetDialog(const CustomPreset* initialPreset, bool isEditing, QWidget* parent)
    : QDialog(parent),
      m_isEditing(isEditing),
      m_initialPreset(initialPreset) {

    setWindowTitle(m_isEditing ? "Edit Preset Profile" : "Save as Preset Profile");
    setWindowIcon(QIcon(":/icons/app_icon.svg"));
    setFixedSize(450, 280);
    setModal(true);

    setupUi();
    applyTheme();

    if (m_initialPreset) {
        if (m_isEditing || !m_initialPreset->name.isEmpty()) {
            m_editName->setText(m_initialPreset->name);
        }
        secondsToUi(m_initialPreset->workDurationSeconds, m_spinWorkVal, m_comboWorkUnit);
        secondsToUi(m_initialPreset->breakDurationSeconds, m_spinBreakVal, m_comboBreakUnit);
    } else {
        m_spinWorkVal->setValue(20);
        m_comboWorkUnit->setCurrentText("Minutes");
        m_spinBreakVal->setValue(20);
        m_comboBreakUnit->setCurrentText("Seconds");
    }

    m_editName->setFocus();
    m_editName->selectAll();
}

void CustomPresetDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(22, 20, 22, 20);
    mainLayout->setSpacing(16);

    QLabel* titleLabel = new QLabel(m_isEditing ? "Edit Preset Profile" : "Save as Preset Profile");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: 700; color: #38bdf8;");
    mainLayout->addWidget(titleLabel);

    QFormLayout* form = new QFormLayout();
    form->setSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft);

    m_editName = new QLineEdit();
    m_editName->setPlaceholderText("e.g. Sprint, Reading, Deep Focus");
    m_editName->setMaxLength(30);
    m_editName->setFixedHeight(36);
    m_editName->setStyleSheet("padding: 6px 10px; border: 1px solid #334155; border-radius: 6px; background: #0f172a; color: #f8fafc; font-size: 13px; font-weight: 500;");
    form->addRow("Profile Name:", m_editName);

    auto setupUnitCombo = [](QComboBox* combo) {
        combo->setMaxVisibleItems(5);
        QListView* lv = new QListView(combo);
        lv->setMouseTracking(true);

        QPalette pal = lv->palette();
        pal.setColor(QPalette::Base, QColor("#0f172a"));
        pal.setColor(QPalette::Window, QColor("#0f172a"));
        pal.setColor(QPalette::Text, QColor("#f8fafc"));
        pal.setColor(QPalette::Highlight, QColor("#0284c7"));
        pal.setColor(QPalette::HighlightedText, QColor("#ffffff"));
        lv->setPalette(pal);

        combo->setView(lv);

        if (QFrame* frame = qobject_cast<QFrame*>(combo->view()->parentWidget())) {
            frame->setObjectName("comboContainer");
            frame->setFrameShape(QFrame::NoFrame);
            frame->setLineWidth(0);
            frame->setContentsMargins(0, 0, 0, 0);
            frame->setAttribute(Qt::WA_TranslucentBackground, true);
            frame->setStyleSheet("QFrame#comboContainer { background-color: #0f172a; border: 1px solid #38bdf8; border-radius: 6px; }");
        }

        lv->setStyleSheet(R"(
            QListView {
                background-color: transparent;
                color: #f8fafc;
                border: none;
                padding: 4px;
                outline: none;
            }
            QListView::item {
                background-color: transparent;
                color: #f8fafc;
                padding: 6px 12px;
                border-radius: 4px;
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
    };

    // Work Interval controls: SpinBox + Unit ComboBox
    QHBoxLayout* workLayout = new QHBoxLayout();
    workLayout->setSpacing(8);
    m_spinWorkVal = new QSpinBox();
    m_spinWorkVal->setRange(1, 9999);
    m_spinWorkVal->setValue(20);
    m_spinWorkVal->setFixedHeight(36);

    m_comboWorkUnit = new QComboBox();
    m_comboWorkUnit->addItems({"Minutes", "Seconds", "Hours"});
    m_comboWorkUnit->setFixedWidth(120);
    m_comboWorkUnit->setFixedHeight(36);
    setupUnitCombo(m_comboWorkUnit);

    workLayout->addWidget(m_spinWorkVal, 1);
    workLayout->addWidget(m_comboWorkUnit);
    form->addRow("Work Duration:", workLayout);

    // Break Duration controls: SpinBox + Unit ComboBox
    QHBoxLayout* breakLayout = new QHBoxLayout();
    breakLayout->setSpacing(8);
    m_spinBreakVal = new QSpinBox();
    m_spinBreakVal->setRange(1, 9999);
    m_spinBreakVal->setValue(20);
    m_spinBreakVal->setFixedHeight(36);

    m_comboBreakUnit = new QComboBox();
    m_comboBreakUnit->addItems({"Minutes", "Seconds", "Hours"});
    m_comboBreakUnit->setFixedWidth(120);
    m_comboBreakUnit->setFixedHeight(36);
    setupUnitCombo(m_comboBreakUnit);

    breakLayout->addWidget(m_spinBreakVal, 1);
    breakLayout->addWidget(m_comboBreakUnit);
    form->addRow("Break Duration:", breakLayout);

    mainLayout->addLayout(form);
    mainLayout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(10);
    btnLayout->addStretch();

    m_btnCancel = new QPushButton("Cancel");
    m_btnCancel->setFixedHeight(36);
    m_btnCancel->setFixedWidth(95);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet("background-color: #334155; color: #f8fafc; border: none; border-radius: 6px; font-weight: 600; font-size: 13px;");
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnSave = new QPushButton(m_isEditing ? "Update" : "Save Profile");
    m_btnSave->setFixedHeight(36);
    m_btnSave->setFixedWidth(115);
    m_btnSave->setCursor(Qt::PointingHandCursor);
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

    int workSecs = durationToSeconds(m_spinWorkVal, m_comboWorkUnit);
    int breakSecs = durationToSeconds(m_spinBreakVal, m_comboBreakUnit);

    if (workSecs <= 0 || breakSecs <= 0) {
        QMessageBox::warning(this, "Invalid Durations", "Work and break durations must both be greater than zero.");
        return;
    }

    accept();
}

CustomPreset CustomPresetDialog::preset() const {
    CustomPreset p;
    p.name = m_editName->text().trimmed();
    p.workDurationSeconds = durationToSeconds(m_spinWorkVal, m_comboWorkUnit);
    p.breakDurationSeconds = durationToSeconds(m_spinBreakVal, m_comboBreakUnit);
    return p;
}

int CustomPresetDialog::durationToSeconds(QSpinBox* valSpin, QComboBox* unitCombo) const {
    int val = valSpin->value();
    if (val <= 0) val = 1;
    QString unit = unitCombo->currentText();
    if (unit == "Hours") {
        return val * 3600;
    } else if (unit == "Minutes") {
        return val * 60;
    }
    return val; // Seconds
}

void CustomPresetDialog::secondsToUi(int totalSeconds, QSpinBox* valSpin, QComboBox* unitCombo) {
    if (totalSeconds <= 0) {
        valSpin->setValue(1);
        unitCombo->setCurrentText("Seconds");
        return;
    }

    if (totalSeconds % 3600 == 0) {
        valSpin->setValue(totalSeconds / 3600);
        unitCombo->setCurrentText("Hours");
    } else if (totalSeconds % 60 == 0) {
        valSpin->setValue(totalSeconds / 60);
        unitCombo->setCurrentText("Minutes");
    } else {
        valSpin->setValue(totalSeconds);
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
        QSpinBox {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 5px 8px;
            color: #f8fafc;
            font-size: 13px;
            font-weight: 600;
        }
        QSpinBox:focus {
            border: 1px solid #38bdf8;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            width: 18px;
            background: #1e293b;
            border: none;
            border-radius: 2px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background: #334155;
        }
        QComboBox {
            background-color: #0f172a;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 5px 10px;
            color: #f8fafc;
            font-size: 13px;
            font-weight: 500;
        }
        QComboBox:hover {
            border-color: #475569;
        }
        QComboBox:focus {
            border: 1px solid #38bdf8;
        }
        QFrame#comboContainer,
        QComboBoxPrivateContainer {
            background-color: #0f172a;
            border: 1px solid #38bdf8;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView,
        QComboBox QListView {
            background-color: transparent;
            color: #f8fafc;
            border: none;
            padding: 4px;
            outline: none;
        }
        QComboBox QAbstractItemView::item,
        QComboBox QListView::item {
            background-color: transparent;
            color: #f8fafc;
            padding: 6px 12px;
            border-radius: 4px;
        }
        QComboBox QAbstractItemView::item:selected:!hover,
        QComboBox QListView::item:selected:!hover {
            background-color: #1e293b;
            color: #38bdf8;
        }
        QComboBox QAbstractItemView::item:hover,
        QComboBox QAbstractItemView::item:selected:hover,
        QComboBox QListView::item:hover,
        QComboBox QListView::item:selected:hover {
            background-color: #0284c7;
            color: #ffffff;
        }
        QPushButton#btnCancel:hover {
            background-color: #475569;
        }
        QPushButton#btnSave:hover {
            background-color: #0369a1;
        }
    )");
}
