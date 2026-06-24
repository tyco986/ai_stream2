#!/usr/bin/env python3
"""Export DeepStream GStreamer elements from gst-inspect to CSV and Markdown."""

import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

EXAMPLE_ELEMENTS = {"dsexample", "dsexample-cuda"}
EXAMPLE_PLUGIN_PREFIXES = ("nvdsgst_dsexample", "gst-dsexample")

SOURCE_OFFICIAL = "official"
SOURCE_OFFICIAL_EXAMPLE = "official-example"

FIELDNAMES = ["name", "description", "type", "source"]
CSV_NAME = "deepstream_gst_plugins.csv"
MD_NAME = "deepstream_gst_plugins.md"


def classify_type(stdout: str, name: str) -> str:
    hierarchy = stdout.split("GObject")[-1] if "GObject" in stdout else stdout
    if " +----GstBin" in hierarchy:
        return "Bin"
    if "GstVideoEncoder" in hierarchy:
        return "Encoder"
    if "GstVideoDecoder" in hierarchy:
        return "Decoder"
    if ("mux" in name or name.endswith("mixer")) and "demux" not in name:
        return "Mux"
    if "demux" in name:
        return "Demux"
    if "tiler" in name or "blender" in name:
        return "Compositor"
    if "GstPushSrc" in hierarchy or (
        "GstBaseSrc" in hierarchy and "GstBin" not in hierarchy
    ):
        return "Source"
    if "GstVideoSink" in hierarchy or "GstBaseSink" in hierarchy:
        return "Sink"
    if "GstBaseTransform" in hierarchy:
        return "Transform"
    return "Element"


def is_official_example(name: str, plugin: str) -> bool:
    if name in EXAMPLE_ELEMENTS:
        return True
    return any(plugin.startswith(prefix) for prefix in EXAMPLE_PLUGIN_PREFIXES)


def collect_elements() -> list[tuple[str, str, str]]:
    out = subprocess.check_output(["gst-inspect-1.0"], stderr=subprocess.DEVNULL, text=True)
    elements: list[tuple[str, str, str]] = []
    for line in out.splitlines():
        match = re.match(r"^(\S+):\s+(\S+):\s+(.+)$", line)
        if not match:
            continue
        plugin, elem, desc = match.group(1), match.group(2), match.group(3)
        if not (elem.startswith("nv") or elem == "dsexample"):
            continue
        if plugin == "dsd":
            continue
        elements.append((elem, desc, plugin))
    return elements


def build_rows() -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    seen: set[str] = set()
    for elem, short_desc, plugin in sorted(collect_elements(), key=lambda item: item[0]):
        if elem in seen:
            continue
        seen.add(elem)
        detail = subprocess.run(
            ["gst-inspect-1.0", elem],
            capture_output=True,
            text=True,
            check=False,
        )
        stdout = detail.stdout
        long_desc = short_desc
        for line in stdout.splitlines():
            stripped = line.strip()
            if stripped.startswith("Description"):
                long_desc = stripped.split(None, 1)[1] if " " in stripped else short_desc
        rows.append(
            {
                "name": elem,
                "description": long_desc,
                "type": classify_type(stdout, elem),
                "source": SOURCE_OFFICIAL_EXAMPLE if is_official_example(elem, plugin) else SOURCE_OFFICIAL,
            }
        )
    return rows


def write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)


def write_markdown(path: Path, rows: list[dict[str, str]]) -> None:
    lines = [
        "# DeepStream 9.0 GStreamer Plugin List",
        "",
        "Source: `gst-inspect-1.0` in DeepStream container.",
        f"Total: **{len(rows)}** inspectable elements.",
        "",
        "| name | description | type | source |",
        "| --- | --- | --- | --- |",
    ]
    for row in rows:
        desc = row["description"].replace("|", "\\|")
        lines.append(
            f"| `{row['name']}` | {desc} | {row['type']} | {row['source']} |"
        )
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export DeepStream GStreamer elements from gst-inspect.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="Write deepstream_gst_plugins.csv and .md to this directory.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    rows = build_rows()

    if args.output_dir:
        args.output_dir.mkdir(parents=True, exist_ok=True)
        write_csv(args.output_dir / CSV_NAME, rows)
        write_markdown(args.output_dir / MD_NAME, rows)
        print(f"Wrote {len(rows)} elements to {args.output_dir}", file=sys.stderr)
        return

    writer = csv.DictWriter(sys.stdout, fieldnames=FIELDNAMES)
    writer.writeheader()
    writer.writerows(rows)
    print(len(rows), file=sys.stderr)


if __name__ == "__main__":
    main()
