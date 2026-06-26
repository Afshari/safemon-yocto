"""
test_workers.py - Unit tests for core/workers.py.
Tests QThread worker signal emission, error handling,
and thread safety. Requires pytest-qt.
"""

import threading
import time
import pytest
from pytestqt.plugin import QtBot

from core.workers import Worker


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def successful_task(x, y):
    return x + y


def failing_task():
    raise ValueError("intentional error for testing")


def slow_task(duration):
    time.sleep(duration)
    return "done"


def thread_id_task():
    return threading.get_ident()


# ---------------------------------------------------------------------------
# Signal emission
# ---------------------------------------------------------------------------

class TestWorkerSignals:

    def test_finished_ok_emitted_on_success(self, qtbot):
        worker = Worker(successful_task, 2, 3)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == 5

    def test_failed_emitted_on_exception(self, qtbot):
        worker = Worker(failing_task)
        with qtbot.waitSignal(worker.failed, timeout=3000) as blocker:
            worker.start()
        assert "ValueError" in blocker.args[0]
        assert "intentional error for testing" in blocker.args[0]

    def test_failed_contains_traceback(self, qtbot):
        worker = Worker(failing_task)
        with qtbot.waitSignal(worker.failed, timeout=3000) as blocker:
            worker.start()
        assert "Traceback" in blocker.args[0]

    def test_finished_ok_not_emitted_on_failure(self, qtbot):
        worker = Worker(failing_task)
        with qtbot.waitSignal(worker.failed, timeout=3000):
            worker.start()
        assert not worker.isRunning()

    def test_failed_not_emitted_on_success(self, qtbot):
        worker = Worker(successful_task, 1, 1)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000):
            worker.start()
        assert not worker.isRunning()


# ---------------------------------------------------------------------------
# Return values
# ---------------------------------------------------------------------------

class TestWorkerReturnValues:

    def test_returns_none(self, qtbot):
        worker = Worker(lambda: None)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] is None

    def test_returns_string(self, qtbot):
        worker = Worker(lambda: "hello")
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == "hello"

    def test_returns_dict(self, qtbot):
        worker = Worker(lambda: {"key": "value"})
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == {"key": "value"}

    def test_returns_tuple(self, qtbot):
        worker = Worker(lambda: (1, 2, 3))
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == (1, 2, 3)

    def test_passes_args_to_target(self, qtbot):
        worker = Worker(successful_task, 10, 20)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == 30

    def test_passes_kwargs_to_target(self, qtbot):
        def task_with_kwargs(a, b=0):
            return a + b

        worker = Worker(task_with_kwargs, 5, b=15)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        assert blocker.args[0] == 20


# ---------------------------------------------------------------------------
# Threading
# ---------------------------------------------------------------------------

class TestWorkerThreading:

    def test_runs_in_different_thread(self, qtbot):
        main_thread_id = threading.get_ident()
        worker = Worker(thread_id_task)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000) as blocker:
            worker.start()
        worker_thread_id = blocker.args[0]
        assert worker_thread_id != main_thread_id

    def test_worker_not_running_after_completion(self, qtbot):
        worker = Worker(successful_task, 1, 1)
        with qtbot.waitSignal(worker.finished_ok, timeout=3000):
            worker.start()
        qtbot.waitUntil(lambda: not worker.isRunning(), timeout=1000)
        assert not worker.isRunning()

    def test_worker_not_running_after_failure(self, qtbot):
        worker = Worker(failing_task)
        with qtbot.waitSignal(worker.failed, timeout=3000):
            worker.start()
        qtbot.waitUntil(lambda: not worker.isRunning(), timeout=1000)
        assert not worker.isRunning()

    def test_multiple_workers_run_independently(self, qtbot):
        results = []

        w1 = Worker(successful_task, 1, 1)
        w2 = Worker(successful_task, 2, 2)

        w1.finished_ok.connect(lambda r: results.append(r))
        w2.finished_ok.connect(lambda r: results.append(r))

        w1.start()
        w2.start()

        qtbot.waitUntil(lambda: len(results) == 2, timeout=3000)
        assert sorted(results) == [2, 4]