/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in original_faction.h. */
#include "original_faction.h"
#include <type_traits>
namespace alpha {
static uint32_t u32(const uint8_t* p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
template<class T>static T field(const uint8_t* p){
 uint32_t n=sizeof(T)==1?p[0]:sizeof(T)==2?(uint32_t)p[0]|((uint32_t)p[1]<<8):u32(p);
 if constexpr(std::is_signed<T>::value){
  if constexpr(sizeof(T)==1)return n<=127?(T)n:(T)(-1-(int32_t)(255-n));
  if constexpr(sizeof(T)==2)return n<=32767?(T)n:(T)(-1-(int32_t)(65535-n));
  if constexpr(sizeof(T)==4)return n<=0x7fffffffu?(T)n:(T)(-1-(int32_t)(~n));
 }else return (T)n;
}
GameReadError original_faction_definition(const OriginalGameView& game,size_t index,OriginalFactionDefinition& output){
 const size_t size=0x59c;
 if(index>=8 || !game.faction_definitions.data || game.faction_definitions.size/size<=index)return GameReadError::bad_index;
 const uint8_t* p=game.faction_definitions.data+index*size;OriginalFactionDefinition result{};
 auto offset=[](size_t n){return n;};auto value=[](size_t,unsigned){return false;};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+offset(off));
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=value(off,i)?0:field<type>(p+offset(off)+i*sizeof(type));
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[offset(off)+i];result.name[count]=0;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned i=0;i<rows;++i)for(unsigned j=0;j<cols;++j)result.name[i][j]=field<type>(p+offset(off)+(i*cols+j)*sizeof(type));
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned i=0;i<rows;++i){for(unsigned j=0;j<cols;++j)result.name[i][j]=(char)p[offset(off)+i*cols+j];result.name[i][cols]=0;}
#define WIRE_GOALS(name,count,off) for(unsigned i=0;i<count;++i){const uint8_t* q=p+offset(off)+16*i;result.name[i]={field<int16_t>(q),field<int16_t>(q+2),field<int32_t>(q+4),field<int32_t>(q+8),field<int32_t>(q+12)};}
#include "original_faction_definition_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 output=result;return GameReadError::ok;
}
GameReadError original_faction_state(const OriginalGameView& game,size_t index,OriginalFactionState& output){
 const size_t size=game.data_version>=11?0x20cc:0x20bc;
 if(index>=8 || !game.factions.data || game.factions.size/size<=index)return GameReadError::bad_index;
 const uint8_t* p=game.factions.data+index*size;OriginalFactionState result{};
 const bool legacy=game.data_version<11;
 auto offset=[&](size_t n){return !legacy?n:n>=0x207c?n-16:n>=0x464?n-4:n;};
 auto value=[&](size_t off,unsigned i){return legacy && ((off==0x458 && i>=8) || (off==0x205c && i>=5));};
#define WIRE_SCALAR(type,name,off) result.name=field<type>(p+offset(off));
#define WIRE_ARRAY(type,name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=value(off,i)?0:field<type>(p+offset(off)+i*sizeof(type));
#define WIRE_STRING(name,count,off) for(unsigned i=0;i<count;++i)result.name[i]=(char)p[offset(off)+i];result.name[count]=0;
#define WIRE_MATRIX(type,name,rows,cols,off) for(unsigned i=0;i<rows;++i)for(unsigned j=0;j<cols;++j)result.name[i][j]=field<type>(p+offset(off)+(i*cols+j)*sizeof(type));
#define WIRE_TEXT_MATRIX(name,rows,cols,off) for(unsigned i=0;i<rows;++i){for(unsigned j=0;j<cols;++j)result.name[i][j]=(char)p[offset(off)+i*cols+j];result.name[i][cols]=0;}
#define WIRE_GOALS(name,count,off) for(unsigned i=0;i<count;++i){const uint8_t* q=p+offset(off)+16*i;result.name[i]={field<int16_t>(q),field<int16_t>(q+2),field<int32_t>(q+4),field<int32_t>(q+8),field<int32_t>(q+12)};}
#include "original_faction_state_fields.inc"
#undef WIRE_SCALAR
#undef WIRE_ARRAY
#undef WIRE_STRING
#undef WIRE_MATRIX
#undef WIRE_TEXT_MATRIX
#undef WIRE_GOALS
 output=result;return GameReadError::ok;
}
}
