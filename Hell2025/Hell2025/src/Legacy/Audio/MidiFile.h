#pragma once
#include <string>
#include <vector>

enum class MidiEventType {
    NOTE_ON,
    SUSTAIN
};

struct ScheduledEvent {
    double timestamp = 0.0;
    MidiEventType type = MidiEventType::NOTE_ON;
    int note = 0;
    int velocity = 0;
    double duration = 0.0;
    bool sustainValue = false;

    bool operator<(const ScheduledEvent& other) const {
        return timestamp < other.timestamp;
    }
};

struct MidiFile {
    bool LoadFromFile(const std::string& filename);
    void Play();
    void Stop();
    void Update(float deltaTime);

    const std::string GetFileName () const  { return m_filename; }
    const bool IsComplete() const           { return m_complete; }
    const float GetPlaybackTime() const     { return (float)m_playbackTime; }
    const float GetDuration() const         { return (float)m_totalDurationSeconds; }

private:
    std::string m_filename = "";
    size_t m_startingIndex = 0;
    size_t m_nextEventIndex = 0;
    std::vector<ScheduledEvent> m_scheduledEvents;
    double m_playbackTime = 0.0;
    double m_initialTime = 0.0f;
    int m_initialTempo = 500000;
    int m_initialTimeSigNumerator = 4;
    int m_initialTimeSigDenominator = 4;
    int m_ticksPerQuarterNote = 0;
    double m_barDurationSeconds = 0.0;
    double m_totalDurationSeconds = 0.0;
    bool m_play = false;
    bool m_complete = false;
};