#!/bin/bash
# Resizes/compresses the user-supplied UI art in ui/ into assets/ui/ for BinaryData embedding.
# Originals in ui/ are untouched. Re-run after editing any source art.
set -euo pipefail
cd "$(dirname "$0")/.."

SRC=ui
DST=assets/ui
mkdir -p "$DST"

resize_png() {
    local src="$1" dst="$2" size="$3"
    magick "$SRC/$src" -resize "${size}x${size}" -strip "$DST/$dst"
    pngquant --force --skip-if-larger --quality=80-100 --speed 1 -o "$DST/$dst" "$DST/$dst" || true
    optipng -quiet -o2 "$DST/$dst"
}

# Knobs (square, alpha, drawn at "noon" — rotated in code)
resize_png bakelite_knob.png  bakelite_knob.png  256   # Volume/Drive/Tone main knobs
resize_png trim_knob.png      trim_knob.png      96    # Presence trim knobs
resize_png vol_trim.png       vol_trim.png       256   # Input/Output peripheral trim knobs

# LED bezels (glow baked in)
resize_png bezel_led_off.png    bezel_led_off.png    128
resize_png bezel_led_yellow.png bezel_led_yellow.png 128
resize_png bezel_led_red.png    bezel_led_red.png    128

# Footswitch (note: source is "Footswitch_up.png", capital F; normalize output to lowercase)
resize_png Footswitch_up.png  footswitch_up.png   200
resize_png footswitch_down.png footswitch_down.png 200

# Clip switch (3 discrete positions)
resize_png switch_up.png   switch_up.png   128
resize_png switch_Mid.png  switch_mid.png  128
resize_png switch_down.png switch_down.png 128

# Background texture — no alpha; convert to JPEG (smooth gradient, lossy compresses very well)
magick "$SRC/MoT_Texture.png" -resize 1536x -strip -quality 88 "$DST/mot_texture.jpg"

echo "--- processed sizes ---"
ls -la "$DST"
