#pragma once

#include "Authentication.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <portaudio.h>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>
#include "Connection.hpp"

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define NUMBER_OF_CHANNELS  1
#define MAX_FILE_SIZE 1024 * 1024  

class AudioStorage {
public:
    AudioStorage();
    ~AudioStorage();

    void initRecord();
    void startRecord();
    void stopRecord();
    void parseAudio(const void* inputAudio, unsigned long framesNumber);

private:
    std::vector<float> audiodata;
    int index = 0;
    PaStream* stream;
    asio::thread_pool threadpool;

    Authentication auth;

    void sendFile();
    void WriteWavHeader(std::ofstream& outFile, int dataSize);
};
