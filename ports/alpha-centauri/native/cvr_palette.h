/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_PALETTE_H
#define ALPHA_CVR_PALETTE_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
/* Construct the default gamma1 game palette: owner PCX slots0..235 become
 * game slots10..245 with channels capped235; reserved slots follow the normal
 * indexed Windows path. Alternate OS-system palette/custom gamma are excluded. */
bool build_cvr_default_global_palette(const uint8_t* pcx_rgb,size_t input_size,
 uint8_t* output,size_t capacity);
/* Build a24-shade x256-index faction table from an explicit global RGB palette,
 * neutral shade-index table and RGB tint. Original RGB multiplication truncates
 * each channel after /255; nearest palette colour uses unweighted squared RGB
 * distance and retains the lowest index on ties. Inputs/output may not overlap.
 * This does not initialise system slots, apply gamma or choose a faction tint. */
bool build_cvr_faction_palette(const uint8_t* global_rgb,size_t global_size,
 const uint8_t* neutral_shades,size_t neutral_size,const uint8_t* tint,size_t tint_size,
 uint8_t* output,size_t capacity);
}
#endif
