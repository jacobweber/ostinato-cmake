#pragma once

#include "juce_audio_utils/juce_audio_utils.h"

#include "../State.h"
#include "../Constants.h"

class FileButton : public juce::ImageButton {
public:
    explicit FileButton(State &_state) : state(_state) {
        juce::ImageButton();
    }

    void showFileMenu() {
        juce::PopupMenu menu;
        menu.addItem(1, juce::CharPointer_UTF8("Save Preset\u2026"));
        menu.addItem(2, "Open Presets Folder");
        menu.addItem(3, juce::CharPointer_UTF8("Export Settings\u2026"));
        menu.addItem(4, juce::CharPointer_UTF8("Import Settings\u2026"));
        menu.addSeparator();
        refreshPresetNames();
        menu.addItem(5, "<defaults>");
        int idx = 6;
        for (juce::File &preset : presetNames) {
            menu.addItem(idx++, preset.getFileNameWithoutExtension());
        }
        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
            juce::ModalCallbackFunction::forComponent(fileMenuItemChosenCallback, this));
    }

private:
    static void fileMenuItemChosenCallback(int result, FileButton* component) {
        if (component == nullptr) return;
        switch (result) {
            case 1: // save
                component->showSaveDialog();
                break;
            case 2: // show
                component->showPresetsDir();
                break;
            case 3: // export
                component->showExportDialog();
                break;
            case 4: // import
                component->showImportDialog();
                break;
            case 5: // reset
                component->state.resetToDefaults();
                break;
            default: // preset name
                // relies on order not changing after creating menu
                juce::File presetName = component->presetNames[result - 6];
                component->lastPresetName = presetName.getFileNameWithoutExtension();
                component->state.loadFromFile(presetName);
                break;
        }
    }

    static void saveDialogClosedCallback(int result, FileButton* component) {
        if (component == nullptr) return;
        auto& aw = *(component->asyncAlertWindow);
        aw.exitModalState(result);
        aw.setVisible(false);
        if (result != 1) return;
        component->lastPresetName = aw.getTextEditorContents("presetName");
        auto fileName = juce::File::createLegalFileName(component->lastPresetName + ".xml");
        auto presetsDir = component->getPresetsDir();
        if (!presetsDir.exists() && !presetsDir.createDirectory().wasOk()) return;
        component->state.saveToFile(presetsDir.getChildFile(fileName));
    }

    void showPresetsDir() {
        auto presetsDir = getPresetsDir();
        if (!presetsDir.exists() && !presetsDir.createDirectory().wasOk()) return;
        presetsDir.revealToUser();
    }

    juce::File getPresetsDir() {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            #if JUCE_MAC
            .getChildFile("Application Support")
            #endif
            .getChildFile(juce::JUCEApplication::getInstance()->getApplicationName())
            .getChildFile("Presets");
    }

    void refreshPresetNames() {
        auto presetsDir = getPresetsDir();
        if (!presetsDir.exists() && !presetsDir.createDirectory().wasOk()) {
            presetNames.clear();
            return;
        }
        presetNames = presetsDir.findChildFiles(juce::File::findFiles | juce::File::ignoreHiddenFiles, true, "*.xml");
        presetNames.sort();
    }

    void showSaveDialog() {
        asyncAlertWindow = std::make_unique<juce::AlertWindow>("Save Preset",
            "Enter the preset name.", juce::MessageBoxIconType::NoIcon);
        asyncAlertWindow->addTextEditor("presetName", lastPresetName, "Name:");
        asyncAlertWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
        asyncAlertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));
        asyncAlertWindow->getTextEditor("presetName")->setExplicitFocusOrder(1);
        asyncAlertWindow->enterModalState(true, juce::ModalCallbackFunction::forComponent(saveDialogClosedCallback, this));
    }

    void showExportDialog() {
        fc.reset(new juce::FileChooser("Enter file to export settings to.", {}, "*.xml",  true));
        fc->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& chooser) {
                auto result = chooser.getURLResult();
                if (result.isEmpty()) return;
                if (!result.isLocalFile()) return;
                state.saveToFile(result.getLocalFile());
            }
        );
    }

    void showImportDialog() {
        fc.reset(new juce::FileChooser("Select settings file to import.", {}, "*.xml", true));
        fc->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& chooser) {
                juce::String chosen;
                auto result = chooser.getURLResult();
                if (result.isEmpty()) return;
                if (!result.isLocalFile()) return;
                state.loadFromFile(result.getLocalFile());
            }
        );
    }

private:
    State &state;
    juce::String lastPresetName{""};
    juce::Array<juce::File> presetNames{};

    std::unique_ptr<juce::FileChooser> fc;
    std::unique_ptr<juce::AlertWindow> asyncAlertWindow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileButton)
};
