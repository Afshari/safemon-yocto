"""
workers.py - QThread worker base class.
Runs a blocking function in a background thread and reports back
to the UI thread safely via Qt signals.
"""

import traceback
from PyQt6.QtCore import QThread, pyqtSignal


class Worker(QThread):
    """
    Generic background worker.

    Usage:
        worker = Worker(some_function, arg1, arg2, kwarg1=value)
        worker.finished_ok.connect(on_success)
        worker.failed.connect(on_error)
        worker.start()

    The target function runs in a background thread. Do not touch
    any QWidget directly inside the target function - only return
    a value or raise an exception. Update widgets only inside the
    slots connected to finished_ok / failed.
    """

    finished_ok = pyqtSignal(object)
    failed = pyqtSignal(str)

    def __init__(self, target, *args, **kwargs):
        super().__init__()
        self._target = target
        self._args = args
        self._kwargs = kwargs

    def run(self):
        try:
            result = self._target(*self._args, **self._kwargs)
        except Exception:
            self.failed.emit(traceback.format_exc())
            return
        self.finished_ok.emit(result)