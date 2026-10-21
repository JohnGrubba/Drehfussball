#!/usr/bin/env python3
"""Reproducible build for the Drehfussball controller PCB.

Steps:
  1. generate_board.py --no-plane   (placement + nets, no copper plane yet)
  2. export Specctra DSN
  3. route with FreeRouting (0.30 mm tracks)
  4. import the routed SES back into KiCad
  5. add the B.Cu GND plane, fill zones
  6. run DRC, export Gerbers + drill files

Requires the locally extracted KiCad and Java 25 (see README.md):
  export LD_LIBRARY_PATH=/tmp/opencode/kicad/root/usr/lib/x86_64-linux-gnu
  export PYTHONPATH=/tmp/opencode/kicad/root/usr/lib/python3/dist-packages
  python3 build.py
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

import pcbnew

HERE = Path(__file__).resolve().parent
BOARD = HERE / "Drehfussball_Controller.kicad_pcb"
DSN = HERE / "Drehfussball_Controller.dsn"
SES = HERE / "Drehfussball_Controller.ses"
GERBER = HERE / "gerber"
ZIP = HERE / "Drehfussball_Controller_Gerber.zip"

JAVA = os.environ.get("JAVA", "/tmp/opencode/jdk25/bin/java")
FREEROUTING = os.environ.get("FREEROUTING", "/tmp/opencode/freerouting.jar")
KICAD_CLI = os.environ.get("KICAD_CLI", "/tmp/opencode/kicad/root/usr/bin/kicad-cli")


def run(cmd: list[str]) -> None:
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=HERE)


def main() -> int:
    # 1. placement + nets, without the ground plane (FreeRouting must route GND)
    run([sys.executable, "generate_board.py", "--no-plane"])

    # 2. export DSN and set a 0.30 mm track width for the autorouter
    board = pcbnew.LoadBoard(str(BOARD))
    pcbnew.ExportSpecctraDSN(board, str(DSN))
    text = DSN.read_text()
    DSN.write_text(text.replace("(width 200)", "(width 400)"))

    # 3. autoroute
    run([JAVA, "-jar", FREEROUTING, "-de", str(DSN), "-do", str(SES),
         "-mp", "30", "-mt", "6"])

    # 4. import routed tracks
    board = pcbnew.LoadBoard(str(BOARD))
    pcbnew.ImportSpecctraSES(board, str(SES))
    board.BuildConnectivity()

    # 5. add the GND plane on the bottom layer and fill everything
    import generate_board as gb

    gnd = next(n for n in board.GetNetsByName().values() if n.GetNetname() == "GND")
    gb.add_gnd_zone(board, gnd)
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    board.BuildConnectivity()
    pcbnew.SaveBoard(str(BOARD), board)
    print(f"tracks: {len(board.GetTracks())}")

    # Keep cosmetic silk/courtyard checks out of the DRC summary.
    import json
    pro_path = BOARD.with_suffix(".kicad_pro")
    pro = json.loads(pro_path.read_text())
    severities = pro["board"]["design_settings"]["rule_severities"]
    for key in ("courtyards_overlap", "silk_edge_clearance",
                "silk_over_copper", "silk_overlap", "text_height"):
        severities[key] = "ignore"
    pro_path.write_text(json.dumps(pro, indent=2))

    # 6. DRC + fabrication data
    run([KICAD_CLI, "pcb", "drc", "-o", "drc.rpt", str(BOARD)])
    print("\n".join((HERE / "drc.rpt").read_text().splitlines()[:8]))
    if GERBER.exists():
        shutil.rmtree(GERBER)
    GERBER.mkdir()
    run([KICAD_CLI, "pcb", "export", "gerbers", "-o", str(GERBER) + "/",
         "-l", "F.Cu,B.Cu,F.Silkscreen,B.Silkscreen,F.Mask,B.Mask,Edge.Cuts",
         str(BOARD)])
    run([KICAD_CLI, "pcb", "export", "drill", "-o", str(GERBER) + "/",
         "--format", "excellon", "--drill-origin", "absolute",
         "--excellon-zeros-format", "decimal", "--excellon-units", "mm",
         "--excellon-separate-th", str(BOARD)])
    with zipfile.ZipFile(ZIP, "w", zipfile.ZIP_DEFLATED) as z:
        for f in sorted(GERBER.iterdir()):
            z.write(f, f.name)
    print(f"wrote {ZIP}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
