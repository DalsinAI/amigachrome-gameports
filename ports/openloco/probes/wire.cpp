#include <S5/SawyerStream.h>
#include <algorithm>
#include <cstdio>
#include <vector>

int main()
{
    using namespace OpenLoco;
    std::vector<uint8_t> payload(257);
    for (size_t i = 0; i < payload.size(); ++i)
        payload[i] = static_cast<uint8_t>(i * 17);
    MemoryStream stream;
    SawyerStreamWriter writer(stream);
    writer.writeChunk(SawyerEncoding::uncompressed, payload.data(), payload.size());
    writer.writeChecksum();
    auto bytes = stream.getSpan();
    if (bytes.size() != payload.size() + 9 || bytes[0] != std::byte{0}
        || bytes[1] != std::byte{1} || bytes[2] != std::byte{1}
        || bytes[3] != std::byte{0} || bytes[4] != std::byte{0}) return 1;
    uint32_t checksum = 0;
    for (size_t i = 0; i < bytes.size() - 4; ++i)
        checksum += static_cast<uint8_t>(bytes[i]);
    for (unsigned i = 0; i < 4; ++i)
        if (bytes[bytes.size() - 4 + i] != std::byte((checksum >> (8 * i)) & 255)) return 2;
    stream.setPosition(0);
    SawyerStreamReader reader(stream);
    auto result = reader.readChunk();
    if (result.size() != payload.size()) return 3;
    for (size_t i = 0; i < payload.size(); ++i)
        if (result[i] != std::byte(payload[i])) return 4;
    const auto position = stream.getPosition();
    if (!reader.validateChecksum() || stream.getPosition() != position) return 5;
    stream.setPosition(bytes.size() - 1);
    const uint8_t corrupt = 255;
    stream.write(&corrupt, 1);
    if (reader.validateChecksum()) return 6;
    MemoryStream truncated;
    const uint8_t shortHeader[] = {0, 1, 1};
    truncated.write(shortHeader, sizeof(shortHeader));
    truncated.setPosition(0);
    SawyerStreamReader badReader(truncated);
    try {
        badReader.readChunk();
        return 7;
    } catch (const std::exception&) {
    }
    std::puts("Sawyer length/checksum bytes, payload round trip, position, corruption and truncation: PASS");
    return 0;
}
