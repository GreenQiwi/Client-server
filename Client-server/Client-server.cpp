#include <iostream>
#include <boost/regex.hpp>
#include <portaudio.h>
#include <vector>
#include <fstream>
#include <conio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512
#define NUMBER_OF_CHANNELS  1
#define MAX_FILE_SIZE 1024 * 1024  

class AudioStorage {
public:
    std::vector<float> audiodata;
    int index = 0;

    void SaveToFile() {
        std::ofstream outFile("audio_part_" + std::to_string(index++) + ".wav", std::ios::binary);
        if (!outFile) {
            std::cerr << "Error opening file for writing." << std::endl;
            return;
        }

        WriteWavHeader(outFile, audiodata.size() * sizeof(float));

        outFile.write(reinterpret_cast<const char*>(audiodata.data()), audiodata.size() * sizeof(float));
        outFile.close();

        audiodata.clear();
    }

private:
    void WriteWavHeader(std::ofstream& outFile, int dataSize) {
        // Constants for WAV header
        const int chunkSize = 36 + dataSize;
        const int subChunk1Size = 16;
        const short audioFormat = 3;  // IEEE float format
        const int byteRate = SAMPLE_RATE * NUMBER_OF_CHANNELS * sizeof(float);
        const short blockAlign = NUMBER_OF_CHANNELS * sizeof(float);
        const short bitsPerSample = sizeof(float) * 8;
        const short numChannels = NUMBER_OF_CHANNELS;
        const int sampleRate = SAMPLE_RATE;

        outFile.write("RIFF", 4);
        outFile.write(reinterpret_cast<const char*>(&chunkSize), sizeof(int));
        outFile.write("WAVE", 4);

        outFile.write("fmt ", 4);
        outFile.write(reinterpret_cast<const char*>(&subChunk1Size), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&audioFormat), sizeof(short));
        outFile.write(reinterpret_cast<const char*>(&numChannels), sizeof(short));  
        outFile.write(reinterpret_cast<const char*>(&sampleRate), sizeof(int));     
        outFile.write(reinterpret_cast<const char*>(&byteRate), sizeof(int));
        outFile.write(reinterpret_cast<const char*>(&blockAlign), sizeof(short));
        outFile.write(reinterpret_cast<const char*>(&bitsPerSample), sizeof(short));

        outFile.write("data", 4);
        outFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(int));
    }


};

static int RecordCallback(
    const void* inputAudio,
    void* outputAudio,
    unsigned long framesNumber,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    AudioStorage* storage = (AudioStorage*)userData;
    const float* input = (const float*)inputAudio;

    if (inputAudio != nullptr) {
        for (unsigned long i = 0; i < framesNumber; i++) {
            storage->audiodata.push_back(*input++);

            if (storage->audiodata.size() * sizeof(float) >= MAX_FILE_SIZE) {
                storage->SaveToFile();
            }
        }
    }
    return paContinue;
}

int main() {
    Pa_Initialize();

    AudioStorage data;
    PaStream* stream;

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    inputParams.channelCount = NUMBER_OF_CHANNELS;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    if (Pa_OpenStream(
        &stream,
        &inputParams,
        nullptr,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        RecordCallback,
        &data
    ) != paNoError) {
        std::cerr << "Failed to open stream." << std::endl;
        return 1;
    }

    Pa_StartStream(stream);
    std::cout << "Recording... Press 'q' to stop." << std::endl;

    while (true) {
        if (_kbhit()) {
            char c = _getch();
            if (c == 'q') break;
        }
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    if (!data.audiodata.empty()) {
        data.SaveToFile();
    }

    std::cout << "Recording stopped." << std::endl;
    return 0;
}
