#ifdef NDEBUG
#undef NDEBUG
#endif
#include <S5/S5Wire.hpp>
#include <S5/S5GameState.h>
#include <S5/S5Options.h>
#include <S5/SawyerStream.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

using Bytes = std::vector<std::byte>;
using namespace OpenLoco;
using namespace OpenLoco::S5;
static void put(Bytes& b, size_t at, uint32_t n, size_t width)
{
    for (size_t i = 0; i < width; ++i) b.at(at+i) = std::byte((n >> (i*8)) & 255);
}
template<class F> static void reject(F&& f)
{
    bool rejected = false;
    try { f(); } catch (const std::runtime_error&) { rejected = true; }
    assert(rejected);
}
static void selfTest()
{
    Bytes raw(sizeof(Header)+1, std::byte{0});
    auto unaligned = std::span(raw).subspan(1);
    Bytes bytes(sizeof(Header), std::byte{0xA5});
    bytes[0] = std::byte{1}; bytes[1] = std::byte{8};
    put(bytes, 2, 0x1234, 2); put(bytes, 4, Wire::kVersion, 4); put(bytes, 8, kMagicNumber, 4);
    std::copy(bytes.begin(), bytes.end(), unaligned.begin());
    const auto h = Wire::decodeHeader(unaligned);
    assert(h.type == S5Type::scenario && h.flags == HeaderFlags::hasSaveDetails);
    assert(h.numPackedObjects == 0x1234 && h.version == Wire::kVersion && h.magic == kMagicNumber);
    assert(h.padding[0] == std::byte{0xA5} && h.padding[19] == std::byte{0xA5});
    reject([&]{Wire::decodeHeader(std::span(bytes).first(31));});
    reject([&]{Wire::decodeHeader(raw);});
    Wire::requireGameStateSupport<std::endian::little>(h);
    reject([&]{Wire::requireGameStateSupport<std::endian::big>(h);});
    if constexpr (std::endian::native == std::endian::big)
        reject([&]{Wire::requireGameStateSupport(h);});
    auto bad = h; bad.version = 0;
    reject([&]{Wire::requireGameStateSupport<std::endian::little>(bad);});
    Bytes details(sizeof(SaveDetails), std::byte{0x5A});
    put(details, offsetof(SaveDetails,date), 0x12345678, 4);
    put(details, offsetof(SaveDetails,performanceIndex), 0x1234, 2);
    put(details, offsetof(SaveDetails,challengeFlags), 0x87654321, 4);
    const auto d = Wire::decodeSaveDetails(details);
    assert(d.date == 0x12345678 && d.performanceIndex == 0x1234 && enumValue(d.challengeFlags) == 0x87654321);
    assert(d.image[0] == 0x5A && d.image[49999] == 0x5A && d.company[255] == 0x5A);
    reject([&]{Wire::decodeSaveDetails(std::span(details).first(details.size()-1));});
    Bytes optionBytes(sizeof(Options),std::byte{0x5A});
    put(optionBytes,offsetof(Options,scenarioStartYear),1960,2);
    put(optionBytes,offsetof(Options,scenarioFlags),0x1234,2);
    put(optionBytes,offsetof(Options,numberOfForests),0x1234,2);
    put(optionBytes,offsetof(Options,numberRandomTrees),0x2345,2);
    put(optionBytes,offsetof(Options,objective)+offsetof(Objective,companyValue),0x12345678,4);
    put(optionBytes,offsetof(Options,objective)+offsetof(Objective,monthlyVehicleProfit),0x23456789,4);
    put(optionBytes,offsetof(Options,objective)+offsetof(Objective,deliveredCargoAmount),0x3456789A,4);
    for(auto offset:{offsetof(Options,scenarioText),offsetof(Options,objectiveDeliveredCargo),offsetof(Options,currency)}) {
        put(optionBytes,offset,0x12345678,4);put(optionBytes,offset+12,0x3456789A,4);
    }
    const auto options=Wire::decodeOptions(optionBytes);
    assert(options.scenarioStartYear==1960 && options.scenarioFlags==0x1234);
    assert(options.numberOfForests==0x1234 && options.numberRandomTrees==0x2345);
    assert(options.objective.companyValue==0x12345678 && options.objective.monthlyVehicleProfit==0x23456789 && options.objective.deliveredCargoAmount==0x3456789A);
    assert(options.scenarioText.flags==0x12345678 && options.currency.checksum==0x3456789A && options.objectiveDeliveredCargo.checksum==0x3456789A);
    assert(options.preview[0][0]==0x5A && options.preview[127][127]==0x5A && options.scenarioName[63]==0x5A);
    reject([&]{Wire::decodeOptions(std::span(optionBytes).first(optionBytes.size()-1));});
    Bytes output(16,std::byte{0xCC}), shortInput(7,std::byte{0});
    reject([&]{Wire::copyExact(shortInput,output.data(),8);});
    assert(output == Bytes(16,std::byte{0xCC}));
    reject([&]{Wire::copyExact(output,output.data(),8);});
    Wire::copyExact(shortInput,output.data(),7);
    assert(output[6] == std::byte{0} && output[7] == std::byte{0xCC});
    assert(Wire::tileCount(output,8,2) == 2);
    reject([&]{Wire::tileCount(shortInput,8,2);});
    reject([&]{Wire::tileCount(output,8,1);});
    reject([&]{Wire::tileCount(output,0,1);});
    Bytes state(0x436,std::byte{0});
    assert(Wire::isLegacyGameState(state));
    state[0x434] = std::byte{2}; assert(!Wire::isLegacyGameState(state));
    state[0x434] = std::byte{0}; state[0x435] = std::byte{1}; assert(Wire::isLegacyGameState(state));
    reject([&]{Wire::isLegacyGameState(std::span(state).first(0x435));});
    std::puts("PASS S5 unaligned header, save metadata, exact sizes, tile bounds, legacy flags and endian gate");
}
int main(int argc, char** argv)
{
    try {
        selfTest();
        for (int i = 1; i < argc; ++i) {
            std::ifstream f(argv[i],std::ios::binary);
            if (!f) throw std::runtime_error("Could not open S5 input");
            const std::vector<char> input((std::istreambuf_iterator<char>(f)),{});
            MemoryStream memory;
            memory.write(input.data(),input.size()); memory.setPosition(0);
            SawyerStreamReader reader(memory);
            assert(reader.validateChecksum());
            const auto header = Wire::decodeHeader(reader.readChunk());
            std::printf("S5 input %d type=%u flags=%u packed=%u\n",i,unsigned(header.type),unsigned(header.flags),unsigned(header.numPackedObjects));
            assert(header.version == Wire::kVersion);
            if(header.hasFlags(HeaderFlags::hasSaveDetails)) Wire::decodeSaveDetails(reader.readChunk());
            if(header.type == S5Type::scenario) {
                Wire::decodeOptions(reader.readChunk());
            }
            for(unsigned j=0;j<header.numPackedObjects;++j) {
                std::array<std::byte,16> object{};reader.read(object.data(),object.size());reader.readChunk();
            }
            Wire::requireSize(reader.readChunk(),859*16);
            bool hasTiles = true;
            if(header.type == S5Type::scenario) {
                const auto general = reader.readChunk();
                Wire::requireSize(general,sizeof(GeneralState));
                hasTiles = (Utility::readLittleEndian<uint32_t>(general,offsetof(GeneralState,flags)) & 1) != 0;
                Wire::requireSize(reader.readChunk(),offsetof(S5::GameState,entities)-offsetof(S5::GameState,towns));
                Wire::requireSize(reader.readChunk(),sizeof(S5::GameState)-offsetof(S5::GameState,animations));
            } else {
                const auto state = reader.readChunk();
                Wire::requireSize(state,Wire::isLegacyGameState(state)?sizeof(GameStateType2):sizeof(S5::GameState));
            }
            const auto tiles=hasTiles?Wire::tileCount(reader.readChunk(),8,0x6C000):0;
            std::printf("PASS S5 file %d type=%u packed=%u version=%lu tiles=%lu\n",i,unsigned(header.type),unsigned(header.numPackedObjects),static_cast<unsigned long>(header.version),static_cast<unsigned long>(tiles));
        }
        return 0;
    } catch(const std::exception& e) { std::fprintf(stderr,"FAIL S5: %s\n",e.what());return 1; }
}
