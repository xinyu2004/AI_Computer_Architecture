#!/usr/bin/env python3
# Turn the O3PipeView trace of one measurement window into two files:
# the committed instructions, and a short pipeline diagram.
# labs/i386/gem5/launch.sh runs this after gem5.

import argparse
import html
import os
import subprocess
import sys

# One gem5 tick is 1 ps. The measured CPU is clocked at 1 GHz.
CYCLE_TICKS = 1000
BAR_PX = 720

STAGES = (
    ("fetch", "fetch", "#4c78a8"),
    ("decode", "decode", "#72b7b2"),
    ("rename", "rename", "#54a24b"),
    ("dispatch", "dispatch", "#eeca3b"),
    ("issue", "issue", "#f58518"),
    ("complete", "complete", "#b279a2"),
    ("retire", "retire", "#e45756"),
)


def parse_trace(path):
    groups = []
    cur = None
    with open(path, errors="replace") as trace:
        for raw in trace:
            line = raw.rstrip("\n")
            if line.startswith("O3PipeView:fetch:"):
                if cur is not None:
                    groups.append(cur)
                parts = line.split(":", 6)
                if len(parts) < 7:
                    cur = None
                    continue
                cur = {
                    "lines": [line],
                    "pc": parts[3],
                    "upc": parts[4],
                    "sn": int(parts[5]),
                    "disasm": parts[6].strip(),
                    "retire": 0,
                }
            elif cur is not None and line.startswith("O3PipeView:"):
                cur["lines"].append(line)
                if line.startswith("O3PipeView:retire:"):
                    bits = line.split(":")
                    if len(bits) > 2:
                        cur["retire"] = int(bits[2])
    if cur is not None:
        groups.append(cur)
    return [g for g in groups if g["retire"] != 0]


def from_start(groups, start):
    # The trace is dumped when each instruction is destroyed, which is
    # not program order. Cut at the first sequence number of the window
    # and then sort, so a microop destroyed early is not dropped.
    if start is None:
        return sorted(groups, key=lambda group: group["sn"])
    window = [g["sn"] for g in groups if int(g["pc"], 16) == start]
    if not window:
        return sorted(groups, key=lambda group: group["sn"])
    cut = min(window)
    return sorted(
        (g for g in groups if g["sn"] >= cut),
        key=lambda group: group["sn"],
    )


def select(groups, mode):
    # timer fits on a few screens. schedule is the task switch, still
    # short enough to keep. The page-table fill and the page copy are
    # one loop repeated thousands of times: keep the start, and for
    # paging also the epilogue that writes CR3 and CR0.
    # A user range keeps every committed microop. A long stretch is the
    # caller's choice; the named paging and copy_page runs are the ones
    # that slice a repeated loop.
    if mode in ("timer", "schedule", "range"):
        return groups, None
    head = 32
    tail = 12 if mode == "paging" else 0
    total = len(groups)
    if total <= head + tail:
        return groups, None
    if tail == 0:
        chosen = groups[:head]
        note = "Showing the first %d of %d microops.\n" % (head, total)
    else:
        chosen = groups[:head] + groups[-tail:]
        note = "Showing the first %d and the last %d of %d microops.\n" % (
            head,
            tail,
            total,
        )
    return chosen, note


def stage_spans(group):
    ticks = {}
    for line in group["lines"]:
        if not line.startswith("O3PipeView:"):
            continue
        bits = line.split(":")
        if len(bits) < 3:
            continue
        name = bits[1]
        if name not in {stage[0] for stage in STAGES}:
            continue
        tick = int(bits[2])
        if tick > 0:
            ticks[name] = tick
    present = [name for name, _label, _color in STAGES if name in ticks]
    spans = []
    for index, name in enumerate(present):
        if index + 1 < len(present):
            cycles = (ticks[present[index + 1]] - ticks[name]) // CYCLE_TICKS
        else:
            cycles = 1
        if cycles < 1:
            cycles = 1
        spans.append((name, cycles))
    return spans


