#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname "$0")/common.sh"
manifest_ref="$(python3 - "$ROOT/resources/aros/boot/manifest.json" <<'PY'
import json,sys
print(json.load(open(sys.argv[1],encoding='utf-8'))['aros']['commit'])
PY
)"
[ "$manifest_ref" = "$AROS_REF" ] || { echo "AROS lock differs from boot manifest" >&2; exit 1; }
for f in \
  "$ROOT/vendor/amigachrome-guest/scripts/stage_aros_module.py" \
  "$ROOT/vendor/amigachrome-guest/devices/acatapi.device/aros/mmakefile.src" \
  "$ROOT/vendor/amigachrome-guest/devices/acatapi.device/aros/acatapi.conf" \
  "$ROOT/vendor/amigachrome-guest/devices/acatapi.device/aros/acatapi_device.c" \
  "$ROOT/vendor/amigachrome-guest/devices/acatapi.device/core/acatapi_core.c" \
  "$ROOT/vendor/amigachrome-guest/devices/acscsi.device/aros/mmakefile.src" \
  "$ROOT/vendor/amigachrome-guest/devices/acscsi.device/aros/acscsi.conf" \
  "$ROOT/vendor/amigachrome-guest/devices/acscsi.device/aros/acscsi_device.c" \
  "$ROOT/vendor/amigachrome-guest/filesystems/acfs/aros/mmakefile.src" \
  "$ROOT/vendor/amigachrome-guest/filesystems/acfs/aros/acfs_handler.c" \
  "$ROOT/vendor/amigachrome-guest/filesystems/acfs/aros/DH0" \
  "$ROOT/vendor/amigachrome-guest/common/protocol/acstorage.h" \
  "$ROOT/vendor/amigachrome-guest/build/hf5-autoboot/elf_flatten.py" \
  "$ROOT/vendor/amigachrome-guest/build/hf5-autoboot/acstorage_bootrom.s" \
  "$ROOT/scripts/build_aros_boot_resource.py" \
  "$ROOT/scripts/build_boot_folder_resource.py" \
  "$ROOT/scripts/build_boot_folder_resource.py" \
  "$TOOLS_DIR/snapshot-toolchain.sh" \
  "$TOOLS_DIR/restore-toolchain.sh"; do
  require_file "$f"
done
for f in "$TOOLS_DIR"/*.sh "$ROOT/BUILD-AROS-LOCAL.sh" "$ROOT/EXPORT-AROS-BUILD-CAPSULE.sh"; do
  bash -n "$f"
done
acscsi_mmake="$ROOT/vendor/amigachrome-guest/devices/acscsi.device/aros/mmakefile.src"
grep -F -q -e '-I $(SRCDIR)/$(CURDIR)/core' "$acscsi_mmake" || { echo "acscsi AROS include path must use module-local core/" >&2; exit 1; }
grep -F -q -e 'core/acstorage_core' "$acscsi_mmake" || { echo "acscsi AROS source path must use module-local core/" >&2; exit 1; }
if grep -F -q -e '../core' "$acscsi_mmake"; then
  echo "acscsi AROS mmakefile still references stale ../core layout" >&2
  exit 1
fi
printf 'AROS_REF=%s\n' "$AROS_REF"
printf 'AROS_TOOLCHAIN_PREFIX=%s\n' "$AROS_TOOLCHAIN_DIR"
printf 'GUEST_SOURCE_DIGEST=%s\n' "$(find "$ROOT/vendor/amigachrome-guest/devices/acatapi.device" "$ROOT/vendor/amigachrome-guest/devices/acscsi.device" "$ROOT/vendor/amigachrome-guest/filesystems/acfs" "$ROOT/vendor/amigachrome-guest/common/protocol" -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}')"
printf 'BUILDER_DIGEST=%s\n' "$(find "$TOOLS_DIR" -type f -print0 | sort -z | xargs -0 sha256sum | sha256sum | awk '{print $1}')"
if check_toolchain_marker >/dev/null 2>&1; then
  echo 'TOOLCHAIN_STATUS=ready'
elif [ -f "$TOOLCHAIN_SNAPSHOT" ]; then
  echo 'TOOLCHAIN_STATUS=snapshot-available'
else
  echo 'TOOLCHAIN_STATUS=build-required'
fi
echo "Local AROS builder self-check: OK"

