#!/usr/bin/env python3
"""
Drehfussball Controller v1 - PCB generator.

Builds a KiCad 9 board (2 layers, THT) for:
  - ESP32-DevKitC V4 (38 pin) socket
  - A4988 / DRV8825 stepper driver socket
  - 4x IR light barrier inputs (dual beam per goal)
  - 2x WS2812B strips (5 V, level shifted)
  - 12-24 V input, 5 V 2 A switching regulator
  - reset button

Run with the locally extracted KiCad (see hardware/README.md):
  export LD_LIBRARY_PATH=/tmp/opencode/kicad/root/usr/lib/x86_64-linux-gnu
  export PYTHONPATH=/tmp/opencode/kicad/root/usr/lib/python3/dist-packages
  python3 generate_board.py
"""

from __future__ import annotations

import csv
import os
import sys
from pathlib import Path

import pcbnew

HERE = Path(__file__).resolve().parent
KICAD_ROOT = Path(os.environ.get("KICAD_ROOT", "/tmp/opencode/kicad/root"))
FP_LIB = KICAD_ROOT / "usr/share/kicad/footprints"
BOARD_PATH = HERE / "Drehfussball_Controller.kicad_pcb"
BOM_PATH = HERE / "BOM.csv"

W = 110.0
H = 100.0

MM = pcbnew.FromMM


def v(x: float, y: float) -> pcbnew.VECTOR2I:
    return pcbnew.VECTOR2I(MM(x), MM(y))


# ---------------------------------------------------------------------------
# Footprints
# ---------------------------------------------------------------------------

FPS = {
    "r": ("Resistor_THT.pretty", "R_Axial_DIN0207_L6.3mm_D2.5mm_P10.16mm_Horizontal"),
    "c": ("Capacitor_THT.pretty", "C_Disc_D5.0mm_W2.5mm_P5.00mm"),
    "cp10": ("Capacitor_THT.pretty", "CP_Radial_D10.0mm_P5.00mm"),
    "cp8": ("Capacitor_THT.pretty", "CP_Radial_D8.0mm_P3.50mm"),
    "led": ("LED_THT.pretty", "LED_D5.0mm"),
    "d201": ("Diode_THT.pretty", "D_DO-201_P15.24mm_Horizontal"),
    "d15": ("Diode_THT.pretty", "D_DO-15_P10.16mm_Horizontal"),
    "dip14": ("Package_DIP.pretty", "DIP-14_W7.62mm"),
    "to220": ("Package_TO_SOT_THT.pretty", "TO-220-3_Vertical"),
    "sw": ("Button_Switch_THT.pretty", "SW_PUSH_6mm"),
    "fuse": ("Fuse.pretty", "Fuseholder_Cylinder-5x20mm_Schurter_0031_8201_Horizontal_Open"),
    "tb2": ("TerminalBlock.pretty", "TerminalBlock_bornier-2_P5.08mm"),
    "tb3": ("TerminalBlock.pretty", "TerminalBlock_bornier-3_P5.08mm"),
    "tb4": ("TerminalBlock.pretty", "TerminalBlock_bornier-4_P5.08mm"),
    "tb3s": ("TerminalBlock_4Ucon.pretty", "TerminalBlock_4Ucon_1x03_P3.50mm_Horizontal"),
    "tb4s": ("TerminalBlock_4Ucon.pretty", "TerminalBlock_4Ucon_1x04_P3.50mm_Horizontal"),
    "s19": ("Connector_PinSocket_2.54mm.pretty", "PinSocket_1x19_P2.54mm_Vertical"),
    "s8": ("Connector_PinSocket_2.54mm.pretty", "PinSocket_1x08_P2.54mm_Vertical"),
    "s3": ("Connector_PinSocket_2.54mm.pretty", "PinSocket_1x03_P2.54mm_Vertical"),
    "h2": ("Connector_PinHeader_2.54mm.pretty", "PinHeader_1x02_P2.54mm_Vertical"),
    "h3": ("Connector_PinHeader_2.54mm.pretty", "PinHeader_1x03_P2.54mm_Vertical"),
    "mh": ("MountingHole.pretty", "MountingHole_3.2mm_M3"),
}