def write_html(path, groups, note, mode):
    rows = []
    for group in groups:
        spans = stage_spans(group)
        rows.append((group, spans, sum(cycles for _name, cycles in spans)))
    longest = max((total for _group, _spans, total in rows), default=1)
    if longest < 1:
        longest = 1

    def esc(text):
        return html.escape(text, quote=True)

    parts = [
        "<!DOCTYPE html>",
        '<meta charset="utf-8">',
        "<title>%s pipeline</title>" % esc(mode),
        "<style>",
        "body { margin: 16px; background: #1b1b1b; color: #e6e6e6;",
        "  font: 13px/1.35 ui-monospace, monospace; }",
        "h1 { font-size: 16px; font-weight: 600; }",
        ".legend { display: flex; gap: 14px; margin: 8px 0 16px; }",
        ".legend span { display: inline-flex; align-items: center; gap: 6px; }",
        ".swatch { width: 12px; height: 12px; display: inline-block; }",
        "table { border-collapse: collapse; }",
        "td { padding: 1px 8px; white-space: nowrap; vertical-align: middle; }",
        ".pc { color: #9cdcfe; }",
        ".bar { display: flex; height: 14px; background: #2a2a2a; }",
        ".seg { height: 14px; min-width: 2px; }",
        ".note { color: #bbbbbb; }",
        "</style>",
        "<h1>%s</h1>" % esc(mode),
        '<p class="note">One row is one microop. The left edge is fetch. '
        "A block is time in that stage or waiting for the next one. "
        "Rows share one scale: the longest row is %d cycles.</p>" % longest,
    ]
    if note:
        parts.append('<p class="note">%s</p>' % esc(note.strip()))
    parts.append('<div class="legend">')
    for _name, label, color in STAGES:
        parts.append(
            '<span><i class="swatch" style="background:%s"></i>%s</span>'
            % (color, esc(label))
        )
    parts.append("</div>")
    parts.append("<table>")
    colors = {name: color for name, _label, color in STAGES}
    for group, spans, total in rows:
        parts.append("<tr>")
        parts.append(
            '<td class="pc">%s.%s</td>' % (esc(group["pc"]), esc(group["upc"]))
        )
        parts.append("<td>%s</td>" % esc(group["disasm"]))
        parts.append('<td><div class="bar">')
        for name, cycles in spans:
            width = max(2, int(round(BAR_PX * cycles / longest)))
            parts.append(
                '<i class="seg" style="width:%dpx;background:%s"></i>'
                % (width, colors[name])
            )
        parts.append("</div></td>")
        parts.append("<td>%d</td>" % total)
        parts.append("</tr>")
    parts.append("</table>")
    with open(path, "w") as out:
        out.write("\n".join(parts) + "\n")


def write_instructions(path, groups, note):
    with open(path, "w") as out:
        out.write(
            "Committed microops in this window. "
            "Several microops can share one x86 instruction.\n"
        )
        if note:
            out.write(note)
        out.write("\n")
        for group in groups:
            out.write(
                "%s.%-2s  %s\n" % (group["pc"], group["upc"], group["disasm"])
            )


def write_pipeline(path, groups, pipeview, pythonpath):
    raw_path = path + ".slice"
    with open(raw_path, "w") as raw:
        for group in groups:
            for line in group["lines"]:
                raw.write(line + "\n")
    env = os.environ.copy()
    env["PYTHONPATH"] = pythonpath + (
        os.pathsep + env["PYTHONPATH"] if env.get("PYTHONPATH") else ""
    )
    drawn = path + ".drawn"
    proc = subprocess.run(
        [
            sys.executable,
            pipeview,
            "--only_committed",
            "-c",
            "1000",
            "-w",
            "72",
            "-o",
            drawn,
            raw_path,
        ],
        env=env,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    os.remove(raw_path)
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        raise SystemExit("o3-pipeview failed.")
    with open(path, "w") as out, open(drawn) as src:
        out.write(
            "Each row is one microop. Time moves right. "
            "f fetch, d decode, n rename, p dispatch, "
            "i issue, c complete, r retire.\n"
            "A dot is a cycle in that stage or waiting for the next one.\n\n"
        )
        out.write(src.read())
    os.remove(drawn)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--trace", required=True)
    parser.add_argument("--mode", required=True)
    parser.add_argument("--start", required=True)
    parser.add_argument("--instructions", required=True)
    parser.add_argument("--pipeline", required=True)
    parser.add_argument("--html", required=True)
    parser.add_argument("--pipeview", required=True)
    parser.add_argument("--pythonpath", required=True)
    parser.add_argument("--expect", required=True, type=int)
    args = parser.parse_args()

    if not os.path.isfile(args.trace) or os.path.getsize(args.trace) == 0:
        sys.stderr.write("No pipeline trace in %s.\n" % args.trace)
        raise SystemExit(1)

    groups = from_start(parse_trace(args.trace), int(args.start, 16))
    if not groups:
        sys.stderr.write("The pipeline trace has no committed instructions.\n")
        raise SystemExit(1)
    if len(groups) != args.expect:
        sys.stderr.write(
            "The trace has %d committed microops. "
            "The microops counter is %d.\n" % (len(groups), args.expect)
        )
        raise SystemExit(1)
    chosen, omitted = select(groups, args.mode)
    write_instructions(args.instructions, chosen, omitted)
    write_pipeline(args.pipeline, chosen, args.pipeview, args.pythonpath)
    write_html(args.html, chosen, omitted, args.mode)


if __name__ == "__main__":
    main()
