#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <sidplayfp/sidplayfp.h>
#include <sidplayfp/SidInfo.h>
#include <sidplayfp/SidTune.h>
#include <sidplayfp/SidTuneInfo.h>
#include <sidplayfp/SidConfig.h>

class Player {
public:
    Player();
    ~Player();
    
    bool loadFile(const std::string& filename);
    void play();
    void pause();
    void stop();
    void nextTrack();
    void prevTrack();
    
    bool isPlaying() const { return playing; }
    bool isPaused() const { return paused; }
    
    std::string getCurrentFile() const { return current_file; }
    int getCurrentTrack() const { return current_track; }
    int getTrackCount() const { return track_count; }
    std::string getTitle() const { return title; }
    std::string getAuthor() const { return author; }
    std::string getCopyright() const { return copyright; }
    int getPlayTime() const { return play_time; }
    
private:
    void audioThread();
    void updatePlayTime();
    
    std::unique_ptr<sidplayfp> engine;
    std::unique_ptr<SidTune> tune;
    class ReSIDfpBuilder* sid_builder;
    
    std::string current_file;
    int current_track;
    int track_count;
    std::string title;
    std::string author;
    std::string copyright;
    
    std::atomic<bool> playing;
    std::atomic<bool> paused;
    std::atomic<bool> should_stop;
    std::atomic<int> play_time;
    
    std::thread audio_thread;
    std::thread timer_thread;
};