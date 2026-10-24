#ifdef NDEBUG
#undef NDEBUG
#endif
#include <S5/S5Wire.hpp>
#include <S5/S5StateWire.hpp>
#include <S5/SawyerStream.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace OpenLoco;
using Bytes=std::vector<std::byte>;
namespace SW=OpenLoco::S5::StateWire;
static uint32_t digest=2166136261u;
static void add(uint8_t b){digest=(digest^b)*16777619u;}
template<class T> static void values(T& v) {
    if constexpr(std::is_array_v<T>) { for(auto& x:v)values(x); }
    else if constexpr(std::is_integral_v<T> || std::is_enum_v<T>) {
        if constexpr(sizeof(T)==1) {uint8_t n;std::memcpy(&n,&v,1);add(n);}
        else if constexpr(sizeof(T)==2) {uint16_t n;std::memcpy(&n,&v,2);for(int i=0;i<2;++i)add((n>>(i*8))&255);}
        else if constexpr(sizeof(T)==4) {uint32_t n;std::memcpy(&n,&v,4);for(int i=0;i<4;++i)add((n>>(i*8))&255);}
        else static_assert(sizeof(T)==0,"Unreviewed scalar width");
    } else if constexpr(SW::isVector<T>) {values(v.x);values(v.y);if constexpr(requires{v.z;})values(v.z);}
    else SW::fields(v,[](auto& x){values(x);});
}
template<class T> static void checkState(Bytes wire) {
    auto native=std::make_unique<T>();
    S5::Wire::copyExact(wire,native.get(),sizeof(T));
    SW::native(*native);
    values(*native);
    assert(SW::encode(*native)==wire);
    // A forced swap must be an involution on every active field.
    SW::swapAll(*native);SW::swapAll(*native);
    assert(SW::encode(*native)==wire);
}
static void selfTest() {
    auto s=std::make_unique<S5::GameState>();
    for(auto& e:s->entities)e.base.baseType=0xff;
    s->general.currentYear=1960;s->general.rng[0]=0x12345678;s->general.savedViewX=-1234;
    s->companies[0].cash=currency48_t(static_cast<int64_t>(0x123456789abLL));
    s->routings[999][63]=0x4321;
    s->towns[0].name=0x87fb;s->towns[0].population=0x12345678;
    s->stations[0].name=0x8123;s->industries[0].name=0x8234;
    S5::VehicleHead head{};head.base.baseType=0;head.base.type=0;head.base.id=0x1234;
    std::memcpy(&s->entities[0],&head,sizeof head);
    auto before=Bytes(sizeof(*s));std::memcpy(before.data(),s.get(),before.size());
    SW::swapAll(*s);SW::swapAll(*s);
    assert(std::memcmp(before.data(),s.get(),before.size())==0);
    auto wire=SW::encode(*s);
    // Independent wire-byte assertions catch a symmetric but incomplete walk.
    const size_t townOffset=0x092444;
    assert(wire[townOffset]==std::byte{0xfb} && wire[townOffset+1]==std::byte{0x87});
    auto restored=std::make_unique<S5::GameState>();std::memcpy(restored.get(),wire.data(),wire.size());SW::native(*restored);
    assert(restored->general.currentYear==1960 && restored->general.savedViewX==-1234 && restored->general.rng[0]==0x12345678);
    assert(restored->towns[0].name==0x87fb && restored->towns[0].population==0x12345678);
    assert(restored->stations[0].name==0x8123 && restored->industries[0].name==0x8234);
    assert(restored->companies[0].cash.asInt64()==0x123456789abLL && restored->entities[0].base.id==0x1234 && restored->routings[999][63]==0x4321);
    // Multi-byte packed map fields are selected by their byte discriminator.
    S5::TileElement t{};auto raw=t.rawData();raw[0]=2<<2;raw[2]=1;raw[6]=0x34;raw[7]=0x12;
    SW::native(t);assert(t.as<S5::StationElement>()->stationId()==0x234);
    auto encoded=SW::encode(t);assert(encoded[6]==std::byte{0x34}&&encoded[7]==std::byte{0x12});
    std::puts("PASS S5 typed state, currency48, routing, vehicle and packed tile round trips");
}
int main(int argc,char** argv) {
    try {
        selfTest();
        for(int i=1;i<argc;++i) {
            std::ifstream f(argv[i],std::ios::binary);if(!f)throw std::runtime_error("Input not found");
            std::vector<char> bytes((std::istreambuf_iterator<char>(f)),{});
            MemoryStream mem;mem.write(bytes.data(),bytes.size());mem.setPosition(0);SawyerStreamReader r(mem);assert(r.validateChecksum());
            auto header=S5::Wire::decodeHeader(r.readChunk());assert(header.version==S5::Wire::kVersion);
            if(header.hasFlags(S5::HeaderFlags::hasSaveDetails))S5::Wire::decodeSaveDetails(r.readChunk());
            if(header.type==S5::S5Type::scenario)S5::Wire::decodeOptions(r.readChunk());
            for(unsigned j=0;j<header.numPackedObjects;++j){std::array<std::byte,16> h{};r.read(h.data(),h.size());r.readChunk();}
            auto objects=r.readChunk();assert(objects.size()==859*16);
            digest=2166136261u;bool hasTiles=true;
            if(header.type==S5::S5Type::scenario) {
                Bytes state(sizeof(S5::GameState),std::byte{0});
                auto copy=[&](size_t offset,size_t size){auto b=r.readChunk();S5::Wire::copyExact(b,state.data()+offset,size);};
                copy(0,sizeof(S5::GeneralState));hasTiles=(Utility::readLittleEndian<uint32_t>(state,0x10)&1)!=0;
                copy(0x092444,0x123480);copy(0x4268c4,0x79d80);
                checkState<S5::GameState>(std::move(state));
            } else {
                auto chunk=r.readChunk();Bytes state(chunk.begin(),chunk.end());
                if(S5::Wire::isLegacyGameState(state))checkState<S5::GameStateType2>(std::move(state));
                else checkState<S5::GameState>(std::move(state));
            }
            const uint32_t stateDigest=digest;size_t tiles=0;
            if(hasTiles){auto b=r.readChunk();tiles=S5::Wire::tileCount(b,8,0x6c000);std::vector<S5::TileElement> native(tiles);std::memcpy(native.data(),b.data(),b.size());for(auto& t:native)SW::native(t);assert(SW::encodeArray<S5::TileElement>(native)==Bytes(b.begin(),b.end()));for(auto& t:native)values(t);}
            std::printf("PASS S5 file=%d state=%08lx combined=%08lx tiles=%lu\n",i,(unsigned long)stateDigest,(unsigned long)digest,(unsigned long)tiles);
        }
        return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"S5_STATE_FAIL %s\n",e.what());return 1;}
}