# reference, footprint key, x, y, rotation, value, description
PLACEMENTS = [
    # --- power input / regulator -------------------------------------------
    ("J1", "tb2", 9, 10, 0, "12-24V IN", "Screw terminal 12-24 V input"),
    ("F1", "fuse", 20, 10, 0, "FUSE 5x20 3A", "5x20 mm fuse holder"),
    ("D1", "d201", 60, 10, 180, "SR560", "Reverse polarity protection"),
    ("D2", "d15", 70, 10, 0, "P6KE33A", "TVS transient protection"),
    ("C2", "c", 84, 10, 0, "100nF", "Input HF decoupling"),
    ("C1", "cp10", 94, 20, 0, "470uF/50V", "Input bulk capacitor"),
    ("U1", "to220", 102, 14, 0, "TSR2-2450", "5 V 2 A switching regulator"),
    ("C3", "cp10", 94, 32, 0, "470uF/10V", "5 V bulk capacitor"),
    ("C4", "c", 103, 32, 0, "100nF", "5 V HF decoupling"),
    ("JP1", "h3", 92, 40, 90, "5V SEL", "5 V source select: 1-2 internal, 2-3 external"),
    ("J2", "h2", 102, 40, 90, "5V EXT", "External 5 V input"),
    ("R1", "r", 74, 28, 0, "1k", "Power LED resistor"),
    ("LED1", "led", 74, 34, 0, "GREEN", "Power LED"),

    # --- ESP32 --------------------------------------------------------------
    ("J12", "s19", 12, 38, 90, "ESP32 J2", "ESP32-DevKitC J2 socket"),
    ("J13", "s19", 12, 63.4, 90, "ESP32 J3", "ESP32-DevKitC J3 socket"),
    ("C5", "c", 16, 34, 0, "100nF", "3V3 decoupling"),

    # --- stepper driver -----------------------------------------------------
    ("J3", "s8", 86, 46, 90, "A4988 TOP", "A4988/DRV8825 left row"),
    ("J4", "s8", 86, 61.24, 90, "A4988 BOT", "A4988/DRV8825 right row"),
    ("C14", "cp8", 81, 64, 0, "100uF/50V", "Stepper VMOT bulk capacitor"),
    ("C13", "c", 106, 70, 90, "100nF", "Stepper VDD decoupling"),
    ("JP3", "h2", 76, 46, 90, "MS1", "Microstep MS1 to +5 V"),
    ("JP4", "h2", 76, 51, 90, "MS2", "Microstep MS2 to +5 V"),
    ("JP5", "h2", 76, 56, 90, "MS3", "Microstep MS3 to +5 V"),
    ("J5", "tb4s", 90, 78, 0, "MOTOR", "Stepper motor 1A 1B 2A 2B"),

    # --- sensor front end ---------------------------------------------------
    ("U2", "dip14", 64, 76, 0, "74HC14", "Schmitt trigger, inverting"),
    ("J6", "s3", 10, 94, 90, "S1", "Goal 1 beam A"),
    ("J7", "s3", 24, 94, 90, "S2", "Goal 1 beam B"),
    ("J8", "s3", 38, 94, 90, "S3", "Goal 2 beam A"),
    ("J9", "s3", 52, 94, 90, "S4", "Goal 2 beam B"),
    ("R2", "r", 8, 87, 0, "10k", "S1 pull-up"),
    ("R3", "r", 22, 87, 0, "10k", "S2 pull-up"),
    ("R4", "r", 36, 87, 0, "10k", "S3 pull-up"),
    ("R5", "r", 50, 87, 0, "10k", "S4 pull-up"),
    ("R6", "r", 8, 82, 0, "1k", "S1 series"),
    ("R7", "r", 22, 82, 0, "1k", "S2 series"),
    ("R8", "r", 36, 82, 0, "1k", "S3 series"),
    ("R9", "r", 50, 82, 0, "1k", "S4 series"),
    ("C6", "c", 18, 76, 0, "100nF", "S1 filter"),
    ("C7", "c", 32, 76, 0, "100nF", "S2 filter"),
    ("C8", "c", 46, 76, 0, "100nF", "S3 filter"),
    ("C9", "c", 54, 76, 0, "100nF", "S4 filter"),
    ("C11", "c", 66, 64, 0, "100nF", "74HC14 decoupling"),

    # --- buttons / LED output ----------------------------------------------
    ("SW1", "sw", 34, 67.5, 0, "RESET", "Reset button"),
    ("R10", "r", 44, 67.5, 0, "10k", "Reset pull-up"),
    ("C10", "c", 58, 69, 0, "100nF", "Reset debounce"),
    ("U3", "dip14", 78, 72, 0, "74AHCT125", "WS2812 level shifter"),
    ("C12", "c", 72, 66, 0, "100nF", "74AHCT125 decoupling"),
    ("R14", "r", 80, 90, 0, "330R", "Blue LED data series"),
    ("R15", "r", 92, 90, 0, "330R", "Red LED data series"),
    ("J10", "tb3s", 78, 96, 0, "LED BLUE", "WS2812 blue strip"),
    ("J11", "tb3s", 92, 96, 0, "LED RED", "WS2812 red strip"),
    ("C15", "cp10", 68, 48, 0, "1000uF/10V", "LED strip bulk capacitor"),

    # --- mechanical ---------------------------------------------------------
    ("H1", "mh", 3, 3, 0, "M3", "Mounting hole"),
    ("H2", "mh", 107, 3, 0, "M3", "Mounting hole"),
    ("H3", "mh", 3, 97, 0, "M3", "Mounting hole"),
    ("H4", "mh", 107, 97, 0, "M3", "Mounting hole"),
]

