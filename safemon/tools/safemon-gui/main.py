"""
main.py - Entry point for safemon-gui.
Initializes config files on first run, then launches the main window.
"""

import sys
from pathlib import Path

from PyQt6.QtWidgets import QApplication, QStyleFactory
from PyQt6.QtGui import QIcon

from core.config_manager import ensure_all_configs
from ui.main_window import MainWindow


def main():
    # Step 1 - ensure all config files exist before anything else
    ensure_all_configs()

    # Step 2 - create the Qt application
    app = QApplication(sys.argv)
    app.setApplicationName("safemon-gui")
    app.setApplicationDisplayName("Safemon")

    # Step 3 - force Fusion style so QSS border-radius and colors work on Windows
    app.setStyle(QStyleFactory.create("Fusion"))

    # Step 4 - apply dark theme stylesheet
    app.setStyleSheet(_stylesheet())

    # Step 5 - create and show the main window
    window = MainWindow()
    window.show()

    sys.exit(app.exec())


def _stylesheet() -> str:
    return """
    QMainWindow, QWidget {
        background-color: #242424;
        color: #FFF;
        font-family: Segoe UI, Arial, sans-serif;
        font-size: 13px;
    }

    QTabWidget::pane {
        border: none;
        background-color: #242424;
    }

    QTabBar::tab {
        background-color: #4a4a4a;
        color: grey;
        padding: 8px 18px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
    }

    QTabBar::tab:selected {
        background-color: #1f6aa5;
        color: #FFF;
    }

    QTabBar::tab:hover:!selected {
        background-color: #666;
        color: #FFF;
    }

    QPushButton {
        background-color: #1f6aa5;
        color: #FFF;
        border: none;
        padding: 7px 16px;
        border-radius: 4px;
        font-size: 13px;
    }

    QPushButton:hover {
        background-color: #2a7ab8;
    }

    QPushButton:pressed {
        background-color: #18527e;
    }

    QPushButton:disabled {
        background-color: #4a4a4a;
        color: grey;
    }

    QLineEdit, QSpinBox {
        background-color: #4a4a4a;
        color: #FFF;
        border: 1px solid #666;
        border-radius: 4px;
        padding: 5px 8px;
    }

    QLineEdit:focus, QSpinBox:focus {
        border: 1px solid #1f6aa5;
    }

    QLabel {
        color: #FFF;
    }

    QGroupBox {
        border: 1px solid #4a4a4a;
        border-radius: 4px;
        margin-top: 10px;
        padding-top: 10px;
        color: grey;
        font-size: 12px;
    }

    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        padding: 0 6px;
        color: grey;
    }

    QTableWidget {
        background-color: #2e2e2e;
        alternate-background-color: #242424;
        gridline-color: #4a4a4a;
        border: none;
    }

    QTableWidget::item {
        padding: 4px 8px;
    }

    QHeaderView::section {
        background-color: #4a4a4a;
        color: grey;
        padding: 6px 8px;
        border: none;
        border-right: 1px solid #666;
        font-size: 12px;
    }

    QScrollBar:vertical {
        background-color: #242424;
        width: 10px;
        border-radius: 5px;
    }

    QScrollBar::handle:vertical {
        background-color: #64686b;
        border-radius: 5px;
        min-height: 20px;
    }

    QScrollBar::handle:vertical:hover {
        background-color: #1f6aa5;
    }

    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
    }

    QComboBox {
        background-color: #444;
        color: #FFF;
        border: 1px solid #666;
        border-radius: 4px;
        padding: 5px 8px;
    }

    QComboBox::drop-down {
        border: none;
        width: 24px;
    }

    QComboBox QAbstractItemView {
        background-color: #444;
        color: #FFF;
        selection-background-color: #1f6aa5;
    }

    QTextEdit {
        background-color: #1c1c1c;
        color: #FFF;
        border: 1px solid #4a4a4a;
        border-radius: 4px;
        font-family: Consolas, Courier New, monospace;
        font-size: 12px;
    }

    QSplitter::handle {
        background-color: #4a4a4a;
    }
    """

if __name__ == "__main__":
    main()