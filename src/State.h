#pragma once

#include <random>
#include <juce_audio_utils/juce_audio_utils.h>
#include <readerwriterqueue.h>

#include "StepState.h"
#include "Constants.h"
#include "Step.h"

class State {
public:
    explicit State(juce::AudioProcessorValueTreeState &p) : parameters(p) {
        stretchParameter = dynamic_cast<juce::AudioParameterBool *> (parameters.getParameter("stretch"));
        stepsParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("steps"));
        voicesParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("voices"));
        rateParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("rate"));
        rateTypeParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("rateType"));
        notesParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("notes"));
        for (stepnum_t i = 0; i < constants::MAX_STEPS; i++) {
            for (voicenum_t j = 0; j < constants::MAX_VOICES; j++) {
                juce::AudioParameterBool *voiceParameter = dynamic_cast<juce::AudioParameterBool *> (parameters.getParameter(
                        "step" + std::to_string(i) + "_voice" + std::to_string(j)));
                stepState[i].voiceParameters[j] = voiceParameter ? voiceParameter : nullptr;
            }
            stepState[i].octaveParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter(
                    "step" + std::to_string(i) + "_octave"));
            stepState[i].lengthParameter = dynamic_cast<juce::AudioParameterFloat *> (parameters.getParameter(
                    "step" + std::to_string(i) + "_length"));
            stepState[i].volParameter = dynamic_cast<juce::AudioParameterFloat *> (parameters.getParameter(
                    "step" + std::to_string(i) + "_volume"));
            stepState[i].tieParameter = dynamic_cast<juce::AudioParameterBool *> (parameters.getParameter(
                    "step" + std::to_string(i) + "_tie"));
            stepState[i].powerParameter = dynamic_cast<juce::AudioParameterBool *> (parameters.getParameter(
                    "step" + std::to_string(i) + "_power"));
        }
    }

    void randomizeParams(bool stepsAndVoices, bool rate, bool notes) {
        std::random_device rd;
        std::mt19937 mt(rd());

        std::uniform_int_distribution<int> randVoiceEnabled(0, 5);

        size_t numOctaveChoices = constants::MAX_OCTAVES * 2 + 1;
        std::discrete_distribution<int> randOctave(numOctaveChoices, 0.0, static_cast<double>(numOctaveChoices),
                                                   [](double val) { // make central octaves more likely
                                                       double weight = std::floor(val);
                                                       if (weight > constants::MAX_OCTAVES) {
                                                           weight = constants::MAX_OCTAVES * 2 - weight;
                                                       }
                                                       return std::pow(weight, 2);
                                                   });

        std::uniform_real_distribution<float> randLength(0.0, 1.0);
        std::uniform_int_distribution<int> randTie(0, 10);
        std::uniform_real_distribution<float> randVolume(0.0, 1.0);
        std::uniform_int_distribution<int> randPower(0, 10);

        *(stretchParameter) = false;

        stepnum_t numSteps;
        voicenum_t numVoices;
        if (stepsAndVoices) {
            std::uniform_int_distribution<stepnum_t> randNumSteps(1, constants::MAX_STEPS);
            std::uniform_int_distribution<voicenum_t> randNumVoices(1, constants::MAX_VOICES);
            numSteps = randNumSteps(mt);
            numVoices = randNumVoices(mt);
            *(stepsParameter) = static_cast<int>(numSteps - 1); // index
            *(voicesParameter) = static_cast<int>(numVoices - 1); // index
        } else {
            numSteps = static_cast<stepnum_t>(stepsParameter->getIndex()) + 1;
            numVoices = static_cast<voicenum_t>(voicesParameter->getIndex()) + 1;
        }

        if (rate) {
            std::uniform_int_distribution<int> randRate(0, rateParameter->getAllValueStrings().size() - 1);
            std::uniform_int_distribution<int> randRateType(0, rateTypeParameter->getAllValueStrings().size() - 1);
            *(rateParameter) = randRate(mt); // index
            *(rateTypeParameter) = randRateType(mt); // index
        }

        std::uniform_int_distribution<voicenum_t> randMainVoice(0, numVoices - 1);

        if (notes) {
            std::uniform_int_distribution<int> randNotes(0, notesParameter->getAllValueStrings().size() - 1);
            *(notesParameter) = randNotes(mt); // index
        }

        for (stepnum_t i = 0; i < numSteps; i++) {
            voicenum_t mainVoice = randMainVoice(mt);
            for (voicenum_t j = 0; j < numVoices; j++) {
                bool enabled = j == mainVoice || randVoiceEnabled(mt) == 0;
                *(stepState[i].voiceParameters[j]) = enabled;
            }
            *(stepState[i].octaveParameter) = randOctave(mt); // index
            *(stepState[i].lengthParameter) = randLength(mt);
            *(stepState[i].tieParameter) = randTie(mt) == 0;
            *(stepState[i].volParameter) = randVolume(mt);
            *(stepState[i].powerParameter) = randPower(mt) != 0;
        }
    }

public:
    juce::AudioProcessorValueTreeState &parameters;

    juce::AudioParameterBool *stretchParameter = nullptr;
    juce::AudioParameterChoice *stepsParameter = nullptr;
    juce::AudioParameterChoice *voicesParameter = nullptr;
    juce::AudioParameterChoice *rateParameter = nullptr;
    juce::AudioParameterChoice *rateTypeParameter = nullptr;
    juce::AudioParameterChoice *notesParameter = nullptr;
    std::array<StepState, constants::MAX_STEPS> stepState;

    std::atomic<stepnum_t> stepIndex{0};
    std::atomic<bool> playing{false};
    std::atomic<bool> recordButton{false};
    std::atomic<bool> recordedRest{false};

    moodycamel::ReaderWriterQueue<UpdatedSteps> updateStepsFromAudioThread{16};
};
