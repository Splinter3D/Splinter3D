"""End-to-end smoke tests of the interactive application: it must start and keep running
(no crash during startup, asset lookup, OpenGL/OCCT initialization...), and quit cleanly
when asked to by Ctrl+C or SIGTERM."""

import os
import signal
import subprocess
import time
import unittest

from splinter3d_e2e import IS_LINUX, IS_WINDOWS, binary_path

STARTUP_SECONDS = 8
SHUTDOWN_SECONDS = 10


@unittest.skipIf(IS_LINUX and not os.environ.get("DISPLAY"), "needs a display (run under xvfb-run)")
class GuiStartup(unittest.TestCase):
    def start(self) -> subprocess.Popen:
        process = subprocess.Popen([str(binary_path())], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.addCleanup(self.stop, process)
        time.sleep(STARTUP_SECONDS)
        exited = process.poll()
        if exited is not None:
            self.fail(f"splinter3D exited during startup with code {exited}:\n{self.output(process)}")
        return process

    @staticmethod
    def output(process: subprocess.Popen) -> str:
        return process.stdout.read().decode("utf-8", errors="replace") if process.stdout else ""

    @staticmethod
    def stop(process: subprocess.Popen):
        if process.poll() is None:
            process.kill()
            process.wait()
        if process.stdout:
            process.stdout.close()

    def assertQuitsCleanly(self, process: subprocess.Popen, sig: int):
        process.send_signal(sig)
        try:
            code = process.wait(timeout=SHUTDOWN_SECONDS)
        except subprocess.TimeoutExpired:
            self.fail(f"splinter3D ignored signal {sig} for {SHUTDOWN_SECONDS}s")
        self.assertEqual(code, 0, f"unclean exit after signal {sig}:\n{self.output(process)}")

    def test_starts_and_stays_up(self):
        self.start()

    @unittest.skipIf(IS_WINDOWS, "POSIX signals")
    def test_sigterm_quits_cleanly(self):
        self.assertQuitsCleanly(self.start(), signal.SIGTERM)

    @unittest.skipIf(IS_WINDOWS, "POSIX signals")
    def test_ctrl_c_quits_cleanly(self):
        self.assertQuitsCleanly(self.start(), signal.SIGINT)


if __name__ == "__main__":
    unittest.main()
