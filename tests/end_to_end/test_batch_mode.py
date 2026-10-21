"""End-to-end: drives the real splinter3D binary through `--batch` scripts and checks
its exit code, its output, and the STL files it writes."""

import os
import shutil
import unittest

from splinter3d_e2e import ASSETS, BatchTestCase, bounding_box, read_stl, run_app

CUBE_ASCII = str(ASSETS / "ascii" / "cube.stl")
CUBE_BINARY = str(ASSETS / "binary" / "cube.stl")
UNIT_CUBE = ((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))


class ImportExport(BatchTestCase):
    def test_ascii_cube_round_trips(self):
        run = self.run_batch([{"open": CUBE_ASCII}, {"export": "out.stl"}])

        self.assertBatchOk(run)
        triangles = read_stl(self.tmp / "out.stl")
        self.assertEqual(len(triangles), 12)
        self.assertBoxAlmostEqual(bounding_box(triangles), UNIT_CUBE)

    def test_binary_cube_round_trips(self):
        run = self.run_batch([{"open": CUBE_BINARY}, {"export": "out.stl"}])

        self.assertBatchOk(run)
        self.assertEqual(len(read_stl(self.tmp / "out.stl")), 12)

    def test_larger_model_keeps_its_triangles(self):
        source = ASSETS / "binary" / "magnolia_binary.stl"
        run = self.run_batch([{"open": str(source)}, {"export": "out.stl"}])

        self.assertBatchOk(run)
        original = read_stl(source)
        exported = read_stl(self.tmp / "out.stl")
        self.assertEqual(len(exported), len(original))
        self.assertBoxAlmostEqual(bounding_box(exported), bounding_box(original), places=3)

    def test_paths_are_relative_to_the_script(self):
        shutil.copy(CUBE_ASCII, self.tmp / "model.stl")
        (self.tmp / "out").mkdir()

        run = self.run_batch([{"open": "model.stl"}, {"export": "out/model.stl"}])

        self.assertBatchOk(run)
        self.assertTrue((self.tmp / "out" / "model.stl").is_file())

    def test_non_ascii_paths(self):
        folder = self.tmp / "modèles 3D"
        folder.mkdir()
        shutil.copy(CUBE_ASCII, folder / "cube é.stl")

        run = self.run_batch([{"open": "modèles 3D/cube é.stl"}, {"export": "modèles 3D/sortie.stl"}],
                             name="scénario.json")

        self.assertBatchOk(run)
        self.assertIn("'cube é.stl'", run.stdout)
        self.assertEqual(len(read_stl(folder / "sortie.stl")), 12)

    def test_clear_empties_the_scene(self):
        run = self.run_batch([{"open": CUBE_ASCII}, {"clear": True}, {"export": "out.stl"}])

        self.assertEqual(run.returncode, 1)
        self.assertIn("no model loaded", run.stderr)
        self.assertFalse((self.tmp / "out.stl").exists())

    def test_reports_each_step(self):
        run = self.run_batch([{"open": CUBE_ASCII}, {"open": CUBE_ASCII}, {"transform": {"target": 1}}])

        self.assertBatchOk(run)
        self.assertIn("step 1/3: open", run.stdout)
        self.assertIn("-> model 1 'cube.stl (2)'", run.stdout)
        self.assertIn("step 3/3: transform 1", run.stdout)
        self.assertIn("done: 3 step(s)", run.stdout)


class ExportPlacement(BatchTestCase):
    """The export contains every model, each where the viewer shows it."""

    def test_export_applies_the_transform(self):
        run = self.run_batch([
            {"open": CUBE_ASCII},
            {"transform": {"move": [10, 20, 30]}},
            {"export": "out.stl"},
        ])

        self.assertBatchOk(run)
        self.assertBoxAlmostEqual(bounding_box(read_stl(self.tmp / "out.stl")),
                                  ((10.0, 20.0, 30.0), (11.0, 21.0, 31.0)))

    def test_export_applies_rotations(self):
        run = self.run_batch([
            {"open": CUBE_ASCII},
            {"transform": {"rotate": [0, 0, 90]}},
            {"export": "out.stl"},
        ])

        self.assertBatchOk(run)
        # The unit cube turned 90 degrees around Z spans x in [-1, 0].
        self.assertBoxAlmostEqual(bounding_box(read_stl(self.tmp / "out.stl")),
                                  ((-1.0, 0.0, 0.0), (0.0, 1.0, 1.0)))

    def test_export_writes_every_model(self):
        run = self.run_batch([
            {"open": CUBE_ASCII},
            {"open": CUBE_ASCII},
            {"transform": {"target": 1, "move": [5, 0, 0]}},
            {"export": "out.stl"},
        ])

        self.assertBatchOk(run)
        triangles = read_stl(self.tmp / "out.stl")
        self.assertEqual(len(triangles), 24)
        self.assertBoxAlmostEqual(bounding_box(triangles), ((0.0, 0.0, 0.0), (6.0, 1.0, 1.0)))


class Failures(BatchTestCase):
    def test_missing_model_exits_1(self):
        run = self.run_batch([{"open": "does_not_exist.stl"}, {"export": "out.stl"}])

        self.assertEqual(run.returncode, 1)
        self.assertIn("step 1/2 (open)", run.stderr)
        self.assertFalse((self.tmp / "out.stl").exists())

    def test_invalid_step_exits_1(self):
        run = self.run_batch([{"open": CUBE_ASCII}, {"transform": {"target": 5}}])

        self.assertEqual(run.returncode, 1)
        self.assertIn("step 2/2 (transform): no such target", run.stderr)

    def test_invalid_json_exits_2(self):
        script = self.tmp / "broken.json"
        script.write_text("{ not json", encoding="utf-8")

        run = run_app("--batch", str(script))

        self.assertEqual(run.returncode, 2)
        self.assertIn("invalid JSON", run.stderr)

    def test_missing_script_exits_2(self):
        run = run_app("--batch", str(self.tmp / "nope.json"))

        self.assertEqual(run.returncode, 2)
        self.assertIn("script not found", run.stderr)

    def test_missing_script_argument_exits_2(self):
        run = run_app("--batch")

        self.assertEqual(run.returncode, 2)
        self.assertIn("usage", run.stderr)


class Headless(BatchTestCase):
    def test_batch_mode_needs_no_display(self):
        # Batch mode runs before wxWidgets starts, so it must work on a server or CI
        # runner with no X display (on Linux, the GUI would refuse to start).
        env = {key: value for key, value in os.environ.items() if key not in ("DISPLAY", "WAYLAND_DISPLAY")}

        run = self.run_batch([{"open": CUBE_ASCII}, {"export": "out.stl"}], env=env)

        self.assertBatchOk(run)


if __name__ == "__main__":
    unittest.main()
