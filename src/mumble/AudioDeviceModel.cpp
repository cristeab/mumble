#include "AudioDeviceModel.h"
#include "AudioInput.h"
#include "AudioOutput.h"
#include <QDebug>

AudioDeviceModel::AudioDeviceModel(QObject *parent) : QObject(parent)
{
    setObjectName("audioDevices");
}

void AudioDeviceModel::init(bool input)
{
    if (input && AudioInputRegistrar::qmNew) {
        qInfo() << "Init audio input";
        const auto &keys = AudioInputRegistrar::qmNew->keys();
        setInputSystems(keys);
        if (!_inputSystems.isEmpty()) {
            setInputSystemIndex(0);
            auto *air = AudioInputRegistrar::qmNew->value(_inputSystems.at(0));
            const auto &devs = air->getDeviceChoices();
            _inputDevices.clear();
            for (const auto &it: devs) {
                _inputDevices << it.first;
            }
            emit inputDevicesChanged();
        } else {
            setInputSystemIndex(INVALID_INDEX);
        }
        if (!_inputDevices.isEmpty()) {
            setInputDeviceIndex(0);
        } else {
            setInputDeviceIndex(INVALID_INDEX);
        }
    }
    if (!input && AudioOutputRegistrar::qmNew) {
        qInfo() << "Init audio output";
        const auto &keys = AudioOutputRegistrar::qmNew->keys();
        setOutputSystems(keys);
        if (!_outputSystems.isEmpty()) {
            setOutputSystemIndex(0);
            auto *air = AudioOutputRegistrar::qmNew->value(_outputSystems.at(0));
            const auto &devs = air->getDeviceChoices();
            _outputDevices.clear();
            for (const auto &it: devs) {
                _outputDevices << it.first;
            }
            emit outputDevicesChanged();
        } else {
            setOutputSystemIndex(INVALID_INDEX);
        }
        if (!_outputDevices.isEmpty()) {
            setOutputDeviceIndex(0);
        } else {
            setOutputDeviceIndex(INVALID_INDEX);
        }
    }
}
