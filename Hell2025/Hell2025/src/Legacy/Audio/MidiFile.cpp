
#include "midifile/MidiFile.h"
#include "midifile/MidiEventList.h"
#include "midifile/MidiEvent.h"

#include "MidiFile.h"
#include "Synth.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

#include "World/World.h"

// get me out of here
#include "MidiFileManager.h"
#include "Util/Util.h"
// get me out of here

void MidiFile::Update(float deltaTime) {
    if (!m_play) return;

    m_playbackTime += deltaTime * 1.05f;
    //m_playbackTime += deltaTime * 1.00f;
    bool printDebug = true;

    // Process all events scheduled up to the current playback time
    while (m_nextEventIndex < m_scheduledEvents.size() && m_scheduledEvents[m_nextEventIndex].timestamp <= m_playbackTime) {
        const ScheduledEvent& currentEvent = m_scheduledEvents[m_nextEventIndex];

        Piano& piano = World::GetPianos()[1];

        switch (currentEvent.type) {
            case MidiEventType::NOTE_ON: {
                if (printDebug) {

                    MidiFileManager::AddDebugTextTimes("Time: " + Util::DoubleToString(m_playbackTime) + "s");
                    MidiFileManager::AddDebugTextEvent("Note On: " + std::to_string(currentEvent.note));
                    MidiFileManager::AddDebugTextDurations("Dur: " + Util::DoubleToString(currentEvent.duration) + "s");
                    MidiFileManager::AddDebugTextVelocity("Vel: " + std::to_string(currentEvent.velocity));

                    std::cout << "Time: " << std::fixed << std::setprecision(3) << m_playbackTime << "s | Trigger Note On: " << currentEvent.note << " | Vel: " << currentEvent.velocity << " | Dur: " << currentEvent.duration << "s" << " | @ MIDI Time: " << currentEvent.timestamp << "s\n";
                }
                piano.PlayKey(currentEvent.note, currentEvent.velocity, currentEvent.duration);
                break;
            }

            case MidiEventType::SUSTAIN: {
                if (printDebug) {

                    MidiFileManager::AddDebugTextTimes("Time: " + Util::DoubleToString(m_playbackTime) + "s");
                    MidiFileManager::AddDebugTextEvent(currentEvent.sustainValue ? "Sustain pedal on" : "Sustain pedal off");
                    MidiFileManager::AddDebugTextDurations("  ");
                    MidiFileManager::AddDebugTextVelocity(" ");

                    std::cout << "Time: " << std::fixed << std::setprecision(3) << m_playbackTime << "s | Trigger Sustain: " << (currentEvent.sustainValue ? "ON" : "OFF") << " | @ MIDI Time: " << currentEvent.timestamp << "s\n";
                }
                piano.SetSustain(currentEvent.sustainValue);
                break;
            }
        }

        // Move to the next scheduled event
        m_nextEventIndex++;
    }
    if (m_playbackTime >= m_totalDurationSeconds) {
        m_complete = true;
        m_play = false;
    }
}

bool MidiFile::LoadFromFile(const std::string& filename) {
    smf::MidiFile smfMidiFile;
    if (!smfMidiFile.read("res/audio/midi/" + filename)) {
        return false;
    }

    // Extract meta data

    m_ticksPerQuarterNote = smfMidiFile.getTicksPerQuarterNote();
    m_totalDurationSeconds = smfMidiFile.getFileDurationInSeconds();

    if (m_ticksPerQuarterNote <= 0) {
        std::cout << "MidiFile::LoadFromFile() failed: ticks per quarter note was less than 0\n";
        return false;
    }

    smfMidiFile.linkNotePairs();
    smfMidiFile.doTimeAnalysis();

    if (smfMidiFile.getTrackCount() > 1) {
        smfMidiFile.joinTracks();
    }

    int note_count = 0;
    int sustain_count = 0;
    int velocityMax = 0;

    if (smfMidiFile.getTrackCount() > 0 && smfMidiFile[0].size() > 0) {
        for (int event_idx = 0; event_idx < smfMidiFile[0].size(); event_idx++) {
            smf::MidiEvent* midiEvent = &smfMidiFile[0][event_idx];

            // Check for Note On
            if (midiEvent->isNoteOn() && (*midiEvent)[2] > 0) {
                ScheduledEvent scheduledEvent;
                scheduledEvent.type = MidiEventType::NOTE_ON;
                scheduledEvent.timestamp = midiEvent->seconds;
                scheduledEvent.note = (*midiEvent)[1];
                scheduledEvent.velocity = (*midiEvent)[2];
                scheduledEvent.duration = 0.0;

                if (midiEvent->isLinked()) {
                    scheduledEvent.duration = midiEvent->getDurationInSeconds();
                }
                else {
                    // Assign default duration for unlinked notes
                    scheduledEvent.duration = 0.1;
                }
                if (scheduledEvent.duration < 0.0) {
                    scheduledEvent.duration = 0.1;
                }

                m_scheduledEvents.push_back(scheduledEvent);
                note_count++;
                velocityMax = std::max(velocityMax, scheduledEvent.velocity);
            }

            // Check for Sustain Pedal
            else if (midiEvent->isController() && midiEvent->getControllerNumber() == 64) {
                ScheduledEvent scheduledEvent;
                scheduledEvent.type = MidiEventType::SUSTAIN;
                scheduledEvent.timestamp = midiEvent->seconds;
                int value = midiEvent->getControllerValue();
                scheduledEvent.sustainValue = (value >= 64); // true if >= 64, false otherwise

                m_scheduledEvents.push_back(scheduledEvent);
                sustain_count++;
            }
        }
    }

    // Normalize
    int normalizer = 127 - velocityMax;
    for (ScheduledEvent& scheduledEvent : m_scheduledEvents) {
        scheduledEvent.velocity += normalizer;
    }

    // Sort scheduled events by timestamp
    std::sort(m_scheduledEvents.begin(), m_scheduledEvents.end());

    // Final error check
    if (m_scheduledEvents.empty()) {
        std::cout << "MidiFileReader::LoadExternalMidiFile() failed: no scheduled events were found\n";
        return false;
    }

    // Successful load!
    m_nextEventIndex = 0;
    m_initialTime = m_scheduledEvents[0].timestamp;
    m_filename = filename;
    return true;
}

void MidiFile::Play() {
    m_play = !m_play;

    if (m_play) {
        m_playbackTime = m_initialTime;
        m_nextEventIndex = 0;
        m_complete = false;
    }
}

void MidiFile::Stop() {
    m_play = false;
    m_complete = false;
}