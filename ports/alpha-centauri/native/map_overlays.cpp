/* SPDX-License-Identifier: GPL-3.0-or-later. Attribution in map_overlays.h. */
#include "map_overlays.h"
#include <string.h>
namespace alpha { namespace {
struct Canvas { uint8_t* rgb;int w,h; };
void put(const Canvas& c,int x,int y,const uint8_t* rgb){if(x<0 || y<0 || x>=c.w || y>=c.h)return;uint8_t* p=c.rgb+((size_t)y*c.w+x)*3;p[0]=rgb[0];p[1]=rgb[1];p[2]=rgb[2];}
void brush(const Canvas& c,int x,int y,int size,const uint8_t* rgb){const int lo=-(size-1)/2;for(int dy=lo;dy<lo+size;++dy)for(int dx=lo;dx<lo+size;++dx)put(c,x+dx,y+dy,rgb);}
void line(const Canvas& c,int x0,int y0,int x1,int y1,int size,const uint8_t* rgb){
 int dx=x1>x0?x1-x0:x0-x1,dy=y1>y0?y0-y1:y1-y0,sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx+dy;
 for(int guard=0;guard<8192;++guard){brush(c,x0,y0,size,rgb);if(x0==x1 && y0==y1)break;const int e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}}
}
bool inside_diamond(int dx,int dy,int zoom){
 // Same integer diamond as the flat terrain quad: u,v in [0,64*zoom).
 const int u=dx+2*dy+32*zoom,v=2*dy-dx+32*zoom;return u>=0 && v>=0 && u<64*zoom && v<64*zoom;
}
void fill_diamond(const Canvas& c,int cx,int cy,int zoom,const uint8_t* rgb){
 for(int dy=-16*zoom;dy<16*zoom;++dy)for(int dx=-32*zoom;dx<32*zoom;++dx)if(inside_diamond(dx,dy,zoom))put(c,cx+dx,cy+dy,rgb);
}
bool texture_key(const uint8_t* rgb){return !rgb || (rgb[0]==125 && rgb[1]==0 && rgb[2]==128);}
bool sprite_key(const uint8_t* rgb){
 return !rgb || (rgb[0]==152 && rgb[1]==24 && rgb[2]==228) || (rgb[0]==100 && rgb[1]==16 && rgb[2]==156) || (rgb[0]==24 && rgb[1]==184 && rgb[2]==228) ||
  (rgb[0]==125 && rgb[1]==0 && rgb[2]==128) || (rgb[0]==255 && rgb[1]==0 && rgb[2]==255);
}
/* 56x56 texture.pcx cell through the flat diamond, keyed transparency. */
unsigned texture_cell(const Canvas& c,const PcxImage& t,int cx,int cy,int zoom,unsigned ax,unsigned ay){
 if(t.width()<ax+56 || t.height()<ay+56)return 0;
 unsigned n=0;
 for(int dy=-16*zoom;dy<16*zoom;++dy)for(int dx=-32*zoom;dx<32*zoom;++dx){
  const int u=dx+2*dy+32*zoom,v=2*dy-dx+32*zoom;if(u<0 || v<0 || u>=64*zoom || v>=64*zoom)continue;
  const uint8_t* rgb=t.rgb(ax+(unsigned)(u*56/(64*zoom)),ay+(unsigned)(v*56/(64*zoom)));if(texture_key(rgb))continue;put(c,cx+dx,cy+dy,rgb);++n;}
 return n;
}
/* 100x62 ter1.pcx cell, anchor (50,25) on the tile centre, scaled to 64*zoom. */
unsigned ter1_cell(const Canvas& c,const PcxImage& t,int cx,int cy,int zoom,unsigned ax,unsigned ay){
 if(t.width()<ax+100 || t.height()<ay+62)return 0;
 unsigned n=0;const int w=64*zoom,h=62*64*zoom/100;
 for(int y=0;y<h;++y)for(int x=0;x<w;++x){const uint8_t* rgb=t.rgb(ax+(unsigned)(x*100/w),ay+(unsigned)(y*62/h));if(sprite_key(rgb))continue;put(c,cx-w/2+x,cy-25*w/100+y,rgb);++n;}
 return n;
}
struct Improvement { uint32_t bit;unsigned land_x,land_y,water_x,water_y;uint8_t rgb[3];char glyph; };
// ter1.pcx cells (land, water) and fallback glyph colours.
const Improvement improvements[]={
 {map_bit::soil_enricher,822,453,822,453,{96,176,64},'d'},
 {map_bit::mine,607,64,506,64,{150,150,150},'t'},
 {map_bit::solar,607,127,506,127,{232,200,48},'s'},
 {map_bit::condenser,506,253,506,253,{90,160,230},'s'},
 {map_bit::mirror,607,253,607,253,{224,224,250},'s'},
 {map_bit::borehole,708,253,708,253,{204,92,32},'o'},
 {map_bit::sensor,708,316,708,316,{240,240,240},'m'},
 {map_bit::bunker,506,316,506,316,{124,112,92},'b'},
 {map_bit::airbase,607,316,607,316,{200,200,210},'x'},
 {map_bit::monolith,304,0,304,0,{36,36,40},'l'},
 {map_bit::supply_pod,519,379,418,379,{224,184,64},'p'},
};
void glyph(const Canvas& c,int cx,int cy,int zoom,char kind,const uint8_t* rgb){
 const int s=4*zoom;
 switch(kind){
  case 't':for(int y=0;y<=2*s;++y)line(c,cx-y/2,cy+s-2*s+y,cx+y/2,cy+s-2*s+y,1,rgb);break; // triangle
  case 's':for(int y=-s/2;y<=s/2;++y)line(c,cx-s,cy+y,cx+s,cy+y,1,rgb);break;               // panel
  case 'o':for(int y=-s;y<=s;++y)for(int x=-s;x<=s;++x)if(x*x+y*y<=s*s)put(c,cx+x,cy+y,rgb);break;
  case 'm':line(c,cx,cy+s,cx,cy-2*s,zoom,rgb);for(int y=-s;y<=s;++y)for(int x=-s;x<=s;++x)if(x*x+y*y<=s*s && x*x+y*y>=(s-zoom)*(s-zoom))put(c,cx+x,cy-2*s+y,rgb);break;
  case 'b':for(int y=-s;y<=0;++y)line(c,cx-s,cy+y,cx+s,cy+y,1,rgb);break;
  case 'x':line(c,cx-s,cy-s/2,cx+s,cy+s/2,zoom,rgb);line(c,cx-s,cy+s/2,cx+s,cy-s/2,zoom,rgb);break;
  case 'l':for(int y=-3*s;y<=0;++y)line(c,cx-s/3,cy+y,cx+s/3,cy+y,1,rgb);break;
  case 'p':for(int y=-s/2;y<=s/2;++y){const int half=s/2-(y<0?-y:y);line(c,cx-half,cy+y,cx+half,cy+y,1,rgb);}break;
  default:for(int k=-1;k<=1;++k)for(int j=-1;j<=1;++j)brush(c,cx+j*s,cy+k*s/2,zoom,rgb);break; // dots
 }
}
unsigned variation(int x,int y){return ((unsigned)x*7u+(unsigned)y*3u)&15u;}
// Diagonal edge neighbours (x+-1,y+-1) share a diamond edge; (x+-2,y),(x,y+-2) a corner.
const int edge_dx[4]={1,1,-1,-1},edge_dy[4]={-1,1,1,-1};
const int all_dx[8]={1,2,1,0,-1,-2,-1,0},all_dy[8]={-1,0,1,2,1,0,-1,-2};
}
const OriginalMapTile* map_tile_at(const MapTiles& m,int x,int y){
 if(!m.tiles || y<0 || y>=(int)m.height || !m.width)return nullptr;
 if(!m.flat){const int w=(int)m.width;x%=w;if(x<0)x+=w;}
 if(x<0 || x>=(int)m.width || ((x+y)&1))return nullptr;
 const size_t i=((size_t)y*m.width+(size_t)x)/2;return i<m.count?m.tiles+i:nullptr;
}
bool map_screen_position(const MapTiles& m,const MapCamera& c,int x,int y,int& sx,int& sy){
 int dx=x-c.x;const int width=(int)m.width;
 if(!m.flat && width){if(dx>width/2)dx-=width;else if(dx< -width/2)dx+=width;}
 sx=(int)c.width/2+dx*32*c.zoom;sy=(int)c.height/2+(y-c.y)*16*c.zoom;
 return sx>=-64*c.zoom && sx<(int)c.width+64*c.zoom && sy>=-64*c.zoom && sy<(int)c.height+64*c.zoom;
}
bool map_tile_explored(const OriginalMapTile& t,int viewer){return viewer<0 || viewer>7 || (t.visibility>>viewer&1u);}
uint32_t map_tile_items(const OriginalMapTile& t,int viewer){return viewer>=1 && viewer<=7?t.visible_items[viewer-1]:t.items;}
bool map_tile_ocean(const OriginalMapTile& t){return (t.climate&0xe0u)<0x60;}
bool faction_border_colour(const PcxImage& art,uint8_t rgb[3]){
 const uint8_t* c=art.rgb(161,749);if(!c)return false;memcpy(rgb,c,3);return true;
}
bool draw_map_overlays(uint8_t* frame,size_t capacity,const MapTiles& m,const MapCamera& cam,const MapOverlayStyle& style,MapOverlayStats* stats){
 if(!frame || !m.tiles || !m.width || !m.height || m.width>4096 || m.height>4096 || cam.zoom<1 || cam.zoom>2 || !cam.width || !cam.height ||
    cam.width>4096 || cam.height>4096 || capacity<(size_t)cam.width*cam.height*3 || m.count<((size_t)m.width*m.height)/2)return false;
 MapOverlayStats s{};const Canvas c{frame,(int)cam.width,(int)cam.height};const int z=cam.zoom;
 const int rx=(int)cam.width/(64*z)+3,ry=(int)cam.height/(32*z)+3;
 // A wrapped map narrower than the view is visited once per column.
 int xlo=cam.x-rx,xhi=cam.x+rx;
 if(!m.flat && 2*rx+1>(int)m.width){xlo=cam.x-(int)m.width/2;xhi=xlo+(int)m.width-1;}
 static const uint8_t black[3]={0,0,0},river_rgb[3]={56,104,208},road_rgb[3]={150,112,64},tube_rgb[3]={176,216,224},tube_core[3]={64,96,120};
 const int viewer=style.viewer;
 // Pass 1 per tile: vegetation, rivers, roads, improvements, borders. Pass 2: fog.
 for(int y=cam.y-ry;y<=cam.y+ry;++y)for(int raw=xlo;raw<=xhi;++raw){
  const OriginalMapTile* t=map_tile_at(m,raw,y);if(!t)continue;int sx,sy;if(!map_screen_position(m,cam,raw,y,sx,sy))continue;
  ++s.tiles;if(!map_tile_explored(*t,viewer))continue;
  const uint32_t items=map_tile_items(*t,viewer);const bool ocean=map_tile_ocean(*t);const unsigned var=variation(raw,y);
  if(style.layers&layer_vegetation){
   if(style.texture && (items&map_bit::farm) && !ocean){s.art_cells+=texture_cell(c,*style.texture,sx,sy,z,775,219)?1:0;++s.farms;}
   else if((items&map_bit::farm) && !ocean){for(int k=-2;k<=2;++k)line(c,sx-12*z+4*k*z,sy-3*z+k*2*z,sx+4*k*z,sy+3*z+k*2*z,1,improvements[0].rgb);++s.farms;++s.glyphs;}
   if(items&map_bit::forest){++s.forest;if(style.texture)s.art_cells+=texture_cell(c,*style.texture,sx,sy,z,526+57*(var&3),6+57*(var>>2))?1:0;else{static const uint8_t g[3]={34,96,40};glyph(c,sx,sy,z,'t',g);++s.glyphs;}}
   if(items&map_bit::fungus){++s.fungus;if(style.texture)s.art_cells+=texture_cell(c,*style.texture,sx,sy,z,(ocean?508:280)+57*(var&3),516+57*(var>>2))?1:0;else{static const uint8_t g[3]={200,80,160};glyph(c,sx,sy,z,'d',g);++s.glyphs;}}
  }
  if((style.layers&layer_rivers) && (items&map_bit::river) && !ocean){
   ++s.rivers;bool linked=false;
   for(int e=0;e<4;++e){const OriginalMapTile* n=map_tile_at(m,raw+edge_dx[e],y+edge_dy[e]);if(!n || !map_tile_explored(*n,viewer))continue;
    if((map_tile_items(*n,viewer)&map_bit::river) || map_tile_ocean(*n)){line(c,sx,sy,sx+edge_dx[e]*16*z,sy+edge_dy[e]*8*z,2*z,river_rgb);++s.river_links;linked=true;}}
   if(!linked)brush(c,sx,sy,3*z,river_rgb);
  }
  if((style.layers&layer_roads) && (items&(map_bit::road|map_bit::magtube|map_bit::base))){
   const bool tube=(items&map_bit::magtube)!=0;bool linked=false;
   if(tube)++s.tubes;else ++s.roads;
   for(int e=0;e<8;++e){const OriginalMapTile* n=map_tile_at(m,raw+all_dx[e],y+all_dy[e]);if(!n || !map_tile_explored(*n,viewer))continue;
    const uint32_t ni=map_tile_items(*n,viewer);
    // Connections are drawn half-way from each side so each link is symmetric.
    const int ex=sx+all_dx[e]*16*z,ey=sy+all_dy[e]*8*z;
    if(tube && (ni&(map_bit::magtube|map_bit::base))){line(c,sx,sy,ex,ey,2*z,tube_rgb);line(c,sx,sy,ex,ey,1,tube_core);++s.tube_links;linked=true;}
    else if(ni&(map_bit::road|map_bit::magtube|map_bit::base)){if(!(items&map_bit::base) || (ni&(map_bit::road|map_bit::magtube))){line(c,sx,sy,ex,ey,z+1,road_rgb);++s.road_links;linked=true;}}
   }
   if(!linked && !(items&map_bit::base))brush(c,sx,sy,3*z,tube?tube_rgb:road_rgb);
  }
  if(style.layers&layer_improvements)for(const auto& im:improvements){
   if(!(items&im.bit))continue;
   if(im.bit==map_bit::mine)++s.mines;else if(im.bit==map_bit::solar)++s.solar;else if(im.bit==map_bit::sensor)++s.sensors;else ++s.other_improvements;
   unsigned drawn=0;
   if(style.ter1)drawn=ter1_cell(c,*style.ter1,sx,sy,z,ocean?im.water_x:im.land_x,ocean?im.water_y:im.land_y);
   if(drawn)++s.art_cells;else{glyph(c,sx,sy,z,im.glyph,im.rgb);++s.glyphs;}
  }
  if((style.layers&layer_borders) && t->owner>0 && t->owner<8 && (style.border_mask>>t->owner&1)){
   const uint8_t* rgb=style.borders[t->owner];
   for(int e=0;e<4;++e){const OriginalMapTile* n=map_tile_at(m,raw+edge_dx[e],y+edge_dy[e]);if(n && n->owner==t->owner)continue;
    // Edge between the two diamond corners shared with that neighbour, inset by one pixel.
    const int ax=sx+edge_dx[e]*32*z,ay=sy,bx=sx,by=sy+edge_dy[e]*16*z;
    line(c,ax-edge_dx[e]*2,ay,bx,by-edge_dy[e],z,rgb);++s.border_edges;}
  }
 }
 if(style.layers&layer_fog)for(int y=cam.y-ry;y<=cam.y+ry;++y)for(int raw=xlo;raw<=xhi;++raw){
  const OriginalMapTile* t=map_tile_at(m,raw,y);if(!t || map_tile_explored(*t,viewer))continue;int sx,sy;if(!map_screen_position(m,cam,raw,y,sx,sy))continue;
  fill_diamond(c,sx,sy,z,black);++s.unexplored;
 }
 if(stats)*stats=s;
 return true;
}
}
