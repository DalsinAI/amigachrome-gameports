#include <OpenLoco/Audio/AudioEngine.h>
#include <cassert>
#include <cstdio>
using namespace OpenLoco::Audio;
int main() {
    initialize();
    assert(!isEnabled());assert(!openDevice(""));assert(getAvailableDevices().empty());
    AudioFormat format{};
    const auto buffer=loadBuffer({},format);assert(buffer==BufferId::null);
    const auto handle=create(buffer,ChannelId::effects);assert(handle==AudioHandle::null);
    play(handle);pause(handle);unpause(handle);stop(handle);
    assert(!isPlaying(handle));assert(!isPaused(handle));
    setChannelVolume(ChannelId::effects,-300);assert(getChannelVolume(ChannelId::effects)==-300);
    setChannelVolume(static_cast<ChannelId>(255),10);assert(getChannelVolume(static_cast<ChannelId>(255))==0);
    unloadBuffer(buffer);destroy(handle);stopAll();pauseAll();unpauseAll();closeDevice();shutdown();
    std::puts("PASS: audio-disabled backend reports no device, buffers or voices");
}
