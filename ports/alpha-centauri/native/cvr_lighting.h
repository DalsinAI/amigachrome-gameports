/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_LIGHTING_H
#define ALPHA_CVR_LIGHTING_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
constexpr size_t cvr_shade_lattice_size=24*2137;
/* Generate the reachable shade lattice from spherical geometry. No original
 * executable/table data is needed. Output must have exactly the stated size. */
bool build_cvr_shade_lattice(uint8_t* out,size_t size);
/* Light coefficients already transformed and scaled into object space. No matrix inverse,
 * scene light configuration, faction palette or camera is supplied here.
 * The original hemisphere fold and quantized orientation select 2048 normal
 * shades from the generated lattice. Coefficients are not normalized: original
 * animation matrices and intensity can change their magnitude. Rejects non-finite
 * coefficients; Z magnitude >=1 selects the polar endpoint.
 * Invalid input leaves output and both optional orientation indices unchanged.
 * Input lattice and output must not overlap. */
bool cvr_shades_for_light(const uint8_t* lattice,size_t lattice_size,
 float x,float y,float z,uint8_t* out,size_t size,
 unsigned* inclination=nullptr,unsigned* azimuth=nullptr);
}
#endif
