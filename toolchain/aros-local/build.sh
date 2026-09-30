#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname "$0")/common.sh"
check_source_ref
check_toolchain_marker
[ -f "$AROS_BUILD/config.status" ] || { echo "Environment not prepared. Run ./BUILD-AROS-LOCAL.sh first." >&2; exit 4; }
python3 "$ROOT/vendor/amigachrome-guest/scripts/stage_aros_module.py" "$AROS_SRC"
( cd "$AROS_BUILD" && make -j1 "$AROS_DEVICE_TARGET" "$AROS_SCSI_TARGET" "$AROS_ACFS_TARGET" ) 2>&1 | tee "$LOG_DIR/guest-stack-build.log"
DEVICE="$(find "$AROS_BUILD" -type f -name 'acatapi.device' -print -quit)"
SCSI="$(find "$AROS_BUILD" -type f -name 'acscsi.device' -print -quit)"
ACFS="$(find "$AROS_BUILD" -type f \( -name 'ACFS' -o -name 'acfs-handler' \) -print -quit)"
AROS_TREE="$(find "$AROS_BUILD" -type d -name 'Emergency-Boot' -print -quit)"
[ -n "$DEVICE" ] && [ -f "$DEVICE" ] || { echo "Built acatapi.device not found" >&2; exit 6; }
[ -n "$SCSI" ] && [ -f "$SCSI" ] || { echo "Built acscsi.device not found" >&2; exit 6; }
[ -n "$ACFS" ] && [ -f "$ACFS" ] || { echo "Built ACFS handler not found" >&2; exit 6; }
[ -n "$AROS_TREE" ] && [ -d "$AROS_TREE" ] || { echo "AROS Emergency-Boot tree not found" >&2; exit 6; }
mkdir -p "$OUT"
cp "$DEVICE" "$OUT/acatapi.device"
cp "$SCSI" "$OUT/acscsi.device"
cp "$ACFS" "$OUT/ACFS"
cp "$ROOT/vendor/amigachrome-guest/devices/acatapi.device/aros/CD0" "$OUT/CD0"
cp "$ROOT/vendor/amigachrome-guest/filesystems/acfs/aros/DH0" "$OUT/DH0"

HF5_DIR="$ROOT/vendor/amigachrome-guest/build/hf5-autoboot"
FLATTENER="$HF5_DIR/elf_flatten.py"
BOOTROM_SOURCE="$HF5_DIR/acstorage_bootrom.s"
[ -f "$FLATTENER" ] || { echo "Native autoboot flattener not found: $FLATTENER" >&2; exit 6; }
[ -f "$BOOTROM_SOURCE" ] || { echo "Native autoboot ROM source not found: $BOOTROM_SOURCE" >&2; exit 6; }

rm -f \
  "$OUT/acscsi.flat" "$OUT/acscsi.meta.json" "$OUT/acscsi.reloc.bin" "$OUT/acscsi.inc" \
  "$OUT/acfs.flat" "$OUT/acfs.meta.json" "$OUT/acfs.reloc.bin" "$OUT/acfs.inc" \
  "$OUT/module_offsets.inc" "$OUT/acstorage_bootrom.s" "$OUT/acstorage_bootrom.o" \
  "$OUT/acstorage-boot.rom" "$OUT/ACSTORAGE_BOOTROM_DISASSEMBLY.txt"

python3 "$FLATTENER" "$SCSI" \
  --out "$OUT/acscsi.flat" \
  --meta "$OUT/acscsi.meta.json" \
  --relocs "$OUT/acscsi.reloc.bin" \
  --include "$OUT/acscsi.inc" \
  --symbol acscsi_ROMTag \
  --symbol acscsi_End

python3 "$FLATTENER" "$ACFS" \
  --out "$OUT/acfs.flat" \
  --meta "$OUT/acfs.meta.json" \
  --relocs "$OUT/acfs.reloc.bin" \
  --include "$OUT/acfs.inc" \
  --symbol acfs_ROMTag \
  --symbol acfs_End

cat "$OUT/acscsi.inc" "$OUT/acfs.inc" > "$OUT/module_offsets.inc"
cp "$BOOTROM_SOURCE" "$OUT/acstorage_bootrom.s"

AS="$(find "$AROS_BUILD" "$AROS_TOOLCHAIN_DIR" -type f \( -name 'm68k-aros-as' -o -name 'm68k-unknown-aros-as' \) -perm -111 -print -quit)"
OBJCOPY="$(find "$AROS_BUILD" "$AROS_TOOLCHAIN_DIR" -type f \( -name 'm68k-aros-objcopy' -o -name 'm68k-unknown-aros-objcopy' \) -perm -111 -print -quit)"
OBJDUMP="$(find "$AROS_BUILD" "$AROS_TOOLCHAIN_DIR" -type f \( -name 'm68k-aros-objdump' -o -name 'm68k-unknown-aros-objdump' \) -perm -111 -print -quit)"
[ -n "$AS" ] && [ -x "$AS" ] || { echo "m68k AROS assembler not found" >&2; exit 6; }
[ -n "$OBJCOPY" ] && [ -x "$OBJCOPY" ] || { echo "m68k AROS objcopy not found" >&2; exit 6; }
[ -n "$OBJDUMP" ] && [ -x "$OBJDUMP" ] || { echo "m68k AROS objdump not found" >&2; exit 6; }

(
  cd "$OUT"
  "$AS" -m68020 -I. -o acstorage_bootrom.o acstorage_bootrom.s
  "$OBJDUMP" -dr acstorage_bootrom.o > ACSTORAGE_BOOTROM_DISASSEMBLY.txt
  "$OBJCOPY" -O binary -j .text acstorage_bootrom.o acstorage-boot.rom
)

