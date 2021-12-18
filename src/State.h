#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "StepData.h"
#include "Constants.h"

class State {
public:
    State(juce::AudioProcessorValueTreeState &p) : parameters(p) {
        stepsParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("steps"));
        voicesParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("voices"));
        rateParameter = dynamic_cast<juce::AudioParameterChoice *> (parameters.getParameter("rate"));
        for (size_t i = 0; i < MAX_STEPS; i++) {
            for (size_t j = 0; j < MAX_VOICES; j++) {
                juce::AudioParameterBool *param = dynamic_cast<juce::AudioParameterBool *> (parameters.getParameter(
                        "voice_" + std::to_string(i) + "_" + std::to_string(j)));
                stepData[i].voiceParameters[j] = param ? param : nullptr;
            }
        }
    }

public:
    juce::AudioProcessorValueTreeState &parameters;

    juce::AudioParameterChoice *stepsParameter = nullptr;
    juce::AudioParameterChoice *voicesParameter = nullptr;
    juce::AudioParameterChoice *rateParameter = nullptr;
    std::array<StepData, MAX_STEPS> stepData;
};
