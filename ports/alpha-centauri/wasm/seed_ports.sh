#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Fill emscripten's port cache with SDL2, SDL2_ttf and their dependencies
# from git tags, for hosts where emscripten's own archive download is
# refused (it fetches github.com/.../archive/*.zip and release tarballs).
#
#   seed_ports.sh [WORKDIR]
#
# Needs em-config on PATH (emsdk's upstream/emscripten). Each source is a
# shallow git clone of the exact tag the installed emscripten port file names;
# it is copied to cache/ports/<port>/<subdir> with the port's URL marker, which
# is what emscripten checks before downloading. Run once per emsdk install.
set -eu
work=${1:-${TMPDIR:-/tmp}/alpha-wasm-ports}
cache=$(em-config CACHE)
emroot=$(dirname -- "$(command -v em-config)")
ports_py=$emroot/tools/ports
mkdir -p -- "$work" "$cache/ports"
# Read TAG/VERSION and the download URL from the installed port file so the
# seeded source always matches what that emscripten version would build.
port_value() { # FILE NAME
 sed -n "s/^$2 = ['\"]\\([^'\"]*\\)['\"].*/\\1/p" "$ports_py/$1.py" | head -n 1
}
seed() { # PORT REPO TAG SUBDIR URL
 port=$1 repo=$2 tag=$3 subdir=$4 url=$5
 dest=$cache/ports/$port
 if [ -f "$dest/.emscripten_url" ] && [ "$(cat "$dest/.emscripten_url")" = "$url" ]; then
  echo "seed_ports: $port already seeded ($tag)"; return 0
 fi
 src=$work/$subdir
 if [ ! -d "$src" ]; then
  git clone -q -c advice.detachedHead=false --depth 1 -b "$tag" "$repo" "$src"
 fi
 rm -rf -- "$dest" "$dest.tmp"
 mkdir -p -- "$dest.tmp"
 cp -R -- "$src" "$dest.tmp/$subdir"
 rm -rf -- "$dest.tmp/$subdir/.git"
 printf '%s\n' "$url" > "$dest.tmp/.emscripten_url"
 mv -- "$dest.tmp" "$dest"
 echo "seed_ports: $port <- $repo $tag"
}
zv=$(port_value zlib VERSION)
seed zlib https://github.com/madler/zlib "v$zv" "zlib-$zv" \
 "https://github.com/madler/zlib/archive/refs/tags/v$zv.tar.gz"
ft=$(port_value freetype TAG)
seed freetype https://github.com/freetype/freetype "$ft" "freetype-$ft" \
 "https://github.com/freetype/freetype/archive/$ft.zip"
hv=$(port_value harfbuzz VERSION)
seed harfbuzz https://github.com/harfbuzz/harfbuzz "$hv" "harfbuzz-$hv" \
 "https://github.com/harfbuzz/harfbuzz/releases/download/$hv/harfbuzz-$hv.tar.xz"
sv=$(port_value sdl2 VERSION)
seed sdl2 https://github.com/libsdl-org/SDL "release-$sv" "SDL-release-$sv" \
 "https://github.com/libsdl-org/SDL/archive/release-$sv.zip"
tt=$(port_value sdl2_ttf TAG)
seed sdl2_ttf https://github.com/libsdl-org/SDL_ttf "$tt" "SDL_ttf-$tt" \
 "https://github.com/libsdl-org/SDL_ttf/archive/$tt.zip"
