#include "Header.h"

Header::Header(State &s, PluginProcessor &p) : state(s), pluginProcessor(p) {
    juce::Image floppyDisk = FontAwesome::getInstance()->getIcon(true,
                                                           juce::String::fromUTF8(
                                                                   reinterpret_cast<const char *>(u8"\uf0c7")),
                                                           ICON_SIZE, constants::COLOR_TOGGLE_ACTIVE,
                                                           1);
    fileButton.setImages(true, false, true, floppyDisk, 1.0f, {}, {}, 1.0f, {}, {}, 1.0f, constants::COLOR_TOGGLE_INACTIVE);
    fileButton.setTooltip(constants::TOOLTIP_FILE);
    fileButton.onClick = [this] { fileButton.showFileMenu(); };
    addAndMakeVisible(fileButton);

    juce::Image microphone = FontAwesome::getInstance()->getIcon(true,
                                                                 juce::String::fromUTF8(
                                                                         reinterpret_cast<const char *>(u8"\uf130")),
                                                                 ICON_SIZE, constants::COLOR_TOGGLE_ACTIVE,
                                                                 1);
    recordButton.setImages(true, false, true, microphone, 1.0f, {}, {}, 1.0f, {}, {}, 1.0f,
                           constants::COLOR_TOGGLE_INACTIVE);
    recordButton.setClickingTogglesState(true);
    recordButton.setTooltip(constants::TOOLTIP_RECORD);
    addAndMakeVisible(recordButton);
    recordButton.onClick = [this] {
        state.recordButton = recordButton.getToggleState();
        refreshMessage();
        refreshEnabled();
        onClickRecord();
    };

    juce::Image dice = FontAwesome::getInstance()->getIcon(true,
                                                           juce::String::fromUTF8(
                                                                   reinterpret_cast<const char *>(u8"\uf522")),
                                                           ICON_SIZE, constants::COLOR_TOGGLE_ACTIVE,
                                                           1);
    randomButton.setImages(true, false, true, dice, 1.0f, {}, {}, 1.0f, {}, {}, 1.0f, constants::COLOR_TOGGLE_INACTIVE);
    randomButton.setTooltip(constants::TOOLTIP_RANDOM);
    randomButton.onClick = [this] { onClickRandom(); };
    addAndMakeVisible(randomButton);

    addAndMakeVisible(stepsLabel);
    stepsLabel.setFont(textFont);
    stepsLabel.attachToComponent(&stepsMenu, false);
    for (stepnum_t i = 1; i <= constants::MAX_STEPS; i++) {
        stepsMenu.addItem(std::to_string(i), static_cast<int>(i));
    }
    stepsMenu.onChange = [this] { onUpdateSteps(); };
    addAndMakeVisible(stepsMenu);
    stepsAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "steps", stepsMenu);

    addAndMakeVisible(voicesLabel);
    voicesLabel.setFont(textFont);
    voicesLabel.attachToComponent(&voicesMenu, false);
    for (voicenum_t i = 1; i <= constants::MAX_VOICES; i++) {
        voicesMenu.addItem(std::to_string(i), static_cast<int>(i));
    }
    voicesMenu.onChange = [this] { onUpdateVoices(); };
    addAndMakeVisible(voicesMenu);
    voicesAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "voices", voicesMenu);

    addAndMakeVisible(rateLabel);
    rateLabel.setFont(textFont);
    rateLabel.attachToComponent(&rateMenu, false);
    int rateIndex = 1;
    for (const juce::String &value: state.rateParameter->getAllValueStrings()) {
        rateMenu.addItem(value, rateIndex++);
    }
    addAndMakeVisible(rateMenu);
    rateAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "rate", rateMenu);

    int rateTypeIndex = 1;
    for (const juce::String &value: state.rateTypeParameter->getAllValueStrings()) {
        rateTypeMenu.addItem(value, rateTypeIndex++);
    }
    addAndMakeVisible(rateTypeMenu);
    rateTypeAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "rateType", rateTypeMenu);

    addAndMakeVisible(modeLabel);
    modeLabel.setFont(textFont);
    modeLabel.attachToComponent(&modeMenu, false);
    int modeIndex = 1;
    for (const juce::String &value: state.modeParameter->getAllValueStrings()) {
        modeMenu.addItem(value, modeIndex++);
    }
    addAndMakeVisible(modeMenu);
    modeAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "mode", modeMenu);
    modeMenu.onChange = [this] {
        // shouldn't do this in UI
        int mode = state.modeParameter->getIndex();
        if (mode == constants::modeChoices::Scale) {
            int scaleIndex = state.scaleParameter->getIndex();
            const std::vector<int> &scale = scales.allScales[static_cast<size_t>(scaleIndex)];
            *(state.voicesParameter) = static_cast<int>(juce::jmax(constants::MAX_VOICES, scale.size() + 1));
            // TODO: disable stretch params
        }
        refreshMessage();
        refreshEnabled();
    };

    addAndMakeVisible(scaleLabel);
    scaleLabel.setFont(textFont);
    scaleLabel.attachToComponent(&scaleMenu, false);
    int scaleIndex = 1;
    for (const juce::String &value: state.scaleParameter->getAllValueStrings()) {
        scaleMenu.addItem(value, scaleIndex++);
    }
    addAndMakeVisible(scaleMenu);
    scaleAttachment = std::make_unique<ComboBoxAttachment>(state.parameters, "scale", scaleMenu);

    juce::Image gear = FontAwesome::getInstance()->getIcon(true,
                                                           juce::String::fromUTF8(
                                                                   reinterpret_cast<const char *>(u8"\uf013")),
                                                           ICON_SIZE, constants::COLOR_TOGGLE_ACTIVE,
                                                           1);
    settingsButton.setImages(true, false, true, gear, 1.0f, {}, {}, 1.0f, {}, {}, 1.0f, constants::COLOR_TOGGLE_INACTIVE);
    settingsButton.setTooltip(constants::TOOLTIP_SETTINGS);
    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible(settingsButton);

    addAndMakeVisible(messageLabel);
    messageLabel.setFont(messageFont);
    messageLabel.setColour(juce::Label::ColourIds::textColourId, constants::COLOR_MESSAGE_TEXT);

    refresh();
}

