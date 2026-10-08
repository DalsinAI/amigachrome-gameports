#!/usr/bin/env bash
set -euo pipefail
# shellcheck source=/dev/null
source "$(dirname "$0")/common.sh"
INSTALL_DEPS=0
OFFLINE=0
REBUILD_TOOLCHAIN=0
SKIP_BOOT=0
for arg in "$@"; do
  case "$arg" in
    --install-deps) INSTALL_DEPS=1 ;;
    --offline) OFFLINE=1 ;;
    --rebuild-toolchain) REBUILD_TOOLCHAIN=1 ;;
    --skip-boot) SKIP_BOOT=1 ;;
    *) echo "Unknown option: $arg" >&2; exit 2 ;;
  esac
done

if [ "$INSTALL_DEPS" -eq 1 ]; then
  mapfile -t pkgs < <(grep -vE '^\s*(#|$)' "$TOOLS_DIR/deps-ubuntu.txt")
  if command -v sudo >/dev/null 2>&1; then SUDO=sudo; else SUDO=; fi
  $SUDO apt-get update
  DEBIAN_FRONTEND=noninteractive $SUDO apt-get install -y --no-install-recommends "${pkgs[@]}"
fi

missing=()
for c in git make gcc g++ gawk bison flex perl python3 cmake automake autoconf gperf ccache nasm xorriso mcopy jlha wget; do
  command -v "$c" >/dev/null 2>&1 || missing+=("$c")
done
if [ "${#missing[@]}" -ne 0 ]; then
  echo "Missing local build tools: ${missing[*]}" >&2
  echo "On Ubuntu run: ./BUILD-AROS-LOCAL.sh --install-deps" >&2
  exit 4
fi

if [ ! -d "$AROS_SRC" ]; then
  if [ -f "$SNAPSHOT" ]; then
    "$TOOLS_DIR/restore-inputs.sh"
  elif [ "$OFFLINE" -eq 1 ]; then
    echo "Offline mode requested but no AROS source/input snapshot exists at $SNAPSHOT" >&2
    exit 5
  else
    git clone --recursive "$AROS_REPOSITORY" "$AROS_SRC"
    git -C "$AROS_SRC" checkout --detach "$AROS_REF"
    git -C "$AROS_SRC" submodule update --init --recursive
    printf '%s\n' "$AROS_REF" > "$AROS_SRC/.amigachrome-source-ref"
  fi
fi
check_source_ref
mkdir -p "$AROS_PORTS"

if [ "$REBUILD_TOOLCHAIN" -eq 1 ]; then
  echo "Forcing local AROS toolchain rebuild at $AROS_TOOLCHAIN_DIR"
  rm -rf "$AROS_TOOLCHAIN_DIR" "$AROS_TOOLCHAIN_BUILD"
fi

if ! check_toolchain_marker >/dev/null 2>&1; then
  if [ -f "$TOOLCHAIN_SNAPSHOT" ] && [ "$REBUILD_TOOLCHAIN" -eq 0 ]; then
    "$TOOLS_DIR/restore-toolchain.sh"
  fi
fi

if ! check_toolchain_marker >/dev/null 2>&1; then
  if [ "$OFFLINE" -eq 1 ]; then
    echo "Offline mode requested but no compatible toolchain is installed/restorable." >&2
    echo "Expected toolchain prefix: $AROS_TOOLCHAIN_DIR" >&2
    echo "Expected snapshot: $TOOLCHAIN_SNAPSHOT" >&2
    exit 5
  fi

  echo "Building pinned AROS cross-toolchain once at $AROS_TOOLCHAIN_DIR"
  rm -rf "$AROS_TOOLCHAIN_BUILD" "$AROS_TOOLCHAIN_DIR"
  mkdir -p "$AROS_TOOLCHAIN_BUILD" "$AROS_TOOLCHAIN_DIR"
  (
    cd "$AROS_TOOLCHAIN_BUILD"
    "$AROS_SRC/configure" \
      --target="$AROS_TARGET" \
      --with-serial-debug=yes \
      --enable-ccache \
      --with-portssources="$AROS_PORTS" \
      --with-aros-toolchain-install="$AROS_TOOLCHAIN_DIR"
  ) 2>&1 | tee "$LOG_DIR/toolchain-configure.log"
  ( cd "$AROS_TOOLCHAIN_BUILD" && make -j1 crosstools ) 2>&1 | tee "$LOG_DIR/toolchain-build.log"
  write_toolchain_marker
  check_toolchain_marker
  "$TOOLS_DIR/snapshot-toolchain.sh"
fi

# Configure needs every AmigaChrome module description present in the AROS make graph.
# Stage the complete guest stack before configure; build.sh stages it again so source
# changes made after preparation are still picked up without rebuilding the toolchain.
python3 "$ROOT/vendor/amigachrome-guest/scripts/stage_aros_module.py" "$AROS_SRC"

fingerprint="$(build_config_fingerprint)"
fingerprint_file="$AROS_BUILD/.amigachrome-config-fingerprint"
current_fingerprint=""
[ -f "$fingerprint_file" ] && current_fingerprint="$(tr -d '\r\n' < "$fingerprint_file")"
if [ ! -f "$AROS_BUILD/config.status" ] || [ "$current_fingerprint" != "$fingerprint" ]; then
  rm -rf "$AROS_BUILD"
  mkdir -p "$AROS_BUILD"
  (
    cd "$AROS_BUILD"
    "$AROS_SRC/configure" \
      --target="$AROS_TARGET" \
      --with-serial-debug=yes \
      --enable-ccache \
      --with-portssources="$AROS_PORTS" \
      --with-aros-toolchain-install="$AROS_TOOLCHAIN_DIR" \
      --with-aros-toolchain=yes
  ) 2>&1 | tee "$LOG_DIR/configure.log"
  printf '%s\n' "$fingerprint" > "$fingerprint_file"
fi

# Build the emergency tree once unless this consumer only needs the compiler/SDK.
# Game-port preparation intentionally skips the boot floppy: it is not a game
# dependency, and the floppy image has its own fixed capacity unrelated to host
# filesystem free space.
if [ "$SKIP_BOOT" -eq 0 ]; then
  if ! find "$AROS_BUILD" -type d -name Emergency-Boot -print -quit | grep -q .; then
    ( cd "$AROS_BUILD" && make -j1 "$AROS_BOOT_TARGET" ) 2>&1 | tee "$LOG_DIR/boot-tree.log"
  fi
else
  echo "Skipping AROS emergency boot/floppy target for SDK-only preparation."
fi

# Capture source+ports after AROS has populated all build inputs.
if [ ! -f "$SNAPSHOT" ]; then
  "$TOOLS_DIR/snapshot-inputs.sh"
fi

check_toolchain_marker
echo "AROS local environment prepared: $WORK_ROOT"
echo "Reusable toolchain: $AROS_TOOLCHAIN_DIR"

