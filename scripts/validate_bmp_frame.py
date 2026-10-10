#!/usr/bin/env python3
"""Validate a captured, uncompressed BMP without third-party packages.

The AstroMenace AC090 runtime gate once accepted two technically valid BMPs
whose pixels were stale texture stripes or random readback noise.  This tool
checks the file structure and inexpensive image statistics so CI proves that a
captured frame is varied, spatially coherent and not a flat placeholder.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
import sys
from pathlib import Path


def fail(message: str) -> None:
    raise RuntimeError(message)


def validate(
    path: Path,
    *,
    min_width: int,
    min_height: int,
    min_channel_std: float,
    min_sampled_colours: int,
    max_horizontal_mad: float,
    max_vertical_mad: float,
) -> dict[str, object]:
    data = path.read_bytes()
    if len(data) < 54:
        fail(f"{path}: too small to be a BMP")

    signature, file_size, _reserved1, _reserved2, pixel_offset = struct.unpack_from(
        "<2sIHHI", data, 0
    )
    if signature != b"BM":
        fail(f"{path}: missing BM signature")
    if file_size not in (0, len(data)):
        fail(f"{path}: header size {file_size} does not match {len(data)} bytes")

    dib_size = struct.unpack_from("<I", data, 14)[0]
    if dib_size < 40 or 14 + dib_size > len(data):
        fail(f"{path}: unsupported or truncated DIB header ({dib_size})")

    width, signed_height, planes, bits_per_pixel, compression = struct.unpack_from(
        "<iiHHI", data, 18
    )
    if width < min_width or abs(signed_height) < min_height:
        fail(
            f"{path}: frame is only {width}x{abs(signed_height)}; "
            f"minimum is {min_width}x{min_height}"
        )
    if planes != 1:
        fail(f"{path}: expected one BMP plane, got {planes}")
    if bits_per_pixel not in (24, 32):
        fail(f"{path}: expected 24- or 32-bit pixels, got {bits_per_pixel}")
    if compression != 0:
        fail(f"{path}: compressed BMPs are not supported (compression={compression})")

    height = abs(signed_height)
    bytes_per_pixel = bits_per_pixel // 8
    row_stride = ((width * bits_per_pixel + 31) // 32) * 4
    required = pixel_offset + row_stride * height
    if required > len(data):
        fail(f"{path}: pixel array is truncated ({required} required, {len(data)} present)")

    # Read rows in visual top-to-bottom order regardless of BMP storage order.
    row_indices = range(height - 1, -1, -1) if signed_height > 0 else range(height)

    channel_sum = [0, 0, 0]
    channel_sum_sq = [0, 0, 0]
    pixel_count = width * height
    horizontal_sum = 0
    horizontal_count = 0
    vertical_sum = 0
    vertical_count = 0
    sampled_colours: set[tuple[int, int, int]] = set()
    previous_row: list[tuple[int, int, int]] | None = None

    sample_step_x = max(1, width // 160)
    sample_step_y = max(1, height // 120)

    for visual_y, stored_y in enumerate(row_indices):
        start = pixel_offset + stored_y * row_stride
        raw = data[start : start + width * bytes_per_pixel]
        row: list[tuple[int, int, int]] = []

        for x in range(width):
            offset = x * bytes_per_pixel
            blue, green, red = raw[offset : offset + 3]
            pixel = (red, green, blue)
            row.append(pixel)

            channel_sum[0] += red
            channel_sum[1] += green
            channel_sum[2] += blue
            channel_sum_sq[0] += red * red
            channel_sum_sq[1] += green * green
            channel_sum_sq[2] += blue * blue

            if x:
                left = row[x - 1]
                horizontal_sum += (
                    abs(red - left[0])
                    + abs(green - left[1])
                    + abs(blue - left[2])
                )
                horizontal_count += 3

            if previous_row is not None:
                above = previous_row[x]
                vertical_sum += (
                    abs(red - above[0])
                    + abs(green - above[1])
                    + abs(blue - above[2])
                )
                vertical_count += 3

            if visual_y % sample_step_y == 0 and x % sample_step_x == 0:
                sampled_colours.add(pixel)

        previous_row = row

    channel_std = []
    for total, total_sq in zip(channel_sum, channel_sum_sq):
        mean = total / pixel_count
        variance = max(0.0, total_sq / pixel_count - mean * mean)
        channel_std.append(math.sqrt(variance))

    mean_channel_std = sum(channel_std) / 3.0
    horizontal_mad = horizontal_sum / horizontal_count if horizontal_count else 0.0
    vertical_mad = vertical_sum / vertical_count if vertical_count else 0.0

    metrics: dict[str, object] = {
        "path": str(path),
        "bytes": len(data),
        "width": width,
        "height": height,
        "bitsPerPixel": bits_per_pixel,
        "meanChannelStd": round(mean_channel_std, 4),
        "channelStd": [round(value, 4) for value in channel_std],
        "sampledColours": len(sampled_colours),
        "horizontalAdjacentMAD": round(horizontal_mad, 4),
        "verticalAdjacentMAD": round(vertical_mad, 4),
    }

    problems: list[str] = []
    if mean_channel_std < min_channel_std:
        problems.append(
            f"mean channel standard deviation {mean_channel_std:.3f} < {min_channel_std:.3f}"
        )
    if len(sampled_colours) < min_sampled_colours:
        problems.append(
            f"sampled colour count {len(sampled_colours)} < {min_sampled_colours}"
        )
    if horizontal_mad > max_horizontal_mad:
        problems.append(
            f"horizontal adjacent-pixel MAD {horizontal_mad:.3f} > {max_horizontal_mad:.3f}"
        )
    if vertical_mad > max_vertical_mad:
        problems.append(
            f"vertical adjacent-pixel MAD {vertical_mad:.3f} > {max_vertical_mad:.3f}"
        )

    if problems:
        metrics["ok"] = False
        metrics["problems"] = problems
        print(json.dumps(metrics, indent=2, sort_keys=True))
        fail(f"{path}: framebuffer integrity check failed")

    metrics["ok"] = True
    return metrics


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bmp", type=Path)
    parser.add_argument("--min-width", type=int, default=320)
    parser.add_argument("--min-height", type=int, default=200)
    parser.add_argument("--min-channel-std", type=float, default=5.0)
    parser.add_argument("--min-sampled-colours", type=int, default=100)
    parser.add_argument("--max-horizontal-mad", type=float, default=45.0)
    parser.add_argument("--max-vertical-mad", type=float, default=45.0)
    args = parser.parse_args()

    try:
        metrics = validate(
            args.bmp,
            min_width=args.min_width,
            min_height=args.min_height,
            min_channel_std=args.min_channel_std,
            min_sampled_colours=args.min_sampled_colours,
            max_horizontal_mad=args.max_horizontal_mad,
            max_vertical_mad=args.max_vertical_mad,
        )
    except (OSError, RuntimeError, struct.error) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(json.dumps(metrics, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
