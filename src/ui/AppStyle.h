#pragma once

#include <QString>

namespace birdy {

inline QString appStyleSheet() {
    return QStringLiteral(
        "QMainWindow { background-color: #F5F5F5; }"
        "QWidget { font-family: 'Segoe UI', 'Microsoft YaHei UI', 'Arial', sans-serif; font-size: 10pt; }"
        "QLabel { color: #333333; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QListWidget {"
        "  background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 6px;"
        "  padding: 5px 10px; color: #333333; font-size: 10pt; min-height: 1.1em;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border: 1px solid #2E8B57;"
        "}"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QPushButton {"
        "  background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 6px;"
        "  padding: 6px 14px; color: #333333; font-weight: 500; font-size: 10pt; min-height: 1.2em;"
        "}"
        "QPushButton:hover { background-color: #F0F0F0; border: 1px solid #1E90FF; }"
        "QPushButton:pressed { background-color: #E0E0E0; }"
        "QPushButton:disabled { background-color: #F5F5F5; color: #999999; border: 1px solid #E0E0E0; }"
        "QCheckBox { spacing: 6px; font-size: 10pt; }"
        "QCheckBox::indicator {"
        "  width: 20px; height: 20px; border: 2px solid #E0E0E0; border-radius: 5px; background-color: #FFFFFF;"
        "}"
        "QCheckBox::indicator:checked { background-color: #2E8B57; border: 2px solid #2E8B57; }"
        "QCheckBox::indicator:hover { border: 2px solid #1E90FF; }"
        "QRadioButton { spacing: 6px; font-size: 10pt; }"
        "QRadioButton::indicator {"
        "  width: 20px; height: 20px; border: 2px solid #E0E0E0; border-radius: 10px; background-color: #FFFFFF;"
        "}"
        "QRadioButton::indicator:checked { background-color: #2E8B57; border: 2px solid #2E8B57; }"
        "QRadioButton::indicator:hover { border: 2px solid #1E90FF; }"
        "QProgressBar {"
        "  background-color: #F0F0F0; border: none; border-radius: 8px; text-align: center;"
        "  height: 24px; font-size: 10pt; font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #2E8B57, stop:1 #1E90FF);"
        "  border-radius: 8px;"
        "}"
        "QTextEdit, QPlainTextEdit {"
        "  background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 6px; padding: 6px;"
        "  font-family: 'Consolas', 'Courier New', 'Microsoft YaHei UI', monospace; font-size: 9pt;"
        "}"
        "QTableWidget { background-color: #FFFFFF; border: 1px solid #E0E0E0; border-radius: 6px; font-size: 10pt; }"
        "QHeaderView::section {"
        "  background-color: #F0F0F0; padding: 5px 8px; border: none;"
        "  border-bottom: 1px solid #E0E0E0; font-size: 10pt;"
        "}"
        "QGroupBox {"
        "  font-size: 10pt; border: 1px solid #E0E0E0; border-radius: 6px; margin-top: 12px; padding: 8px;"
        "}"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }"
        "QScrollArea { border: none; background: transparent; }"
        "QScrollArea > QWidget > QWidget { background: transparent; }");
}

inline QString flowCheckStyle() {
    return QStringLiteral(
        "QCheckBox { font-size: 9pt; color: #2E6B4A; font-weight: 600; spacing: 4px; padding: 0 4px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; }");
}

inline QString startButtonStyle() {
    return QStringLiteral(
        "QPushButton { background-color: #2E8B57; color: white; font-weight: bold; padding: 6px 14px;"
        "  border-radius: 6px; font-size: 10pt; border: none; }"
        "QPushButton:hover:enabled { background-color: #277A4B; }"
        "QPushButton:pressed:enabled { background-color: #226A3F; }"
        "QPushButton:disabled { background-color: #BDC3C7; color: #7F8C8D; }");
}

inline QString stopButtonStyle() {
    return QStringLiteral(
        "QPushButton { background-color: #E74C3C; color: white; font-weight: bold; padding: 6px 14px;"
        "  border-radius: 6px; font-size: 10pt; border: none; }"
        "QPushButton:hover:enabled { background-color: #C0392B; }"
        "QPushButton:pressed:enabled { background-color: #A93226; }"
        "QPushButton:disabled { background-color: #BDC3C7; color: #7F8C8D; }");
}

}  // namespace birdy
