#pragma once

#include <QString>

// 全局暗色主题样式表。三个工作台共用，保证界面观感统一。
inline QString appStyleSheet()
{
    return QString::fromUtf8(R"QSS(
QWidget {
    background-color: #1e1e24;
    color: #e0e0e0;
    font-family: "Segoe UI", "Microsoft YaHei", sans-serif;
    font-size: 13px;
}
QFrame[card="true"] {
    background-color: #26262e;
    border-radius: 6px;
    border: 1px solid #33333d;
}
QPushButton {
    background-color: #3b3b47;
    border: 1px solid #4a4a5a;
    border-radius: 4px;
    padding: 6px 14px;
    color: #ffffff;
    font-weight: 500;
}
QPushButton:hover {
    background-color: #484858;
}
QPushButton:pressed {
    background-color: #2b2b35;
}
QPushButton:disabled {
    background-color: #222228;
    color: #555560;
    border-color: #2a2a30;
}
QPushButton#primaryButton {
    background-color: #2563eb;
    border: 1px solid #3b82f6;
}
QPushButton#primaryButton:hover {
    background-color: #1d4ed8;
}
QSlider::groove:horizontal {
    height: 4px;
    background: #3a3a46;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: #2563eb;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #ffffff;
    width: 12px;
    margin: -4px 0;
    border-radius: 6px;
}
QProgressBar {
    border: 1px solid #33333d;
    border-radius: 4px;
    background-color: #1a1a20;
    text-align: center;
    color: #ffffff;
}
QProgressBar::chunk {
    background-color: #2563eb;
    border-radius: 3px;
}
QComboBox, QLineEdit {
    background-color: #2a2a34;
    border: 1px solid #3a3a46;
    border-radius: 4px;
    padding: 3px 6px;
    color: #e0e0e0;
}
QComboBox QAbstractItemView {
    background-color: #26262e;
    border: 1px solid #33333d;
    color: #e0e0e0;
    selection-background-color: #2563eb;
}
QGroupBox {
    border: 1px solid #33333d;
    border-radius: 6px;
    margin-top: 12px;
    padding-top: 8px;
    color: #b0b0c0;
    font-weight: 500;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 8px;
    padding: 0 4px;
}
QCheckBox {
    spacing: 6px;
}
QLabel#timeLabel {
    color: #9fd0ff;
    font-family: "Consolas", monospace;
}
QLabel#hintLabel {
    color: #ff6b6b;
}
QStatusBar {
    background-color: #26262e;
    color: #b0b0c0;
}
)QSS");
}
