import colorsys

from PIL import Image, ImageDraw, ImageFont


def get_sahi_preview(
    info: dict[str, int | dict[str, tuple[int, int, int, int]]],
) -> Image.Image:
    image_width: int = info["width"]
    image_height: int = info["height"]
    row_count: int = info["row"]
    image = Image.new("RGB", (image_width, image_height), (0, 0, 0))
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default()
    boxes = info["box"]

    for label, box in boxes.items():
        x1, y1, x2, y2 = box
        if label == "full":
            color = (255, 255, 255)
        else:
            row = int(label.split("-", maxsplit=1)[0])
            hue = row / max(row_count, 1)
            red, green, blue = colorsys.hsv_to_rgb(hue, 0.85, 1.0)
            color = int(red * 255), int(green * 255), int(blue * 255)
        draw.rectangle((x1, y1, x2, y2), outline=color, width=2)
        text_y = y1 + 2 if y1 + 14 < image_height else max(0, y1 - 12)
        draw.text((x1 + 2, text_y), label, fill=color, font=font)
    return image


def get_sahi_box(
    image_width: int,
    image_height: int,
    slice_width: int,
    slice_height: int,
    overlap_width_ratio: float,
    overlap_height_ratio: float,
    enable_full_frame: bool,
) -> dict[str, int | dict[str, tuple[int, int, int, int]]]:
    """Return SAHI slice layout: frame size, grid ``row``/``column`` counts, ``num``, and ``box``.

    ``box`` maps ``{row}-{column}`` → ``(x1, y1, x2, y2)`` (zero-based grid indices).
    When ``enable_full_frame`` is True, the full-frame tile is under ``box["full"]``.
    ``num`` is the total tile count (grid slices + optional full frame).

    Mirrors ``compute_sahi_slices`` in deepstream-sahi ``gstnvsahipreprocess``.
    """
    w_step = max(1, slice_width - int(slice_width * overlap_width_ratio))
    h_step = max(1, slice_height - int(slice_height * overlap_height_ratio))

    boxes: dict[str, tuple[int, int, int, int]] = {}
    row = 0
    grid_columns = 0
    y = 0
    while y < image_height:
        y_end = min(y + slice_height, image_height)
        y_start = y
        if y_end == image_height and y_start > 0 and (y_end - y_start) < slice_height:
            y_start = max(0, image_height - slice_height)

        col = 0
        x = 0
        while x < image_width:
            x_end = min(x + slice_width, image_width)
            x_start = x
            if x_end == image_width and x_start > 0 and (x_end - x_start) < slice_width:
                x_start = max(0, image_width - slice_width)
            boxes[f"{row}-{col}"] = (x_start, y_start, x_end, y_end)
            col += 1
            if x_end >= image_width:
                break
            x += w_step

        grid_columns = max(grid_columns, col)
        row += 1
        if y_end >= image_height:
            break
        y += h_step

    if enable_full_frame:
        boxes["full"] = (0, 0, image_width, image_height)

    return {
        "width": image_width,
        "height": image_height,
        "row": row,
        "column": grid_columns,
        "num": len(boxes),
        "box": boxes,
    }
