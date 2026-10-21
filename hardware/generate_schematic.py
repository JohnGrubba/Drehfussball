#!/usr/bin/env python3
"""Generate a KiCad schematic for the Drehfussball controller.

The schematic uses the same reference designators, values and net names as the
PCB.  Connections are drawn as short net labels at each pin, which keeps the
schematic easy to generate and easy to edit in KiCad.

Run with:
  export KICAD_SYMBOL_DIR=/tmp/opencode/kicad/root/usr/share/kicad/symbols
  export LD_LIBRARY_PATH=/tmp/opencode/kicad/root/usr/lib/x86_64-linux-gnu
  export PYTHONPATH=/tmp/opencode/kicad/root/usr/lib/python3/dist-packages:/tmp/opencode/pylibs
  python3 generate_schematic.py
"""

from __future__ import annotations

from pathlib import Path

import kicad_sch_api as ksa

import generate_board as gb

HERE = Path(__file__).resolve().parent
OUT = HERE / "Drehfussball_Controller.kicad_sch"

# ref -> (x, y) on an A2 sheet (594 x 420 mm)
LAYOUT = {
    # power input / regulator
    "J1": (30, 30), "F1": (65, 30), "D1": (100, 30), "D2": (130, 30),
    "C2": (160, 30), "C1": (190, 30),
    "U1": (30, 62), "C3": (65, 62), "C4": (95, 62), "JP1": (125, 62),
    "J2": (155, 62), "R1": (185, 62), "LED1": (215, 62),

    # ESP32 sockets
    "J12": (35, 118), "J13": (35, 178), "C5": (110, 118),

    # stepper driver
    "J3": (270, 40), "J4": (340, 40), "C14": (395, 40), "C13": (430, 40),
    "JP3": (270, 105), "JP4": (300, 105), "JP5": (330, 105), "J5": (365, 105),

    # sensors
    "U2": (35, 250), "C11": (90, 250),
    "J6": (120, 250), "J7": (150, 250), "J8": (180, 250), "J9": (210, 250),
    "R2": (120, 282), "R3": (150, 282), "R4": (180, 282), "R5": (210, 282),
    "R6": (120, 310), "R7": (150, 310), "R8": (180, 310), "R9": (210, 310),
    "C6": (120, 338), "C7": (150, 338), "C8": (180, 338), "C9": (210, 338),
    "JP2": (250, 250),

    # button / LED output
    "SW1": (270, 175), "R10": (300, 175), "C10": (330, 175),
    "U3": (270, 215), "C12": (315, 215),
    "C15": (360, 215),
    "R14": (330, 275), "R15": (365, 275),
    "J10": (330, 310), "J11": (365, 310),
}


def symbol_for(key: str, value: str) -> str:
    if key == "r":
        return "Device:R"
    if key == "c":
        return "Device:C"
    if key in ("cp8", "cp10"):
        return "Device:C_Polarized"
    if key == "led":
        return "Device:LED"
    if key == "d15":
        return "Device:D_TVS"
    if key == "d201":
        return "Device:D"
    if key == "dip14":
        return "74xx:74HC14" if "74HC14" in value else "74xx:74AHCT125"
    if key == "to220":
        return "Regulator_Switching:TSR2-2450"
    if key == "sw":
        return "Switch:SW_Push"
    if key == "fuse":
        return "Device:Fuse"
    if key in ("tb2", "h2"):
        return "Connector_Generic:Conn_01x02"
    if key in ("tb3", "tb3s", "s3", "h3"):
        return "Connector_Generic:Conn_01x03"
    if key in ("tb4", "tb4s"):
        return "Connector_Generic:Conn_01x04"
    if key == "s8":
        return "Connector_Generic:Conn_01x08"
    if key == "s19":
        return "Connector_Generic:Conn_01x19"
    raise ValueError(f"no schematic symbol for {key}")


def label_rotation(cx: float, cy: float, px: float, py: float) -> float:
    dx, dy = px - cx, py - cy
    if abs(dx) >= abs(dy):
        return 0.0 if dx > 0 else 180.0
    return 90.0 if dy < 0 else 270.0


def main() -> None:
    pin_to_net = {}
    for net, pins in gb.NETS.items():
        for ref, pad in pins:
            pin_to_net[(ref, pad)] = net

    sch = ksa.create_schematic("Drehfussball Controller v1.0")
    sch.set_paper_size("A2")
    sch.set_title_block(
        title="Drehfussball Controller v1.0",
        date="2026-10-20",
        rev="1.0",
        company="",
        comments={1: "ESP32-DevKitC + A4988 + 4x IR light barrier + 2x WS2812B"},
    )

    components = {}
    for ref, key, x, y, rot, value, desc in gb.PLACEMENTS:
        if key == "mh":
            continue
        lib = symbol_for(key, value)
        px, py = LAYOUT.get(ref, (x, y))
        comp = sch.components.add(lib, ref, value, position=(px, py))
        components[ref] = (comp, key)

    # Add net labels / no-connects at every pin.
    for ref, (comp, key) in components.items():
        cx, cy = comp.position.x, comp.position.y
        pins = sch.list_component_pins(ref)
        for pad, point in pins:
            net = pin_to_net.get((ref, pad))
            if net:
                sch.add_label(net, pin=(ref, pad),
                              rotation=label_rotation(cx, cy, point.x, point.y),
                              size=1.0)
            else:
                sch.no_connects.add((point.x, point.y))

    # Functional notes / block titles
    notes = [
        ("POWER: 12-24V IN -> FUSE -> REVERSE PROTECTION -> 5V/2A", 80, 18, 1.8),
        ("ESP32-DevKitC V4 (J12/J13 are the two 19-pin headers)", 80, 105, 1.8),
        ("STEPPER: A4988 / DRV8825 SOCKET", 260, 28, 2.0),
        ("SENSOR FRONT END: 4x IR LIGHT BARRIER, 74HC14 SCHMITT TRIGGER", 80, 232, 1.8),
        ("BUTTON + WS2812 LEVEL SHIFTER", 260, 160, 2.0),
        ("IR BEAM BLOCKED = GPIO HIGH (74HC14 INVERTS)", 80, 372, 1.6),
        ("JP1: 1-2 = INTERNAL 5V, 2-3 = EXTERNAL 5V", 250, 372, 1.6),
        ("JP2: 1-2 = 3V3 SENSORS, 2-3 = 5V SENSORS", 250, 382, 1.6),
        ("JP3..JP5 CLOSED = MICROSTEP PIN HIGH", 250, 392, 1.6),
        ("A4988 RST/SLP ARE TIED TO +5V", 250, 402, 1.6),
        ("S1..S4: 1=VCC 2=OUT 3=GND", 80, 402, 1.6),
        ("LED CONNECTORS: 1=5V 2=DIN 3=GND", 330, 402, 1.6),
    ]
    for text, x, y, size in notes:
        sch.add_text(text, (x, y), size=size)

    sch.save(str(OUT))
    print(f"wrote {OUT}")
    print(f"components: {len(components)}")


if __name__ == "__main__":
    main()
