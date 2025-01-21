#include "AudioStorage.hpp"
#include "Connection.hpp"

AudioStorage::AudioStorage()
    : m_index(0), m_stream(nullptr), m_threadpool(std::thread::hardware_concurrency()), m_auth() {}


AudioStorage::~AudioStorage()
{
    if(m_stream != nullptr)
    {
        Pa_CloseStream(m_stream);
        Pa_Terminate();
    }

    //m_threadpool.join();
}

void AudioStorage::InitRecord()
{
    m_auth.LogIn();

    PaError err = Pa_Initialize();
    if (err != paNoError) 
    {
        throw std::runtime_error("PortAudio initialization failed: " + std::string(Pa_GetErrorText(err)));
    }

    PaStreamParameters inputParams;
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) 
    {
        throw std::runtime_error("No default input device available.");
    }

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(inputParams.device);
    if (!deviceInfo) 
    {
        throw std::runtime_error("Failed to get device info.");
    }

    inputParams.channelCount = NUMBER_OF_CHANNELS;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(
        &m_stream,
        &inputParams,
        nullptr,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paClipOff,
        [](const void* input, void* output, unsigned long frames,
            const PaStreamCallbackTimeInfo* timeInfo,
            PaStreamCallbackFlags statusFlags, void* userData) -> int {
                static_cast<AudioStorage*>(userData)->ParseAudio(input, frames);
                return paContinue;
        },
        this);

    if (err != paNoError) 
    {
        throw std::runtime_error("Failed to open stream: " + std::string(Pa_GetErrorText(err)));
    }
}

void AudioStorage::StartRecord()
{
    if (Pa_StartStream(m_stream) != paNoError) 
    {
        throw std::runtime_error("Failed to start stream");
    }
    std::cout << "Recording... Press 'q' to stop." << std::endl;
}

void AudioStorage::StopRecord() {
    if (!m_audiodata.empty()) 
    {
        sendFile();
    }

    if (Pa_StopStream(m_stream) != paNoError) 
    {
        throw std::runtime_error("Failed to stop stream.");
    }
    std::cout << "Recording stopped." << std::endl;

    //m_threadpool.join();
}

void AudioStorage::ParseAudio(const void* inputAudio, size_t framesNumber)
{
    if (inputAudio == nullptr) return;

    const float* input = static_cast<const float*>(inputAudio);
    for (size_t i = 0; i < framesNumber; ++i)
    {
        m_audiodata.push_back(*input++);

        if (m_audiodata.size() * sizeof(float) >= MAX_FILE_SIZE) 
        {
            sendFile();
        }
    }
}

void AudioStorage::sendFile()
{
    std::string filename = "audio_part_" + std::to_string(m_index++) + ".wav";
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open())
    {
        std::cerr << "Failed to create file: " << filename << std::endl;
        return;
    }

    int dataSize = m_audiodata.size() * sizeof(float);
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
    outFile.write(reinterpret_cast<const char*>(m_audiodata.data()), m_audiodata.size() * sizeof(float));
    outFile.close();

    if (!std::ifstream(filename))
    {
        std::cerr << "File does not exist or failed to save properly: " << filename << std::endl;
        return;
    }

    int attempt = 0;
    bool success = false;

    while (attempt < 100 && !success) // 100 const
    {
        try
        {
            Connection connection("127.0.0.1", "8080");
            http::response<http::string_body> res = connection.UploadFile(filename, "/upload", "audio/wav", m_auth.GetAuthHeader(), m_auth.GetId());   

            if (res.result_int() != 0 && res.result() == http::status::unauthorized) {
                std::cout << "Digest header generation" << std::endl;
                m_auth.Authenticate(res, "POST", "audio/wav");
            }

            if (res.result_int() != 0 && res.result() != http::status::unauthorized)
                success = true;
            
        }
        catch (const std::exception& ex)
        {
            attempt++;
            std::cerr << "Attempt " << attempt << " failed: " << ex.what() << std::endl;
            if (attempt < 100)
            {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    if (!success)
    {
        std::cerr << "Failed to upload file after 100 attempts: " << filename << std::endl;
    }

    std::remove(filename.c_str());
    m_audiodata.clear();
}