rom_size="$(stat -c %s "$OUT/acstorage-boot.rom")"
[ "$rom_size" -ge 64 ] || { echo "Native autoboot ROM is unexpectedly small: $rom_size" >&2; exit 6; }
[ "$rom_size" -le 65792 ] || { echo "Native autoboot ROM exceeds ACStorage aperture: $rom_size" >&2; exit 6; }
rom_first="$(od -An -tx1 -N1 "$OUT/acstorage-boot.rom" | tr -d '[:space:]')"
[ "$rom_first" = "90" ] || { echo "Native autoboot ROM does not start with Diag-valid 0x90 byte: $rom_first" >&2; exit 6; }

sha256sum \
  "$OUT/acatapi.device" \
  "$OUT/acscsi.device" \
  "$OUT/ACFS" \
  "$OUT/acstorage-boot.rom" \
  "$OUT/acscsi.flat" \
  "$OUT/acfs.flat" \
  "$OUT/acscsi.reloc.bin" \
  "$OUT/acfs.reloc.bin" \
  "$OUT/acscsi.meta.json" \
  "$OUT/acfs.meta.json" \
  "$OUT/CD0" \
  "$OUT/DH0" > "$OUT/SHA256SUMS"

python3 "$ROOT/scripts/build_aros_boot_resource.py" \
  --aros-tree "$AROS_TREE" \
  --acatapi-device "$OUT/acatapi.device" \
  --acscsi-device "$OUT/acscsi.device" \
  --acfs-handler "$OUT/ACFS" \
  --dh0-driver "$OUT/DH0" \
  --aros-license "$AROS_SRC/LICENSE" \
  2>&1 | tee "$LOG_DIR/bootstrap-adf.log"
python3 "$ROOT/scripts/build_boot_folder_resource.py" \
  --aros-tree "$AROS_TREE" \
  --acatapi-device "$OUT/acatapi.device" \
  --acscsi-device "$OUT/acscsi.device" \
  --acfs-handler "$OUT/ACFS" \
  --dh0-driver "$OUT/DH0" \
  2>&1 | tee "$LOG_DIR/bootstrap-folder.log"

python3 - "$ROOT" "$OUT" "$AROS_REF" "$AROS_TARGET" "$AROS_TOOLCHAIN_DIR" "$AROS_TOOLCHAIN_MARKER" <<'PY'
import hashlib,json,os,sys,time
from pathlib import Path
root,out,ref,target,toolchain,marker=Path(sys.argv[1]),Path(sys.argv[2]),sys.argv[3],sys.argv[4],Path(sys.argv[5]),Path(sys.argv[6])
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):
            h.update(b)
    return h.hexdigest()
files=[
 root/'vendor/amigachrome-guest/devices/acatapi.device/aros/acatapi_device.c',
 root/'vendor/amigachrome-guest/devices/acatapi.device/aros/acatapi_base.h',
 root/'vendor/amigachrome-guest/devices/acatapi.device/core/acatapi_core.c',
 root/'vendor/amigachrome-guest/devices/acatapi.device/core/acatapi_scsi.c',
 root/'vendor/amigachrome-guest/devices/acscsi.device/aros/acscsi_device.c',
 root/'vendor/amigachrome-guest/devices/acscsi.device/aros/acscsi_base.h',
 root/'vendor/amigachrome-guest/devices/acscsi.device/core/acstorage_core.c',
 root/'vendor/amigachrome-guest/devices/acscsi.device/core/acstorage_core.h',
 root/'vendor/amigachrome-guest/filesystems/acfs/aros/acfs_handler.c',
 root/'vendor/amigachrome-guest/filesystems/acfs/core/acfs_core.c',
 root/'vendor/amigachrome-guest/filesystems/acfs/core/acfs_core.h',
 root/'vendor/amigachrome-guest/common/protocol/acstorage.h',
 root/'vendor/amigachrome-guest/build/hf5-autoboot/elf_flatten.py',
 root/'vendor/amigachrome-guest/build/hf5-autoboot/acstorage_bootrom.s',
]
prov={
 'schema':2,
 'builtAt':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
 'arosRef':ref,
 'arosTarget':target,
 'devices':{
   'acatapi':{'path':str(out/'acatapi.device'),'sha256':sha(out/'acatapi.device')},
   'acscsi':{'path':str(out/'acscsi.device'),'sha256':sha(out/'acscsi.device')},
   'acfs':{'path':str(out/'ACFS'),'sha256':sha(out/'ACFS')},
   'acstorageAutobootRom':{'path':str(out/'acstorage-boot.rom'),'sha256':sha(out/'acstorage-boot.rom')},
 },
 'toolchain':{
   'prefix':os.path.realpath(toolchain),
   'markerSha256':sha(marker),
   'marker':json.load(marker.open(encoding='utf-8')),
 },
 'guestSources':{str(p.relative_to(root)):sha(p) for p in files},
}
(out/'BUILD_PROVENANCE.json').write_text(json.dumps(prov,indent=2)+'\n')
PY

echo "Local AROS guest stack, native ACStorage autoboot ROM and bootstrap resources rebuilt successfully."
echo "Device: $OUT/acatapi.device"
echo "Autoboot ROM: $OUT/acstorage-boot.rom"
echo "Toolchain: $AROS_TOOLCHAIN_DIR"

[executed on device: daletop (557d2ffd-2bd4-42f8-8777-9a5024193e1e)]