"""Helpers shared by the end-to-end tests: locating and running the real splinter3D
binary, writing batch scripts, and reading back the STL files it exports.

The binary under test is $SPLINTER3D_BIN when set (e.g. one extracted from a release
package), else the one built in build/bin/."""

from __future__ import annotations

import json
import os
import platform
import struct
import subprocess
import tempfile
import unittest
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
ASSETS = REPO_ROOT / "assets" / "stl"
IS_WINDOWS = platform.system() == "Windows"
IS_LINUX = platform.system() == "Linux"


def binary_path() -> Path:
    override = os.environ.get("SPLINTER3D_BIN")
    if override:
        return Path(override)
    name = "splinter3D.exe" if IS_WINDOWS else "splinter3D"
    for candidate in (REPO_ROOT / "build" / "bin" / name, REPO_ROOT / "build" / "bin" / "Debug" / name,
                      REPO_ROOT / "build" / "bin" / "Release" / name):
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"splinter3D binary not found under build/bin; build it or set SPLINTER3D_BIN")


@dataclass
class Run:
    returncode: int
    stdout: str
    stderr: str


def run_app(*args: str, timeout: float = 60, env: dict[str, str] | None = None, cwd: Path | None = None) -> Run:
    completed = subprocess.run(
        [str(binary_path()), *args],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=timeout,
        env=env,
        cwd=cwd,
    )
    return Run(completed.returncode, completed.stdout, completed.stderr)


# --- STL reading -------------------------------------------------------------------

Vertex = tuple[float, float, float]
Triangle = tuple[Vertex, Vertex, Vertex]


def read_stl(path: Path) -> list[Triangle]:
    """Parses an ASCII or binary STL file into its triangles."""
    data = path.read_bytes()
    # Binary STL: 80-byte header, uint32 count, then 50 bytes per triangle. Some binary
    # files also start with "solid", so trust the size check first.
    if len(data) >= 84:
        (count,) = struct.unpack_from("<I", data, 80)
        if len(data) == 84 + 50 * count:
            triangles = []
            for i in range(count):
                floats = struct.unpack_from("<12f", data, 84 + 50 * i)
                triangles.append((floats[3:6], floats[6:9], floats[9:12]))
            return triangles
    vertices = [
        tuple(float(v) for v in line.split()[1:4])
        for line in data.decode("ascii", errors="replace").splitlines()
        if line.strip().startswith("vertex")
    ]
    if len(vertices) % 3:
        raise ValueError(f"{path}: vertex count {len(vertices)} is not a multiple of 3")
    return [tuple(vertices[i:i + 3]) for i in range(0, len(vertices), 3)]


def bounding_box(triangles: list[Triangle]) -> tuple[Vertex, Vertex]:
    points = [p for triangle in triangles for p in triangle]
    return (
        tuple(min(p[axis] for p in points) for axis in range(3)),
        tuple(max(p[axis] for p in points) for axis in range(3)),
    )


# --- Test base class ---------------------------------------------------------------

class BatchTestCase(unittest.TestCase):
    """Gives each test a scratch folder and a way to run a batch script in it."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory(prefix="splinter3d-e2e-")
        self.tmp = Path(self._tmp.name)

    def tearDown(self):
        self._tmp.cleanup()

    def run_batch(self, steps: list[dict], name: str = "script.json", **kwargs) -> Run:
        script = self.tmp / name
        script.write_text(json.dumps({"steps": steps}), encoding="utf-8")
        return run_app("--batch", str(script), **kwargs)

    def assertBatchOk(self, run: Run):
        self.assertEqual(run.returncode, 0, f"batch failed:\nstdout:\n{run.stdout}\nstderr:\n{run.stderr}")

    def assertBoxAlmostEqual(self, box, expected, places: int = 4):
        for actual_corner, expected_corner in zip(box, expected):
            for actual, wanted in zip(actual_corner, expected_corner):
                self.assertAlmostEqual(actual, wanted, places=places, msg=f"bounding box {box} != {expected}")