# net name -> list of (reference, pad number)
NETS: dict[str, list[tuple[str, str]]] = {
    "VIN": [("J1", "1"), ("F1", "1")],
    "VIN_F": [("F1", "2"), ("D1", "2")],
    "VIN_PROT": [
        ("D1", "1"), ("D2", "1"), ("C2", "1"), ("C1", "1"), ("U1", "1"),
        ("J4", "1"), ("C14", "1"),
    ],
    "GND": [
        ("J1", "2"), ("D2", "2"), ("C2", "2"), ("C1", "2"), ("U1", "2"),
        ("C3", "2"), ("C4", "2"), ("LED1", "1"), ("J2", "2"),
        ("J4", "2"), ("J4", "8"), ("C13", "2"), ("C14", "2"),
        ("J12", "14"), ("J13", "1"), ("J13", "7"),
        ("U2", "7"), ("C5", "2"), ("C6", "2"), ("C7", "2"), ("C8", "2"),
        ("C9", "2"), ("C10", "2"), ("C11", "2"), ("C12", "2"), ("U3", "7"),
        ("J6", "3"), ("J7", "3"), ("J8", "3"), ("J9", "3"),
        ("J10", "3"), ("J11", "3"), ("C15", "2"), ("SW1", "2"),
        ("U3", "1"), ("U3", "4"), ("U3", "9"), ("U3", "12"),
        ("U2", "11"), ("U2", "13"),
    ],
    "5V_BUCK": [("U1", "3"), ("C3", "1"), ("C4", "1"), ("JP1", "1")],
    "+5V": [
        ("JP1", "2"), ("J12", "19"), ("J4", "7"), ("C13", "1"), ("U3", "14"),
        ("R1", "1"), ("J10", "1"), ("J11", "1"), ("C15", "1"), ("JP2", "3"),
        ("JP3", "1"), ("JP4", "1"), ("JP5", "1"), ("J3", "5"), ("J3", "6"),
        ("U3", "10"), ("U3", "13"),
    ],
    "5V_EXT": [("J2", "1"), ("JP1", "3")],
    "+3V3": [
        ("J12", "1"), ("C5", "1"), ("R2", "1"), ("R3", "1"), ("R4", "1"),
        ("R5", "1"), ("U2", "14"), ("R10", "1"), ("C11", "1"), ("JP2", "1"),
    ],
    "SENSOR_VCC": [("JP2", "2"), ("J6", "1"), ("J7", "1"), ("J8", "1"), ("J9", "1")],
    "S1_RAW": [("J6", "2"), ("R2", "2"), ("R6", "1")],
    "S1_F": [("R6", "2"), ("C6", "1"), ("U2", "1")],
    "S1_OUT": [("U2", "2"), ("J12", "11")],
    "S2_RAW": [("J7", "2"), ("R3", "2"), ("R7", "1")],
    "S2_F": [("R7", "2"), ("C7", "1"), ("U2", "3")],
    "S2_OUT": [("U2", "4"), ("J12", "12")],
    "S3_RAW": [("J8", "2"), ("R4", "2"), ("R8", "1")],
    "S3_F": [("R8", "2"), ("C8", "1"), ("U2", "5")],
    "S3_OUT": [("U2", "6"), ("J12", "7")],
    "S4_RAW": [("J9", "2"), ("R5", "2"), ("R9", "1")],
    "S4_F": [("R9", "2"), ("C9", "1"), ("U2", "9")],
    "S4_OUT": [("U2", "8"), ("J12", "15")],
    "BTN": [("SW1", "1"), ("R10", "2"), ("C10", "1"), ("J12", "8")],
    "LED_BLUE_DIN": [("J12", "10"), ("U3", "2")],
    "LED_BLUE": [("U3", "3"), ("R14", "1")],
    "LED_BLUE_STRIP": [("R14", "2"), ("J10", "2")],
    "LED_RED_DIN": [("J12", "9"), ("U3", "5")],
    "LED_RED": [("U3", "6"), ("R15", "1")],
    "LED_RED_STRIP": [("R15", "2"), ("J11", "2")],
    "LED_A": [("R1", "2"), ("LED1", "2")],
    "STEP": [("J3", "7"), ("J13", "12")],
    "DIR": [("J3", "8"), ("J13", "6")],
    "EN": [("J3", "1"), ("J13", "11")],
    "MS1": [("J3", "2"), ("JP3", "2")],
    "MS2": [("J3", "3"), ("JP4", "2")],
    "MS3": [("J3", "4"), ("JP5", "2")],
    "MOT_1A": [("J4", "5"), ("J5", "1")],
    "MOT_1B": [("J4", "6"), ("J5", "2")],
    "MOT_2A": [("J4", "4"), ("J5", "3")],
    "MOT_2B": [("J4", "3"), ("J5", "4")],
}

