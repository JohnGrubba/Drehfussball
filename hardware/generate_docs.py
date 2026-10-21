#!/usr/bin/env python3
"""Generate connection and grouped BOM tables from the finished PCB."""

from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path

import pcbnew

import generate_board as gb

HERE = Path(__file__).resolve().parent
BOARD = HERE / "Drehfussball_Controller.kicad_pcb"


def main() -> None:
    board = pcbnew.LoadBoard(str(BOARD))

    rows = []
    for fp in board.GetFootprints():
        ref = fp.GetReference()
        for pad in fp.Pads():
            net = pad.GetNetname()
            if net:
                rows.append((net, ref, pad.GetNumber()))
    rows.sort(key=lambda r: (r[0], r[1], r[2]))
    with (HERE / "connections.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Net", "Reference", "Pad"])
        w.writerows(rows)

    groups = defaultdict(list)
    descriptions = {}
    for ref, key, x, y, rot, value, desc in gb.PLACEMENTS:
        if key == "mh":
            continue
        lib, name = gb.FPS[key]
        fp = f"{lib}/{name}"
        groups[(value, fp)].append(ref)
        descriptions.setdefault((value, fp), desc)

    with (HERE / "BOM_grouped.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["Qty", "Value", "Footprint", "References", "Description"])
        for (value, fp), refs in sorted(groups.items(), key=lambda kv: kv[0][0]):
            w.writerow([len(refs), value, fp, " ".join(sorted(refs)),
                        descriptions[(value, fp)]])

    print(f"wrote connections.csv ({len(rows)} rows)")
    print(f"wrote BOM_grouped.csv ({len(groups)} groups)")


if __name__ == "__main__":
    main()
