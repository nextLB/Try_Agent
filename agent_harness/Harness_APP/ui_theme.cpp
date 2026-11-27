
#include "ui_theme.h"

#include <QApplication>
#include <QColor>
#include <QFont>
#include <QPalette>
#include <QStyleFactory>


// ============================================================
// 初始化应用程序深色主题
// ============================================================

void applyDarkTheme(QApplication &app)
{
    // --------------------------------------------------------
    // 1. 设置 Qt Fusion 风格
    // --------------------------------------------------------

    app.setStyle(QStyleFactory::create("Fusion"));


    // --------------------------------------------------------
    // 2. 设置调色板
    // --------------------------------------------------------

    QPalette palette;

    palette.setColor(QPalette::Window, QColor("#0B1016"));
    palette.setColor(QPalette::WindowText, QColor("#E6EDF3"));
    palette.setColor(QPalette::Base, QColor("#111923"));
    palette.setColor(QPalette::AlternateBase, QColor("#17212C"));
    palette.setColor(QPalette::ToolTipBase, QColor("#18232E"));
    palette.setColor(QPalette::ToolTipText, QColor("#E6EDF3"));
    palette.setColor(QPalette::Text, QColor("#DCE5EE"));
    palette.setColor(QPalette::Button, QColor("#1A2632"));
    palette.setColor(QPalette::ButtonText, QColor("#E6EDF3"));
    palette.setColor(QPalette::BrightText, QColor("#FF6B7A"));
    palette.setColor(QPalette::Highlight, QColor("#54CDB7"));
    palette.setColor(QPalette::HighlightedText, QColor("#071411"));
    palette.setColor(QPalette::Link, QColor("#69D8C4"));
    palette.setColor(QPalette::PlaceholderText, QColor("#778595"));

    app.setPalette(palette);


    // --------------------------------------------------------
    // 3. 设置全局字体
    // --------------------------------------------------------

    QFont appFont("Segoe UI");

    appFont.setPointSize(10);

    app.setFont(appFont);


    // --------------------------------------------------------
    // 4. 设置全局 QSS 样式表
    // --------------------------------------------------------

    app.setStyleSheet(R"(
        QToolTip {
            color: #E6EDF3;
            background-color: #18232E;
            border: 1px solid #344454;
            padding: 5px 8px;
            border-radius: 6px;
        }

        QMainWindow#mainWindow,
        QWidget#rootSurface {
            background-color: #0B1016;
        }

        QWidget#titleBar {
            background-color: transparent;
        }

        QLabel#titleIconLabel {
            background-color: transparent;
        }

        QLabel#appTitleLabel {
            color: #E6EDF3;
            background-color: transparent;
            font-size: 16px;
            font-weight: 600;
        }

        QWidget#connectionPanel {
            background-color: #111923;
            border: 1px solid #344454;
            border-radius: 12px;
        }

        QComboBox#modelCombo {
            color: #DCE5EE;
            background-color: #0B1016;
            border: 1px solid #344454;
            border-radius: 8px;
            padding: 6px 10px;
            selection-background-color: #276B60;
        }

        QComboBox#modelCombo QLineEdit {
            color: #DCE5EE;
            background-color: transparent;
            border: none;
            padding: 0;
        }

        QComboBox#modelCombo QAbstractItemView {
            color: #DCE5EE;
            background-color: #111923;
            border: 1px solid #344454;
            selection-background-color: #276B60;
        }

        QLineEdit#skKeyInput {
            color: #DCE5EE;
            background-color: #0B1016;
            border: 1px solid #344454;
            border-radius: 8px;
            padding: 6px 10px;
            selection-background-color: #276B60;
        }

        QPushButton#connectButton,
        QPushButton#disconnectButton {
            color: #071411;
            background-color: #54CDB7;
            border: none;
            border-radius: 8px;
            padding: 6px 18px;
            font-weight: 600;
        }

        QPushButton#disconnectButton {
            color: #E6EDF3;
            background-color: #344454;
        }

        QPushButton#connectButton:hover,
        QPushButton#disconnectButton:hover {
            background-color: #69D8C4;
        }

        QPushButton#connectButton:disabled,
        QPushButton#disconnectButton:disabled {
            color: #718078;
            background-color: #293B3A;
        }

        QLabel#userIdLabel {
            color: #69D8C4;
            background-color: transparent;
        }

        QWidget#homeCanvas {
            background: qlineargradient(
                x1: 0, y1: 0, x2: 1, y2: 1,
                stop: 0 #141E29,
                stop: 1 #101720
            );
            border: 1px solid #273442;
            border-radius: 14px;
        }

        QTextEdit#chatView {
            color: #DCE5EE;
            background-color: #111923;
            border: 1px solid #273442;
            border-radius: 14px;
            padding: 14px 18px;
            font-size: 11px;
            selection-background-color: #276B60;
        }

        QWidget#composerPanel {
            background-color: #111923;
            border: 1px solid #344454;
            border-radius: 14px;
        }

        QPlainTextEdit#messageInput {
            color: #DCE5EE;
            background-color: transparent;
            border: none;
            selection-background-color: #276B60;
            padding: 4px;
        }

        QPushButton#sendButton {
            color: #071411;
            background-color: #54CDB7;
            border: none;
            border-radius: 8px;
            padding: 9px 22px;
            font-weight: 600;
        }

        QPushButton#sendButton:hover {
            background-color: #69D8C4;
        }

        QPushButton#sendButton:pressed {
            background-color: #3EAD9A;
        }

        QPushButton#sendButton:disabled {
            color: #718078;
            background-color: #293B3A;
        }
    )");
}
