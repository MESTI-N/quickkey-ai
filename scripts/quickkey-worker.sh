#!/bin/bash
# QuickKey's standalone worker. No user terminal is involved.
# The CorridorKey engine and models must be installed independently.
set -Eeuo pipefail
ENGINE="$1"
SOURCE="$2"
OUTPUT="$3"
DESPILL="$4"
mkdir -p "$OUTPUT"
LOG="$OUTPUT/quickkey.log"
exec >>"$LOG" 2>&1
printf 'QuickKey job begun: %s\n' "$(date)"
printf 'Source: %s\nOutput: %s\n' "$SOURCE" "$OUTPUT"
rm -f "$OUTPUT/.quickkey_ready" "$OUTPUT/.quickkey_error" "$OUTPUT/.quickkey_running"
touch "$OUTPUT/.quickkey_running"
fail() {
  rc=$?
  printf 'AI render failed (exit %s) at %s\n' "$rc" "$(date)"
  printf '%s\n' "Error (exit $rc). See quickkey.log" > "$OUTPUT/.quickkey_error"
  rm -f "$OUTPUT/.quickkey_running"
}
trap fail ERR
if [ ! -x "$ENGINE" ]; then
  echo "Missing executable AI engine: $ENGINE" >&2
  exit 127
fi
if [ ! -f "$SOURCE" ]; then
  echo "Source footage missing: $SOURCE" >&2
  exit 2
fi
# The original input footage is used, not a frame already keyed in Resolve.
# EXR cutout contains REAL AI-predicted premultiplied RGBA; video preview does not.
"$ENGINE" render --input "$SOURCE" --output "$OUTPUT" \
  --outputs cutout --format exr --despill "$DESPILL" --overwrite
# Output is committed only if an actual cutout EXR exists.
FIRST="$(find "$OUTPUT" -type f -name '*_cutout.*.exr' -print -quit)"
if [ -z "$FIRST" ]; then
  echo "No cutout EXRs found. Check AI engine output above." >&2
  exit 15
fi
printf 'Ready: %s\n' "$FIRST"
touch "$OUTPUT/.quickkey_ready"
rm -f "$OUTPUT/.quickkey_running"
trap - ERR
