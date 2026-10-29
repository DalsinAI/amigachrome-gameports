/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_CVR_MODEL_H
#define ALPHA_CVR_MODEL_H
#include <stddef.h>
#include <stdint.h>
namespace alpha {
struct CvrVoxel { int16_t z,x,y;uint16_t normal;uint8_t color; };
struct CvrFrame { uint8_t flags;float location[3],matrix[9]; };
struct CvrObject { char name[64];float units;uint32_t count;CvrVoxel* voxels;CvrFrame* frames; };
/* Owned, bounded CVR 10.0/10.1/10.2 geometry and animation.
 * Chained ordered LOD levels select by original projected-scale threshold;
 * single streams and offset-free jump groups are supported.
 * Offset jump headers and unknown geometry flags are rejected.
 * No original pointers or renderer ABI are imported. */
class CvrModel {
public:
 CvrModel();~CvrModel();
 bool load(const void* data,size_t size);
 bool load_at_scale(const void* data,size_t size,float projected_scale);
 uint32_t object_count()const{return objects_;}
 uint32_t frame_count()const{return frames_;}
 uint32_t voxel_count()const{return voxels_;}
 const CvrObject* object(uint32_t i)const{return i<objects_?items_+i:nullptr;}
 const uint8_t* rgb(uint8_t color,unsigned shade)const;
 uint8_t shaded_index(uint8_t color,unsigned shade)const{return shade<24?shades_[shade*256+color]:0;}
 const uint8_t* palette_rgb(uint8_t index)const{return palette_+3*index;}
private:
 CvrModel(const CvrModel&)=delete;CvrModel& operator=(const CvrModel&)=delete;
 bool parse(const uint8_t* data,size_t size,float projected_scale);void clear();
 CvrObject* items_;uint32_t objects_,frames_,voxels_;uint8_t palette_[768],shades_[6144];
};
}
#endif
