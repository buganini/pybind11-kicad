from pathlib import Path
import unittest

import pybind11_kicad as kk
import pcbnew


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = Path(__file__).parent / "golden" / "simple_board.kicad_pcb"
TMP_DIR = ROOT / "tmp"


class PcbnewCompatTests(unittest.TestCase):
    def test_pcbnew_load_behavior_matches_backend_mode(self):
        if kk.backend_version() == "kicad-10.0.4-native-pybind11-kicad-0.1":
            board = pcbnew.LoadBoard(FIXTURE)
            self.assertEqual(len(board.GetFootprints()), 1)

            TMP_DIR.mkdir(parents=True, exist_ok=True)
            output = TMP_DIR / "pybind11-kicad-pcbnew-test-roundtrip.kicad_pcb"
            board.Save(output)
            self.assertEqual(len(pcbnew.LoadBoard(output).GetFootprints()), 1)
            return

        with self.assertRaises(Exception) as raised:
            pcbnew.LoadBoard(FIXTURE)

        message = str(raised.exception).lower()
        self.assertIn("native", message)
        self.assertTrue(
            "alternate board-file implementation" in message
            or "board io" in message
        )

    def test_pcbnew_units_and_metadata(self):
        self.assertEqual(pcbnew.ToMM(pcbnew.FromMM(1.0)), 1.0)
        self.assertEqual(pcbnew.FromMils(1), 25_400)
        self.assertEqual(pcbnew.ToMils(25_400), 1.0)
        self.assertEqual(pcbnew.PCB_IU_PER_MM, 1_000_000.0)
        self.assertEqual(pcbnew.PAD_ATTRIB_PTH, 0)
        self.assertEqual(pcbnew.PAD_ATTRIB_SMD, 1)
        self.assertEqual(pcbnew.PAD_ATTRIB_CONN, 2)
        self.assertEqual(pcbnew.PAD_ATTRIB_NPTH, 3)
        self.assertEqual(pcbnew.PAD_SHAPE_OVAL, 2)
        self.assertEqual(pcbnew.PAD_DRILL_SHAPE_CIRCLE, 1)
        self.assertEqual(pcbnew.PAD_DRILL_SHAPE_OBLONG, 2)
        self.assertEqual(pcbnew.VIATYPE_THROUGH, 4)
        self.assertEqual(pcbnew.ZONE_FILL_MODE_HATCH_PATTERN, 1)
        self.assertEqual(pcbnew.FP_EXCLUDE_FROM_POS_FILES, 4)
        self.assertEqual(pcbnew.In1_Cu, 4)
        self.assertEqual(pcbnew.In30_Cu, 62)
        self.assertEqual(pcbnew.GetMajorMinorVersion(), "10.0")
        self.assertEqual(pcbnew.CompatibilityLevel(), "partial-pcbnew-v10")
        self.assertIn("pybind11-kicad pcbnew compatibility layer", pcbnew.GetBuildVersion())

    def test_pcbnew_value_types_for_kikit_unit_tests(self):
        angle = pcbnew.EDA_ANGLE(180, pcbnew.DEGREES_T)
        self.assertEqual(angle.AsDegrees(), 180)
        self.assertAlmostEqual(pcbnew.EDA_ANGLE(3.141592653589793, pcbnew.RADIANS_T).AsDegrees(), 180)
        self.assertEqual(pcbnew.EDA_ANGLE(900, pcbnew.TENTHS_OF_A_DEGREE_T).AsDegrees(), 90)
        self.assertEqual((2 * pcbnew.EDA_ANGLE(15, pcbnew.DEGREES_T)).AsDegrees(), 30)

        origin = pcbnew.VECTOR2I(10, 20)
        size = pcbnew.VECTOR2I(30, 40)
        box = pcbnew.BOX2I(origin, size)
        self.assertEqual(origin + size, pcbnew.VECTOR2I(40, 60))
        self.assertEqual(size - origin, pcbnew.VECTOR2I(20, 20))
        self.assertEqual(origin[0], 10)
        self.assertEqual(tuple(origin), (10, 20))
        self.assertEqual(box.GetX(), 10)
        self.assertEqual(box.GetY(), 20)
        self.assertEqual(box.GetWidth(), 30)
        self.assertEqual(box.GetHeight(), 40)
        self.assertEqual(box.GetEnd(), pcbnew.VECTOR2I(40, 60))

        segment = pcbnew.PCB_SHAPE()
        segment.SetShape(pcbnew.S_SEGMENT)
        segment.SetStart(pcbnew.VECTOR2I(1000, 0))
        segment.SetEnd(pcbnew.VECTOR2I(2000, 0))
        segment.Rotate(pcbnew.VECTOR2I(0, 0), pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
        self.assertEqual(segment.GetStart(), pcbnew.VECTOR2I(0, -1000))
        self.assertEqual(segment.GetEnd(), pcbnew.VECTOR2I(0, -2000))

        arc = pcbnew.PCB_SHAPE()
        arc.SetShape(pcbnew.S_ARC)
        arc.SetArcGeometry(
            pcbnew.VECTOR2I(10, 0),
            pcbnew.VECTOR2I(0, 10),
            pcbnew.VECTOR2I(-10, 0),
        )
        start_angle = pcbnew.EDA_ANGLE()
        end_angle = pcbnew.EDA_ANGLE()
        arc.CalcArcAngles(start_angle, end_angle)
        self.assertEqual(arc.GetCenter(), pcbnew.VECTOR2I(0, 0))
        self.assertEqual(arc.GetRadius(), 10)
        self.assertEqual(start_angle.AsDegrees(), 0)
        self.assertEqual(end_angle.AsDegrees(), 180)

        arc.Rotate(pcbnew.VECTOR2I(0, 0), pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
        arc.CalcArcAngles(start_angle, end_angle)
        self.assertEqual(arc.GetStart(), pcbnew.VECTOR2I(0, -10))
        self.assertEqual(arc.GetEnd(), pcbnew.VECTOR2I(0, 10))
        self.assertEqual(start_angle.AsDegrees(), -90)
        self.assertEqual(end_angle.AsDegrees(), 90)

        constructed_arc = pcbnew.PCB_SHAPE()
        constructed_arc.SetShape(pcbnew.S_ARC)
        constructed_arc.SetStart(pcbnew.VECTOR2I(10, 0))
        constructed_arc.SetCenter(pcbnew.VECTOR2I(0, 0))
        constructed_arc.SetArcAngleAndEnd(pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
        constructed_arc.CalcArcAngles(start_angle, end_angle)
        self.assertEqual(constructed_arc.GetEnd(), pcbnew.VECTOR2I(0, 10))
        self.assertEqual(constructed_arc.GetRadius(), 10)
        self.assertEqual(start_angle.AsDegrees(), 0)
        self.assertEqual(end_angle.AsDegrees(), 90)

        poly = pcbnew.PCB_SHAPE()
        poly.SetShape(pcbnew.S_POLYGON)
        poly.SetFilled(True)
        outline = poly.GetPolyShape().NewOutline()
        poly.GetPolyShape().Append(0, 0, outline)
        poly.GetPolyShape().Append(10, 0, outline)
        poly.GetPolyShape().Append(10, 20, outline)
        self.assertTrue(poly.IsSolidFill())
        self.assertEqual(poly.GetBoundingBox(), pcbnew.BOX2I(pcbnew.VECTOR2I(0, 0), pcbnew.VECTOR2I(10, 20)))

        rect = pcbnew.PCB_SHAPE()
        rect.SetShape(pcbnew.S_RECT)
        rect.SetStart(pcbnew.VECTOR2I(1000, 1000))
        rect.SetEnd(pcbnew.VECTOR2I(3000, 4000))
        rect.SetWidth(200)
        self.assertEqual(rect.GetBoundingBox(), pcbnew.BOX2I(pcbnew.VECTOR2I(900, 900), pcbnew.VECTOR2I(2200, 3200)))
        rect.SetWidth(0)
        self.assertEqual(rect.GetBoundingBox(), pcbnew.BOX2I(pcbnew.VECTOR2I(1000, 1000), pcbnew.VECTOR2I(2000, 3000)))
        rect.SetWidth(200)
        self.assertEqual(rect.GetBoundingBox(), pcbnew.BOX2I(pcbnew.VECTOR2I(900, 900), pcbnew.VECTOR2I(2200, 3200)))

        via = pcbnew.PCB_VIA(pcbnew.BOARD())
        via.SetViaType(pcbnew.VIATYPE_THROUGH)
        self.assertEqual(via.GetViaType(), pcbnew.VIATYPE_THROUGH)

        footprint = pcbnew.FootprintLoad("", "NPTH")
        footprint.SetOrientation(pcbnew.EDA_ANGLE(15, pcbnew.DEGREES_T))
        self.assertEqual(footprint.GetOrientation().AsDegrees(), 15)
        pad = footprint.Pads()[0]
        pad.SetShape(pcbnew.PAD_SHAPE_OVAL)
        pad.SetDrillShape(pcbnew.PAD_DRILL_SHAPE_OBLONG)
        self.assertEqual(pad.GetDrillShape(), pcbnew.PAD_DRILL_SHAPE_OBLONG)

        component = pcbnew.FootprintLoad("", "Footprint")
        component.SetFPIDAsString("Resistor_SMD:R_0603_1608Metric")
        component.SetLayer(pcbnew.B_Cu)
        component.SetField("LCSC", "C123")
        component.GetFieldByName("LCSC").SetVisible(False)
        component.Reference().SetVisible(True)
        component.Value().SetVisible(False)
        component.Value().SetLayer(pcbnew.F_Fab)
        component.SetField("Value", "22k")
        self.assertEqual(component.GetFPIDAsString(), "Resistor_SMD:R_0603_1608Metric")
        self.assertEqual(component.GetLayer(), pcbnew.B_Cu)
        self.assertTrue(component.HasField("Value"))
        self.assertIs(component.GetFieldByName("Value"), component.Value())
        self.assertEqual(component.GetValue(), "22k")
        self.assertFalse(component.Value().IsVisible())
        self.assertEqual(component.Value().GetLayer(), pcbnew.F_Fab)
        self.assertEqual(component.GetFieldText("LCSC"), "C123")
        self.assertFalse(component.GetFieldByName("LCSC").IsVisible())

        oriented = pcbnew.FOOTPRINT(None)
        oriented.SetPosition(pcbnew.VECTOR2I(0, 0))
        oriented.Value().SetPosition(pcbnew.VECTOR2I(0, pcbnew.FromMM(1)))
        oriented.SetField("LCSC", "C2843970")
        oriented.GetFieldByName("LCSC").SetPosition(pcbnew.VECTOR2I(0, pcbnew.FromMM(2)))
        oriented.SetOrientation(pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
        self.assertEqual(oriented.Value().GetPosition(), pcbnew.VECTOR2I(pcbnew.FromMM(1), 0))
        self.assertEqual(oriented.GetFieldByName("LCSC").GetName(), "LCSC")
        self.assertEqual(oriented.GetFieldByName("LCSC").GetTextSize(), pcbnew.VECTOR2I(pcbnew.FromMM(1.27), pcbnew.FromMM(1.27)))
        self.assertEqual(oriented.GetFieldByName("LCSC").GetPosition(), pcbnew.VECTOR2I(pcbnew.FromMM(2), 0))
        self.assertEqual(oriented.GetFieldByName("LCSC").GetTextAngle().AsDegrees(), 90)

    def test_pcbnew_shape_remove_and_empty_zone_fill_for_kikit_save(self):
        if kk.backend_version() != "kicad-10.0.4-native-pybind11-kicad-0.1":
            self.skipTest("native board IO is not enabled")

        board = pcbnew.BOARD()
        self.assertEqual(board.GetFileName(), "")

        board = pcbnew.NewBoard()
        edge = pcbnew.PCB_SHAPE()
        edge.SetShape(pcbnew.S_SEGMENT)
        edge.SetLayer(pcbnew.Edge_Cuts)
        edge.SetWidth(pcbnew.FromMM(0.1))
        edge.SetStart(pcbnew.VECTOR2I(0, 0))
        edge.SetEnd(pcbnew.VECTOR2I(pcbnew.FromMM(10), 0))

        board.Add(edge)
        self.assertEqual(len(board.GetDrawings()), 1)

        board.Remove(edge)
        self.assertEqual(len(board.GetDrawings()), 0)

        board.Remove(edge)
        pcbnew.ZONE_FILLER(board).Fill(board.Zones())
        with self.assertRaisesRegex(NotImplementedError, "empty-zone no-op"):
            pcbnew.ZONE_FILLER(board).Fill([object()])

        label = pcbnew.PCB_TEXT(board)
        label.SetText("V-CUT 10.00 mm")
        label.SetLayer(pcbnew.Cmts_User)
        label.SetPosition(pcbnew.VECTOR2I(0, pcbnew.FromMM(1)))
        label.SetTextSize(pcbnew.VECTOR2I(pcbnew.FromMM(2), pcbnew.FromMM(2)))
        label.SetTextThickness(pcbnew.FromMM(0.4))
        label.SetTextAngle(pcbnew.EDA_ANGLE(90, pcbnew.DEGREES_T))
        board.Add(label)
        board.Remove(label)

        hole = pcbnew.FootprintLoad("", "NPTH")
        hole.SetReference("KiKit_MB_test")
        hole.SetValue("mousebite")
        hole.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(1), pcbnew.FromMM(2)))
        hole.SetBoardOnly(True)
        hole.SetExcludedFromPosFiles(True)
        hole.SetExcludedFromBOM(True)
        self.assertFalse(hole.Reference().IsVisible())
        self.assertFalse(hole.Value().IsVisible())
        self.assertTrue(all(pad.GetAttribute() == pcbnew.PAD_ATTRIB_NPTH for pad in hole.Pads()))
        board.Add(hole)

        TMP_DIR.mkdir(parents=True, exist_ok=True)
        output = TMP_DIR / "pybind11-kicad-npth-hidden-text.kicad_pcb"
        board.Save(output)
        saved = output.read_text()

        def property_block(name, value):
            marker = f'(property "{name}" "{value}"'
            start = saved.index(marker)
            next_property = saved.find("\n\t\t(property ", start + len(marker))
            end = saved.find("\n\t\t(duplicate_pad_numbers", start)
            candidates = [index for index in (next_property, end) if index != -1]
            return saved[start:min(candidates)]

        self.assertIn("(hide yes)", property_block("Reference", "KiKit_MB_test"))
        self.assertIn("(hide yes)", property_block("Value", "mousebite"))
        attr_line = next(line for line in saved.splitlines() if line.strip().startswith("(attr "))
        self.assertIn("board_only", attr_line)
        self.assertIn("exclude_from_pos_files", attr_line)
        self.assertIn("exclude_from_bom", attr_line)
        self.assertIn('(pad "" np_thru_hole circle', saved)

        component = pcbnew.FootprintLoad("", "Footprint")
        component.SetReference("R1")
        component.SetValue("10k")
        component.SetFPIDAsString("Resistor_SMD:R_0603_1608Metric")
        component.SetLayer(pcbnew.B_Cu)
        component.SetPosition(pcbnew.VECTOR2I(pcbnew.FromMM(3), pcbnew.FromMM(4)))
        component.SetOrientation(pcbnew.EDA_ANGLE(45, pcbnew.DEGREES_T))
        component.SetField("LCSC", "C123")
        component.GetFieldByName("LCSC").SetVisible(False)
        board.Add(component)

        output = TMP_DIR / "pybind11-kicad-component-footprint.kicad_pcb"
        board.Save(output)
        saved = output.read_text()
        self.assertIn('(footprint "Resistor_SMD:R_0603_1608Metric"', saved)
        self.assertIn('(property "Reference" "R1"', saved)
        self.assertIn('(property "LCSC" "C123"', saved)

    def test_pcbnew_zone_fills_survive_add_save(self):
        if kk.backend_version() != "kicad-10.0.4-native-pybind11-kicad-0.1":
            self.skipTest("native board IO is not enabled")

        def poly_set(points):
            chain = pcbnew.SHAPE_LINE_CHAIN([pcbnew.VECTOR2I(x, y) for x, y in points])
            chain.SetClosed(True)
            poly = pcbnew.SHAPE_POLY_SET()
            poly.AddOutline(chain)
            return poly

        board = pcbnew.NewBoard()
        zone = pcbnew.ZONE(board)
        zone.SetLayerSet(pcbnew.LSET([pcbnew.F_Cu]))
        zone.Outline().AddOutline(
            pcbnew.SHAPE_LINE_CHAIN([
                pcbnew.VECTOR2I(0, 0),
                pcbnew.VECTOR2I(pcbnew.FromMM(10), 0),
                pcbnew.VECTOR2I(pcbnew.FromMM(10), pcbnew.FromMM(10)),
                pcbnew.VECTOR2I(0, pcbnew.FromMM(10)),
            ])
        )
        zone._is_filled = True
        zone._fills = {
            pcbnew.F_Cu: poly_set([
                (pcbnew.FromMM(1), pcbnew.FromMM(1)),
                (pcbnew.FromMM(9), pcbnew.FromMM(1)),
                (pcbnew.FromMM(9), pcbnew.FromMM(9)),
                (pcbnew.FromMM(1), pcbnew.FromMM(9)),
            ])
        }
        board.Add(zone)

        TMP_DIR.mkdir(parents=True, exist_ok=True)
        output = TMP_DIR / "pybind11-kicad-zone-fill-roundtrip.kicad_pcb"
        board.Save(output)
        saved = output.read_text()
        self.assertIn("(fill yes", saved)
        self.assertIn("(filled_polygon", saved)

        [saved_zone] = kk.Board.open(output).zones()
        self.assertTrue(saved_zone.is_filled)
        self.assertEqual([(fill.layer, len(fill.polygons)) for fill in saved_zone.fills], [(pcbnew.F_Cu, 1)])

    def test_unsupported_gui_calls_fail_clearly(self):
        with self.assertRaisesRegex(NotImplementedError, "GUI/editor state"):
            pcbnew.GetPcbFrame()


if __name__ == "__main__":
    unittest.main()