# JP2 is created automatically below, but it is referenced in NETS.
# Add it to PLACEMENTS at import time so the net table stays readable.
PLACEMENTS.append(("JP2", "h3", 4, 72, 90, "SENSOR VCC", "1-2 = 3V3, 2-3 = 5V"))

POWER_NETS = {
    "VIN", "VIN_F", "VIN_PROT", "+5V", "5V_BUCK", "5V_EXT", "+3V3",
    "GND", "SENSOR_VCC", "MOT_1A", "MOT_1B", "MOT_2A", "MOT_2B",
}


def build_board() -> pcbnew.BOARD:
    board = pcbnew.BOARD()
    board.SetCopperLayerCount(2)

    ds = board.GetDesignSettings()
    ds.m_MinClearance = MM(0.20)
    ds.m_TrackMinWidth = MM(0.20)
    ds.m_ViasMinSize = MM(0.60)
    ds.m_ViasMinDrill = MM(0.30)
    ds.m_HoleClearance = MM(0.25)
    ds.m_HoleToHoleMin = MM(0.25)

    footprints: dict[str, pcbnew.FOOTPRINT] = {}
    for ref, key, x, y, rot, value, _desc in PLACEMENTS:
        lib, name = FPS[key]
        fp = pcbnew.FootprintLoad(str(FP_LIB / lib), name)
        if fp is None:
            raise RuntimeError(f"footprint not found: {lib}/{name}")
        fp.SetReference(ref)
        fp.SetValue(value)
        fp.Value().SetVisible(False)
        fp.Reference().SetTextSize(pcbnew.VECTOR2I(MM(0.8), MM(0.8)))
        fp.Reference().SetTextThickness(MM(0.12))
        fp.SetPosition(v(x, y))
        fp.SetOrientationDegrees(rot)
        board.Add(fp)
        footprints[ref] = fp

    nets: dict[str, pcbnew.NETINFO_ITEM] = {}
    for name in NETS:
        net = pcbnew.NETINFO_ITEM(board, name)
        board.Add(net)
        nets[name] = net

    for net_name, pins in NETS.items():
        net = nets[net_name]
        for ref, pad_no in pins:
            fp = footprints[ref]
            found = False
            for pad in fp.Pads():
                if pad.GetNumber() == pad_no:
                    pad.SetNet(net)
                    found = True
            if not found:
                raise RuntimeError(f"pad {ref}.{pad_no} not found")
    board.BuildListOfNets()
    board.BuildConnectivity()
    return board


def add_outline(board: pcbnew.BOARD) -> None:
    pts = [(0, 0), (W, 0), (W, H), (0, H), (0, 0)]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        seg = pcbnew.PCB_SHAPE(board)
        seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
        seg.SetStart(v(x1, y1))
        seg.SetEnd(v(x2, y2))
        seg.SetLayer(pcbnew.Edge_Cuts)
        seg.SetWidth(MM(0.15))
        board.Add(seg)


def add_text(board: pcbnew.BOARD, text: str, x: float, y: float,
             size: float = 1.5, layer: int = pcbnew.F_SilkS,
             angle: float = 0.0) -> None:
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetPosition(v(x, y))
    t.SetLayer(layer)
    t.SetTextSize(pcbnew.VECTOR2I(MM(size), MM(size)))
    t.SetTextThickness(MM(max(0.15, size * 0.15)))
    t.SetTextAngleDegrees(angle)
    board.Add(t)


