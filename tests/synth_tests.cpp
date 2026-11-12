// macOS only: with no SoundFont installed, the game's MIDI music is silent unless the platform's
// General MIDI synthesizer works. Render a note directly from the DLS synth unit. Direct
// rendering needs no audio output device, so this also runs on a machine or CI runner with no
// speakers attached.
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <cmath>
#include <cstdio>

namespace {
int Fail(const char* what, OSStatus status, AudioUnit synth) {
    std::fprintf(stderr, "FAIL: %s (OSStatus %d)\n", what, (int)status);
    if (synth) AudioComponentInstanceDispose(synth);
    return 1;
}
}  // namespace

int main() {
    AudioComponentDescription desc{};
    desc.componentType = kAudioUnitType_MusicDevice;
    desc.componentSubType = kAudioUnitSubType_DLSSynth;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    AudioComponent component = AudioComponentFindNext(nullptr, &desc);
    if (!component) {
        // A macOS without the system synthesizer makes the game's music silent; accept that for
        // an unusual environment but never hide a synth that is present but broken.
        std::fprintf(stderr, "SKIP: the macOS DLS synthesizer is not available\n");
        return 77;
    }

    AudioUnit synth = nullptr;
    OSStatus st = AudioComponentInstanceNew(component, &synth);
    if (st != noErr) return Fail("could not create the DLS synthesizer", st, nullptr);

    AudioStreamBasicDescription format{};
    format.mSampleRate = 44100;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonInterleaved;
    format.mChannelsPerFrame = 2;
    format.mFramesPerPacket = 1;
    format.mBitsPerChannel = 32;
    format.mBytesPerFrame = 4;
    format.mBytesPerPacket = 4;
    st = AudioUnitSetProperty(synth, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &format,
                              sizeof format);
    if (st != noErr) return Fail("the DLS synthesizer rejected a stereo float format", st, synth);
    st = AudioUnitInitialize(synth);
    if (st != noErr) return Fail("could not initialise the DLS synthesizer", st, synth);

    MusicDeviceMIDIEvent(synth, 0xC0, 0, 0, 0);       // program 0: Acoustic Grand Piano
    MusicDeviceMIDIEvent(synth, 0x90, 60, 100, 0);    // middle C, fortissimo

    constexpr int kFrames = 512;
    float left[kFrames] = {};
    float right[kFrames] = {};
    AudioBufferList buffers{};
    buffers.mNumberBuffers = 2;
    buffers.mBuffers[0].mNumberChannels = 1;
    buffers.mBuffers[0].mDataByteSize = sizeof left;
    buffers.mBuffers[0].mData = left;
    buffers.mBuffers[1].mNumberChannels = 1;
    buffers.mBuffers[1].mDataByteSize = sizeof right;
    buffers.mBuffers[1].mData = right;
    AudioTimeStamp time{};
    time.mSampleTime = 0;
    time.mFlags = kAudioTimeStampSampleTimeValid;
    st = AudioUnitRender(synth, nullptr, &time, 0, kFrames, &buffers);
    if (st != noErr) return Fail("could not render from the DLS synthesizer", st, synth);

    double peak = 0;
    for (int i = 0; i < kFrames; i++) peak = std::fmax(peak, std::fabs((double)left[i]));

    AudioUnitUninitialize(synth);
    AudioComponentInstanceDispose(synth);

    if (peak <= 0.0) {
        std::fprintf(stderr, "FAIL: the DLS synthesizer rendered silence\n");
        return 1;
    }
    std::printf("DLS synthesizer rendered a note, peak %.4f\n", peak);
    return 0;
}
