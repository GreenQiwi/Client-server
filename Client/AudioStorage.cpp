#include "AudioStorage.hpp"
#include "Connection.hpp"

AudioStorage::AudioStorage()
    : index(0), stream(nullptr) {}

AudioStorage::~AudioStorage()
{
    if(stream)
    {
        Pa_CloseStream(stream);
        Pa_Terminate();
    }
}

void AudioStorage::initRecord()
{
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
        [](const void* input, void* output, unsigned long frames,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags, void* userData) -> int {
                static_cast<AudioStorage*>(userData)->parseAudio(input, frames);
                return paContinue;
        },
        &data
    ) != paNoError) {
        std::cerr << "Failed to open stream." << std::endl;
    }
}

void AudioStorage::startRecord()
{
    Pa_StartStream(stream);
    std::cout << "Recording... Press 'q' to stop." << std::endl;
}

void AudioStorage::stopRecord() {
    if (!audiodata.empty()) {
        sendFile();
    }

    if (Pa_StopStream(stream) != paNoError) {
        throw std::runtime_error("Failed to stop stream.");
    }
    std::cout << "Recording stopped." << std::endl;
}

void AudioStorage::parseAudio(const void* inputAudio, unsigned long framesNumber)
{
    if (inputAudio == nullptr) return;

    const float* input = static_cast<const float*>(inputAudio);
    for (unsigned long i = 0; i < framesNumber; ++i) {
        audiodata.push_back(*input++);

        if (audiodata.size() * sizeof(float) >= MAX_FILE_SIZE) 
        {
            sendFile();
        }
    }
}

void AudioStorage::sendFile()
{
    std::string filename = "audio_part_" + std::to_string(index++) + ".wav";
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) 
    {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }

    WriteWavHeader(outFile, audiodata.size() * sizeof(float));
    outFile.write(reinterpret_cast<const char*>(audiodata.data()), audiodata.size() * sizeof(float));
    outFile.close();

    if (!std::ifstream(filename)) 
    {
        std::cerr << "File does not exist or failed to save properly: " << filename << std::endl;
        return;
    }

    try 
    {
        Connection client("26.85.236.125", "8080");
        client.UploadFile(filename, "/upload", "audio/wav");
    }
    catch (const std::exception& ex) 
    {
        std::cerr << "File upload failed for " << filename << ": " << ex.what() << std::endl;
    }

    std::remove(filename.c_str());
    audiodata.clear();
}



void AudioStorage::WriteWavHeader(std::ofstream& outFile, int dataSize) 
{
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