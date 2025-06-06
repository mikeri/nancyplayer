#include "player.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/def.h>
#include <sidplayfp/builders/residfp.h>

Player::Player() : current_track(1), track_count(0), playing(false), paused(false), should_stop(false), play_time(0), sid_builder(nullptr) {
    engine = std::make_unique<sidplayfp>();
}

Player::~Player() {
    stop();
    delete sid_builder;
}

bool Player::loadFile(const std::string& filename) {
    stop();
    
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    file.close();
    
    tune = std::make_unique<SidTune>(data.data(), size);
    if (!tune->getStatus()) {
        return false;
    }
    
    const SidTuneInfo* info = tune->getInfo();
    if (!info) {
        return false;
    }
    
    current_file = filename;
    track_count = info->songs();
    current_track = info->startSong();
    
    title = info->infoString(0) ? info->infoString(0) : "";
    author = info->infoString(1) ? info->infoString(1) : "";
    copyright = info->infoString(2) ? info->infoString(2) : "";
    
    tune->selectSong(current_track);
    
    // Clean up previous SID builder if it exists
    delete sid_builder;
    
    // Reset the engine to clean state
    engine.reset();
    engine = std::make_unique<sidplayfp>();
    
    // Create ReSIDfp builder for SID emulation
    sid_builder = new ReSIDfpBuilder("ReSIDfp");
    if (!sid_builder) {
        std::cerr << "Failed to create ReSIDfp builder" << std::endl;
        return false;
    }
    
    // Create SID chips (usually 1, but some tunes use more)
    sid_builder->create(engine->info().maxsids());
    if (!sid_builder->getStatus()) {
        std::cerr << "Failed to create SID chips" << std::endl;
        delete sid_builder;
        sid_builder = nullptr;
        return false;
    }
    
    // Configure the SID engine with ReSIDfp emulation
    SidConfig config;
    config.frequency = 44100;
    config.playback = SidConfig::MONO;
    config.samplingMethod = SidConfig::INTERPOLATE;
    config.fastSampling = false;
    config.sidEmulation = sid_builder; // Use ReSIDfp emulation
    
    if (!engine->config(config)) {
        std::cerr << "Failed to configure SID engine" << std::endl;
        return false;
    }
    
    if (!engine->load(tune.get())) {
        std::cerr << "Failed to load SID tune into engine" << std::endl;
        return false;
    }
    
    
    play_time = 0;
    
    return true;
}

void Player::play() {
    if (tune && !playing) {
        should_stop = false;
        playing = true;
        paused = false;
        
        if (audio_thread.joinable()) {
            audio_thread.join();
        }
        if (timer_thread.joinable()) {
            timer_thread.join();
        }
        
        audio_thread = std::thread(&Player::audioThread, this);
        timer_thread = std::thread(&Player::updatePlayTime, this);
    } else if (playing && paused) {
        paused = false;
    }
}

void Player::pause() {
    if (playing && !paused) {
        paused = true;
    }
}

void Player::stop() {
    if (playing) {
        should_stop = true;
        
        if (audio_thread.joinable()) {
            audio_thread.join();
        }
        if (timer_thread.joinable()) {
            timer_thread.join();
        }
        
        playing = false;
        paused = false;
        play_time = 0;
    }
}

void Player::nextTrack() {
    if (tune && current_track < track_count) {
        current_track++;
        tune->selectSong(current_track);
        engine->load(tune.get());
        play_time = 0;
    }
}

void Player::prevTrack() {
    if (tune && current_track > 1) {
        current_track--;
        tune->selectSong(current_track);
        engine->load(tune.get());
        play_time = 0;
    }
}

void Player::audioThread() {
    pa_simple* pulse = nullptr;
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16LE;
    ss.channels = 1;
    ss.rate = 44100;
    
    int error;
    pulse = pa_simple_new(nullptr, "Nancy SID Player", PA_STREAM_PLAYBACK, nullptr, 
                         "SID Music", &ss, nullptr, nullptr, &error);
    
    if (!pulse) {
        return;
    }
    
    const int buffer_size = 1024;
    short buffer[buffer_size];
    
    while (playing && !should_stop) {
        if (paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }
        
        int samples = engine->play(buffer, buffer_size);
        
        if (samples <= 0) {
            break;
        }
        
        if (pa_simple_write(pulse, buffer, samples * sizeof(short), &error) < 0) {
            break;
        }
        
        // Timing based on sample rate: 1024 samples at 44100 Hz = ~23ms
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    if (pulse) {
        pa_simple_drain(pulse, &error);
        pa_simple_free(pulse);
    }
}

void Player::updatePlayTime() {
    while (playing && !should_stop) {
        if (!paused) {
            play_time++;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}