void Header::showSettingsMenu() {
    juce::PopupMenu menu;
    menu.setLookAndFeel(&getLookAndFeel());
    int index = 1;

    menu.addItem(index++, "Matching pressed keys to voices:", false);
    auto current = state.voiceMatchingParameter->getCurrentValueAsText();
    for (const juce::String &value: state.voiceMatchingParameter->getAllValueStrings()) {
        menu.addItem(index++, value, true, value == current);
    }

    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(settingsButton),
        juce::ModalCallbackFunction::forComponent(settingsMenuItemChosenCallback, this));
}

void Header::settingsMenuItemChosenCallback(int result, Header* component) {
    if (component == nullptr) return;
    if (result == 0) return;

    auto voiceMatchingParameter = component->state.voiceMatchingParameter;
    int firstValue = 2;
    int lastValue = firstValue + voiceMatchingParameter->getAllValueStrings().size() - 1;
    if (result >= firstValue && result <= lastValue) {
        component->state.voiceMatchingParameter->beginChangeGesture();
        *(component->state.voiceMatchingParameter) = result - firstValue;
        component->state.voiceMatchingParameter->endChangeGesture();
        return;
    }
}

void Header::timerCallback() {
    if (state.recordButton != recordButton.getToggleState()) {
        recordButton.setToggleState(state.recordButton, juce::NotificationType::sendNotification);
    }
}

void Header::refresh() {
    refreshMessage();
    refreshEnabled();
}

void Header::refreshMessage() {
    juce::String text;
    if (recordButton.getToggleState()) {
        text = constants::MSG_RECORD;
    } else if (modeMenu.getSelectedItemIndex() == constants::modeChoices::Scale) {
        text = constants::MSG_SCALE;
    } else {
        text = "";
    }
    messageLabel.setText(text, juce::NotificationType::dontSendNotification);
    repaint();
}

void Header::refreshEnabled() {
    const bool notRecording = !state.recordButton;
    const int mode = state.modeParameter->getIndex();
    const bool hasScale = mode == constants::modeChoices::Scale;
    fileButton.setEnabled(notRecording);
    stepsMenu.setEnabled(notRecording);
    voicesMenu.setEnabled(notRecording);
    rateMenu.setEnabled(notRecording);
    rateTypeMenu.setEnabled(notRecording);
    modeMenu.setEnabled(notRecording);
    scaleMenu.setEnabled(notRecording && hasScale);
    settingsButton.setEnabled(notRecording);
    // TODO: disable stretch if mode is Chord/Scale?
    randomButton.setEnabled(notRecording);
}

void Header::paint(juce::Graphics &g) {
    auto area = getLocalBounds();
    if (messageLabel.getText() == "") {
        area = area.withTrimmedBottom(25);
    }
    g.setColour(constants::COLOR_HEADER);
    g.fillRect(area);
}

void Header::resized() {
    auto area = getLocalBounds();
    messageLabel.setBounds(area.removeFromBottom(25).withTrimmedRight(4).withTrimmedLeft(4).withTrimmedBottom(4));

    juce::FlexItem::Margin margin{0.0, 5.0, 0.0, 0.0};

    auto toolbar2Rect = area.removeFromBottom(48).reduced(8);
    juce::FlexBox toolbar2;
    toolbar2.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    toolbar2.alignItems = juce::FlexBox::AlignItems::center;
    toolbar2.items.add(juce::FlexItem(modeMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(130.0).withMargin(margin));
    toolbar2.items.add(juce::FlexItem(scaleMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(130.0).withMargin(margin));
    toolbar2.performLayout(toolbar2Rect);

    auto toolbarRect = area.removeFromBottom(48).reduced(8);
    juce::FlexBox toolbar;
    toolbar.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    toolbar.alignItems = juce::FlexBox::AlignItems::center;
    toolbar.items.add(
            juce::FlexItem(fileButton).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
                    ICON_SIZE + 10).withWidth(
                    static_cast<float>(fileButton.getWidth())).withMargin(margin));
    toolbar.items.add(juce::FlexItem(stepsMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(90.0).withMargin(margin));
    toolbar.items.add(juce::FlexItem(voicesMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(90.0).withMargin(margin));
    toolbar.items.add(juce::FlexItem(rateMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(130.0).withMargin(margin));
    toolbar.items.add(juce::FlexItem(rateTypeMenu).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            MENU_HEIGHT).withWidth(130.0).withMargin(margin));
    toolbar.items.add(juce::FlexItem(settingsButton).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            ICON_SIZE + 10).withWidth(
            static_cast<float>(settingsButton.getWidth())).withMargin({0.0, 5.0, 0.0, 5.0}));
    toolbar.items.add(juce::FlexItem(recordButton).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
            ICON_SIZE + 10).withWidth(
            static_cast<float>(recordButton.getWidth())).withMargin({0.0, 5.0, 0.0, 5.0}));
    toolbar.items.add(
            juce::FlexItem(randomButton).withAlignSelf(juce::FlexItem::AlignSelf::autoAlign).withHeight(
                    ICON_SIZE + 10).withWidth(
                    static_cast<float>(randomButton.getWidth())).withMargin({0.0, 0.0, 0.0, 5.0}));
    toolbar.performLayout(toolbarRect);
}
