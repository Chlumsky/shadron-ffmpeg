
#pragma once

#include <vector>

/// Sound file decoder
class SoundDecoder {

public:
    static SoundDecoder * decode(const void *data, int length);

    SoundDecoder(const SoundDecoder &) = delete;
    virtual ~SoundDecoder();
    SoundDecoder & operator=(const SoundDecoder &) = delete;
    int getSampleRate() const;
    int getSampleCount() const;
    void copyWaveform(void *output, int samples) const;

private:
    int sampleRate;
    int sampleCount;
    std::vector<unsigned char> sampleBuffer;

    SoundDecoder(int sampleRate, int sampleCount, std::vector<unsigned char> &&sampleBuffer);

};
