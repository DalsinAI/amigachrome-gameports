// Copyright (c) 2026 Dalsin Limited. MIT licensed.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "Objects/ObjectWire.hpp"
#include "Objects/ObjectWireFields.hpp"
#include "S5/SawyerStream.h"
#include <OpenLoco/Core/MemoryStream.h>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
using Bytes=std::vector<std::byte>;
static Bytes load(const std::string& name){std::ifstream f(name,std::ios::binary);if(!f)throw std::runtime_error("Opening object failed");std::vector<char> raw((std::istreambuf_iterator<char>(f)),{});Bytes bytes(raw.size());for(size_t i=0;i<raw.size();++i)bytes[i]=std::byte(static_cast<uint8_t>(raw[i]));return bytes;}
static void put(std::ofstream& out,uint32_t value){for(int i=0;i<4;++i)out.put(static_cast<char>((value>>(i*8))&255));}
static void selfTest()
{
 const std::array<std::byte,17> packed={std::byte{0},std::byte{0xab},std::byte{0x56},std::byte{0x34},std::byte{0x12},std::byte{'A'},std::byte{'B'},std::byte{'C'},std::byte{'D'},std::byte{'E'},std::byte{'F'},std::byte{'G'},std::byte{'H'},std::byte{0xef},std::byte{0xcd},std::byte{0xab},std::byte{0x89}};
 const auto bytes=std::span(packed).subspan(1);
 auto header=OpenLoco::ObjectWire::decodeHeader(bytes);
 assert(header.flags==0x123456ab && header.checksum==0x89abcdef);
 const auto encoded=OpenLoco::ObjectWire::encodeHeader(header);
 assert(std::equal(encoded.begin(),encoded.end(),bytes.begin()));
 bool rejected=false;
 try{OpenLoco::ObjectWire::decodeHeader(bytes.first(15));}catch(const std::runtime_error&){rejected=true;}
 assert(rejected);
 const Bytes payload={std::byte{1},std::byte{2},std::byte{3}};
 const auto sum=OpenLoco::ObjectWire::checksum(header,payload);
 header.flags=0xdeadbeab;
 assert(sum==OpenLoco::ObjectWire::checksum(header,payload));
 Bytes climate(sizeof(OpenLoco::ClimateObject),std::byte{7});
 climate[0]=std::byte{0x34};climate[1]=std::byte{0x12};
 const auto original=climate;
 OpenLoco::ObjectWire::decodeFixedFields(OpenLoco::ObjectType::climate,climate);
 OpenLoco::ClimateObject native{};std::memcpy(&native,climate.data(),sizeof native);
 assert(native.name==0x1234 && native.seasonLength[3]==7);
 assert(OpenLoco::ObjectWire::canonicalFixedFields(OpenLoco::ObjectType::climate,climate)==original);
 if constexpr(std::endian::native==std::endian::big)OpenLoco::ObjectWire::swapFixedFields(OpenLoco::ObjectType::climate,climate);
 assert(climate==original);
 std::puts("PASS unaligned object header, truncated-header rejection, checksum flag byte and typed climate field round trip");
}
int main(int argc,char**argv){
 try{
  selfTest();if(argc==1)return 0;
  if(argc!=3){std::fprintf(stderr,"Usage: object-wire path-list output-canonical\n");return 2;}
  std::ifstream list(argv[1]);if(!list)throw std::runtime_error("Opening object path list failed");std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("Opening canonical output failed");
  size_t count=0,total=0;std::map<unsigned,size_t> types;std::string path;
  while(std::getline(list,path)){
   if(!path.empty()&&path.back()=='\r')path.pop_back();if(path.empty())continue;
   auto bytes=load(path);auto header=OpenLoco::ObjectWire::decodeHeader(bytes);
   OpenLoco::MemoryStream memory;memory.write(bytes.data(),bytes.size());memory.setPosition(sizeof(OpenLoco::ObjectHeader));OpenLoco::SawyerStreamReader reader(memory);auto decoded=reader.readChunk();
   if(OpenLoco::ObjectWire::checksum(header,decoded)!=header.checksum)throw std::runtime_error("Original object checksum failed: "+path);
   Bytes native(decoded.begin(),decoded.end());OpenLoco::ObjectWire::decodeFixedFields(header.getType(),native);
   auto fields=OpenLoco::ObjectWire::canonicalFixedFields(header.getType(),native);
   if constexpr(std::endian::native==std::endian::big)OpenLoco::ObjectWire::swapFixedFields(header.getType(),native);
   assert(std::equal(native.begin(),native.end(),decoded.begin(),decoded.end()));
   auto wire=OpenLoco::ObjectWire::encodeHeader(header);assert(std::equal(wire.begin(),wire.end(),bytes.begin()));
   out.write(reinterpret_cast<const char*>(wire.data()),wire.size());put(out,decoded.size());put(out,fields.size());out.write(reinterpret_cast<const char*>(fields.data()),fields.size());
   ++count;total+=decoded.size();++types[static_cast<unsigned>(header.getType())];
  }
  if(!out||count==0)throw std::runtime_error("No objects or canonical write failed");
  std::printf("PASS %lu object headers, original checksums, typed fixed-field decode and wire-order round trips; %lu decoded bytes\n",static_cast<unsigned long>(count),static_cast<unsigned long>(total));
  for(auto [type,n]:types)std::printf("TYPE %u %lu\n",type,static_cast<unsigned long>(n));return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
