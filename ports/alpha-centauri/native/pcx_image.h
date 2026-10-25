/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_PCX_IMAGE_H
#define AC_ALPHA_PCX_IMAGE_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
/* Bounded eight-bit, one-plane PCX import. Owns indexed pixels and RGB palette;
 * no file pointers survive. A failed import preserves the previous image. */
class PcxImage {
 public:
  PcxImage()=default;~PcxImage();
  PcxImage(const PcxImage&)=delete;PcxImage& operator=(const PcxImage&)=delete;
  bool load(const uint8_t* data,size_t size);
  uint32_t width()const{return width_;}uint32_t height()const{return height_;}
  const uint8_t* pixels()const{return pixels_;}const uint8_t* palette()const{return palette_;}
  const uint8_t* rgb(uint32_t x,uint32_t y)const;
 private:
  uint8_t* pixels_=nullptr;uint8_t palette_[768]{};uint32_t width_=0,height_=0;
};
}
#endif
