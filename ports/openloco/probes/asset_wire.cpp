// Copyright (c) 2026 Dalsin Limited. MIT licensed.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Graphics/G1File.hpp"
#include <OpenLoco/Audio/CssFile.hpp>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

using Bytes = std::vector<std::byte>;
using OpenLoco::Utility::readLittleEndian;
static void put(Bytes& b, uint32_t v, size_t n) { for(size_t i=0;i<n;++i)b.push_back(std::byte((v>>(i*8))&255)); }
static Bytes load(const char* name) {
    std::ifstream f(name,std::ios::binary);if(!f)throw std::runtime_error("Opening input failed");
    std::vector<char> chars((std::istreambuf_iterator<char>(f)),{});
    Bytes b(chars.size());for(size_t i=0;i<chars.size();++i)b[i]=std::byte(static_cast<unsigned char>(chars[i]));return b;
}
static void save(const std::string& name,const Bytes& b) {
    std::ofstream f(name,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)throw std::runtime_error("Writing canonical output failed");
}
template<typename F>static void reject(F f) { bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed); }
static void selfTest() {
    using OpenLoco::Gfx::decodeG1File;
    using OpenLoco::Audio::decodeCssFile;
    Bytes bitmap;put(bitmap,1,4);put(bitmap,1,4);put(bitmap,0,4);put(bitmap,1,2);put(bitmap,1,2);put(bitmap,0xfffd,2);put(bitmap,4,2);put(bitmap,1,2);put(bitmap,0,2);put(bitmap,42,1);
    auto g=decodeG1File(bitmap,1);assert(g.elements[0].width==1 && g.elements[0].xOffset==-3 && g.pixels[0]==std::byte{42});
    reject([&]{decodeG1File(std::span(bitmap).first(7),1);});
    reject([&]{decodeG1File(bitmap,0);});
    auto bad=bitmap;bad[8]=std::byte{2};reject([&]{decodeG1File(bad,1);});
    bad=bitmap;bad[4]=std::byte{2};reject([&]{decodeG1File(bad,1);});
    bad=bitmap;bad[20]=std::byte{5};reject([&]{decodeG1File(bad,1);});
    Bytes css;put(css,1,4);put(css,8,4);put(css,2,4);put(css,1,2);put(css,1,2);put(css,22050,4);put(css,44100,4);put(css,2,2);put(css,16,2);put(css,0,2);put(css,0x1234,2);
    auto c=decodeCssFile(css);assert(c.size()==1 && c[0].format.nSamplesPerSec==22050 && c[0].pcm.size()==2);
    reject([&]{decodeCssFile(std::span(css).first(3));});
    bad=css;bad[0]=std::byte{255};reject([&]{decodeCssFile(bad);});
    bad=css;bad[4]=std::byte{4};reject([&]{decodeCssFile(bad);});
    bad=css;bad[8]=std::byte{3};reject([&]{decodeCssFile(bad);});
    bad=css;bad[26]=std::byte{8};reject([&]{decodeCssFile(bad);});
    std::puts("PASS synthetic little-endian fields, signed values, bitmap, CSS PCM and malformed/truncated ranges");
}
int main(int argc,char** argv) {
    try {
        selfTest();if(argc==1)return 0;
        if(argc!=4) {std::fprintf(stderr,"Usage: asset-wire G1.DAT CSS1.DAT output-prefix\n");return 2;}
        auto g1=load(argv[1]);auto g=OpenLoco::Gfx::decodeG1File(g1,0x4101a);Bytes canonical;
        put(canonical,g.header.numEntries,4);put(canonical,g.header.totalSize,4);
        for(const auto& e:g.elements){put(canonical,e.offset,4);put(canonical,static_cast<uint16_t>(e.width),2);put(canonical,static_cast<uint16_t>(e.height),2);put(canonical,static_cast<uint16_t>(e.xOffset),2);put(canonical,static_cast<uint16_t>(e.yOffset),2);put(canonical,static_cast<uint16_t>(e.flags),2);put(canonical,static_cast<uint16_t>(e.zoomOffset),2);}
        canonical.insert(canonical.end(),g.pixels.begin(),g.pixels.end());assert(canonical==g1);
        save(std::string(argv[3])+".g1",canonical);
        std::printf("PASS G1 entries=%lu payload=%lu canonical equals original wire bytes\n",static_cast<unsigned long>(g.elements.size()),static_cast<unsigned long>(g.pixels.size()));
        auto css=load(argv[2]);auto sounds=OpenLoco::Audio::decodeCssFile(css);canonical.clear();put(canonical,sounds.size(),4);
        size_t pcmBytes=0;
        for(const auto& sound:sounds){const auto& f=sound.format;put(canonical,sound.pcm.size(),4);put(canonical,f.wFormatTag,2);put(canonical,f.nChannels,2);put(canonical,f.nSamplesPerSec,4);put(canonical,f.nAvgBytesPerSec,4);put(canonical,f.nBlockAlign,2);put(canonical,f.wBitsPerSample,2);put(canonical,f.cbSize,2);canonical.insert(canonical.end(),sound.pcm.begin(),sound.pcm.end());pcmBytes+=sound.pcm.size();}
        save(std::string(argv[3])+".css",canonical);
        std::printf("PASS CSS sounds=%lu PCM bytes=%lu\n",static_cast<unsigned long>(sounds.size()),static_cast<unsigned long>(pcmBytes));return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
