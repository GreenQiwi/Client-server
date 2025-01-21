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
    asio::thread_pool m_threadpool;
    Authentication m_auth;
    asio::thread_pool m_threadPool;

private:
    void sendFile();

};
