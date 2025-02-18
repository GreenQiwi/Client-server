#pragma once

#include <vector>
#include <portaudio.h>

#include "Authentication.hpp"
#include "Connection.hpp"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define NUMBER_OF_CHANNELS  1
#define MAX_FILE_SIZE 1024 * 1024  
#define CONNECTION_ATTEMPTS 100

class AudioStorage  {
public:
    AudioStorage();
    ~AudioStorage();

    void InitRecord();
    void StartRecord();
    void StopRecord();
    void ParseAudio(const void* inputAudio, size_t framesNumber);

private:
    std::vector<float> m_audiodata;
    int m_index;
    PaStream* m_stream;
    Authentication m_auth;

private:
    void sendFile();
};