def add_silkscreen(board: pcbnew.BOARD) -> None:
    add_text(board, "DREHFUSSBALL CONTROLLER v1.0", 55, 2.2, 2.0)
    add_text(board, "12-24V IN", 8, 15.5, 1.2)
    add_text(board, "FUSE 3A", 28, 16.5, 1.1)
    add_text(board, "5V 2A", 84, 33, 1.0)
    add_text(board, "5V SEL", 94, 36, 0.9)
    add_text(board, "EXT 5V", 102, 35, 0.9)
    add_text(board, "ESP32-DevKitC V4", 36, 34, 1.4)
    add_text(board, "USB", 60, 68, 1.0)
    add_text(board, "ANTENNA KEEPOUT", 5, 52, 0.9, angle=90)
    add_text(board, "A4988 / DRV8825", 94, 43, 1.2)
    add_text(board, "EN", 84, 44, 1.0)
    add_text(board, "VMOT", 92, 59, 1.0)
    add_text(board, "MOTOR", 92, 66, 1.1)
    add_text(board, "MS1", 74, 44, 0.9)
    add_text(board, "MS2", 74, 49, 0.9)
    add_text(board, "MS3", 74, 54, 0.9)
    add_text(board, "74HC14", 62, 74, 1.0)
    add_text(board, "74AHCT125", 80, 74, 1.0)
    add_text(board, "S1", 4, 94, 1.2)
    add_text(board, "S2", 18, 94, 1.2)
    add_text(board, "S3", 32, 94, 1.2)
    add_text(board, "S4", 46, 94, 1.2)
    add_text(board, "SENSORS: VCC OUT GND", 10, 90, 0.8)
    add_text(board, "RESET", 34, 67.5, 1.0)
    add_text(board, "LED BLUE", 78, 91, 0.9)
    add_text(board, "LED RED", 94, 91, 0.9)
    add_text(board, "JP2 3V3/5V", 4, 67, 0.8)


def add_keepout(board: pcbnew.BOARD, net_gnd: pcbnew.NETINFO_ITEM) -> None:
    """Antenna keepout: no copper / tracks / vias on both layers."""
    for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
        zone = pcbnew.ZONE(board)
        zone.SetLayer(layer)
        zone.SetIsRuleArea(True)
        zone.SetDoNotAllowCopperPour(True)
        zone.SetDoNotAllowTracks(True)
        zone.SetDoNotAllowVias(True)
        zone.SetDoNotAllowPads(False)
        zone.SetDoNotAllowFootprints(False)
        poly = zone.Outline()
        poly.NewOutline()
        for x, y in [(0.3, 33.0), (11.0, 33.0), (11.0, 70.0), (0.3, 70.0), (0.3, 33.0)]:
            poly.Append(MM(x), MM(y))
        board.Add(zone)


def add_gnd_zone(board: pcbnew.BOARD, net_gnd: pcbnew.NETINFO_ITEM) -> None:
    zone = pcbnew.ZONE(board)
    zone.SetLayer(pcbnew.B_Cu)
    zone.SetNet(net_gnd)
    zone.SetAssignedPriority(0)
    zone.SetLocalClearance(MM(0.25))
    zone.SetMinThickness(MM(0.25))
    zone.SetThermalReliefGap(MM(0.3))
    zone.SetThermalReliefSpokeWidth(MM(0.5))
    zone.SetPadConnection(pcbnew.ZONE_CONNECTION_FULL)
    zone.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
    poly = zone.Outline()
    poly.NewOutline()
    for x, y in [(0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5), (0.5, 0.5)]:
        poly.Append(MM(x), MM(y))
    board.Add(zone)


def save(board: pcbnew.BOARD) -> None:
    pcbnew.SaveBoard(str(BOARD_PATH), board)


def export_bom(board: pcbnew.BOARD) -> None:
    rows = []
    for ref, key, x, y, rot, value, desc in PLACEMENTS:
        lib, name = FPS[key]
        rows.append({
            "Reference": ref,
            "Value": value,
            "Footprint": f"{lib}/{name}",
            "Description": desc,
            "X_mm": f"{x:.2f}",
            "Y_mm": f"{y:.2f}",
            "Rotation": rot,
        })
    with BOM_PATH.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0]))
        w.writeheader()
        w.writerows(rows)


def main() -> int:
    board = build_board()
    add_outline(board)
    add_silkscreen(board)
    nets = {n.GetNetname(): n for n in board.GetNetsByName().values()}
    add_keepout(board, nets["GND"])
    if "--no-plane" not in sys.argv:
        add_gnd_zone(board, nets["GND"])

    # Zone filling is done by build.py after routing; the pcbnew filler needs
    # a board that has been loaded from disk in this KiCad build.
    board.BuildConnectivity()
    save(board)
    export_bom(board)
    print(f"wrote {BOARD_PATH}")
    print(f"wrote {BOM_PATH}")
    print(f"nets: {board.GetNetCount()}, footprints: {len(board.GetFootprints())}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
