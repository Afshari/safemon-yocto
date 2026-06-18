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
        background-color: #2b2b2b;
        color: #e0e0e0;
        font-family: Segoe UI, Arial, sans-serif;
        font-size: 13px;
    }

    QTabWidget::pane {
        border: none;
        background-color: #2b2b2b;
    }

    QTabBar::tab {
        background-color: #3c3c3c;
        color: #aaaaaa;
        padding: 8px 18px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
    }

    QTabBar::tab:selected {
        background-color: #1e88e5;
        color: #ffffff;
    }

    QTabBar::tab:hover:!selected {
        background-color: #4a4a4a;
        color: #e0e0e0;
    }

    QPushButton {
        background-color: #1e88e5;
        color: #ffffff;
        border: none;
        padding: 7px 16px;
        border-radius: 4px;
        font-size: 13px;
    }

    QPushButton:hover {
        background-color: #1976d2;
    }

    QPushButton:pressed {
        background-color: #1565c0;
    }

    QPushButton:disabled {
        background-color: #555555;
        color: #888888;
    }

    QLineEdit, QSpinBox {
        background-color: #3c3c3c;
        color: #e0e0e0;
        border: 1px solid #555555;
        border-radius: 4px;
        padding: 5px 8px;
    }

    QLineEdit:focus, QSpinBox:focus {
        border: 1px solid #1e88e5;
    }

    QLabel {
        color: #e0e0e0;
    }

    QGroupBox {
        border: 1px solid #555555;
        border-radius: 4px;
        margin-top: 10px;
        padding-top: 10px;
        color: #aaaaaa;
        font-size: 12px;
    }

    QGroupBox::title {
        subcontrol-origin: margin;
        subcontrol-position: top left;
        padding: 0 6px;
        color: #aaaaaa;
    }

    QTableWidget {
        background-color: #313131;
        alternate-background-color: #2b2b2b;
        gridline-color: #444444;
        border: none;
    }

    QTableWidget::item {
        padding: 4px 8px;
    }

    QHeaderView::section {
        background-color: #3c3c3c;
        color: #aaaaaa;
        padding: 6px 8px;
        border: none;
        border-right: 1px solid #555555;
        font-size: 12px;
    }

    QScrollBar:vertical {
        background-color: #2b2b2b;
        width: 10px;
        border-radius: 5px;
    }

    QScrollBar::handle:vertical {
        background-color: #555555;
        border-radius: 5px;
        min-height: 20px;
    }

    QScrollBar::handle:vertical:hover {
        background-color: #1e88e5;
    }

    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
    }

    QComboBox {
        background-color: #3c3c3c;
        color: #e0e0e0;
        border: 1px solid #555555;
        border-radius: 4px;
        padding: 5px 8px;
    }

    QComboBox::drop-down {
        border: none;
        width: 24px;
    }

    QComboBox QAbstractItemView {
        background-color: #3c3c3c;
        color: #e0e0e0;
        selection-background-color: #1e88e5;
    }

    QTextEdit {
        background-color: #1e1e1e;
        color: #e0e0e0;
        border: 1px solid #555555;
        border-radius: 4px;
        font-family: Consolas, Courier New, monospace;
        font-size: 12px;
    }

    QSplitter::handle {
        background-color: #444444;
    }
    """


if __name__ == "__main__":
    main()