#include <catch2/catch.hpp>

#include "MidiProcesserTester.h"

TEST_CASE("MidiProcessor play without transport")
{
    MidiProcessorTester tester{};

    *(tester.state.stepsParameter) = 3; // 3 steps
    *(tester.state.voicesParameter) = 3; // 4 voices
    *(tester.state.rateParameter) = 3; // eighths
    *(tester.state.stepState[0].voiceParameters[3]) = true;
    *(tester.state.stepState[1].voiceParameters[2]) = true;
    *(tester.state.stepState[2].voiceParameters[1]) = true;
    *(tester.state.stepState[3].voiceParameters[0]) = true;

    tester.midiIn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 10);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(2, 62, (juce::uint8) 101), 9);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(3, 64, (juce::uint8) 102), 8);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(4, 65, (juce::uint8) 103), 7);

    tester.processBlocks(10);
    REQUIRE(tester.blocksMidiOutString == "100: Note on C3 Velocity 100 Channel 1\n"
                                          "225: Note off C3 Velocity 0 Channel 1\n"
                                          "350: Note on D3 Velocity 101 Channel 2\n"
                                          "475: Note off D3 Velocity 0 Channel 2\n"
                                          "600: Note on E3 Velocity 102 Channel 3\n"
                                          "725: Note off E3 Velocity 0 Channel 3\n"
                                          "850: Note on F3 Velocity 103 Channel 4\n"
                                          "975: Note off F3 Velocity 0 Channel 4\n");
}

TEST_CASE("MidiProcessor play with transport")
{
    MidiProcessorTester tester{};

    *(tester.state.stepsParameter) = 3; // 3 steps
    *(tester.state.voicesParameter) = 3; // 4 voices
    *(tester.state.rateParameter) = 3; // eighths
    *(tester.state.stepState[0].voiceParameters[3]) = true;
    *(tester.state.stepState[1].voiceParameters[2]) = true;
    *(tester.state.stepState[2].voiceParameters[1]) = true;
    *(tester.state.stepState[3].voiceParameters[0]) = true;

    tester.midiIn.addEvent(juce::MidiMessage::noteOn(1, 60, (juce::uint8) 100), 10);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(2, 62, (juce::uint8) 101), 9);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(3, 64, (juce::uint8) 102), 8);
    tester.midiIn.addEvent(juce::MidiMessage::noteOn(4, 65, (juce::uint8) 103), 7);
    tester.posInfo.isPlaying = true;
    tester.posInfo.ppqPosition = 0;

    tester.processBlocks(9, .2f);
    // 120 bpm, 2 bps, 1000 samples/sec, 500 samples/beat, 2 steps/beat, 250 samples/step,
    // 100 samples/frame, 5 frames/beat, .2 beats/frame (ppqPerBlock)
    REQUIRE(tester.blocksMidiOutString == "0: Note on C3 Velocity 100 Channel 1\n" // aligned to next beat
                                          "124: Note off C3 Velocity 0 Channel 1\n"
                                          "249: Note on D3 Velocity 101 Channel 2\n"
                                          "374: Note off D3 Velocity 0 Channel 2\n"
                                          "499: Note on E3 Velocity 102 Channel 3\n"
                                          "624: Note off E3 Velocity 0 Channel 3\n"
                                          "749: Note on F3 Velocity 103 Channel 4\n"
                                          "874: Note off F3 Velocity 0 Channel 4\n");
}
