/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_TERRAIN_MATERIAL_H
#define ALPHA_TERRAIN_MATERIAL_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
struct TerrainMaterialSource {
 const uint8_t* pixels; size_t pixel_count;
 const uint8_t* rgb; size_t rgb_bytes;
};
// Original terrain PCX preparation after decoding: indices add10 modulo256;
// first236 source RGB entries replace destination entries10..245. Other
// destination palette entries are preserved. No nearest-palette conversion.
// Inputs/output/descriptors cannot overlap. Failure preserves both outputs.
// Not PCX parsing, allocation, texture dispatch or installation of OS palette.
bool prepare_terrain_material(const TerrainMaterialSource&,uint8_t* pixels,size_t bytes,uint8_t* rgb,size_t rgb_bytes);
}
#endif
