// Engineering-only backend. No device, buffers or voices are ever allocated.
// Restore the default backend to qualify OpenAL.module / AHI audio.
#include <OpenLoco/Audio/AudioEngine.h>
#include <array>
#include <cstdio>
namespace OpenLoco::Audio
{
    static std::array<int32_t, static_cast<size_t>(ChannelId::count)> volumes{};
    void initialize() { std::fputs("OpenLoco: AUDIO DISABLED (engineering build)\n", stderr); }
    void shutdown() {}
    bool openDevice(const std::string&) { return false; }
    void closeDevice() {}
    std::vector<std::string> getAvailableDevices() { return {}; }
    BufferId loadBuffer(std::span<const uint8_t>, const AudioFormat&) { return BufferId::null; }
    void unloadBuffer(BufferId) {}
    AudioHandle create(BufferId, ChannelId, const AudioAttributes&) { return AudioHandle::null; }
    void destroy(AudioHandle) {}
    void play(AudioHandle) {}
    void stop(AudioHandle) {}
    void pause(AudioHandle) {}
    void unpause(AudioHandle) {}
    bool isPlaying(AudioHandle) { return false; }
    bool isPaused(AudioHandle) { return false; }
    void setVolume(AudioHandle, int32_t) {}
    void setPan(AudioHandle, int32_t) {}
    void setPitch(AudioHandle, int32_t) {}
    void setAttributes(AudioHandle, const AudioAttributes&) {}
    void setChannelVolume(ChannelId c, int32_t v) { auto i=static_cast<size_t>(c); if(i<volumes.size()) volumes[i]=v; }
    int32_t getChannelVolume(ChannelId c) { auto i=static_cast<size_t>(c); return i<volumes.size()?volumes[i]:0; }
    void setReverb(AudioHandle, const ReverbParams&) {}
    void reclaimFinishedInstances() {}
    void stopAll() {}
    void pauseAll() {}
    void unpauseAll() {}
    bool isEnabled() { return false; }
}
