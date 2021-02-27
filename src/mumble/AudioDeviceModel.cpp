#include "MainWindow.h"
#include "Global.h"
#include "ServerHandler.h"
#include "AudioDeviceModel.h"
#include "AudioInput.h"
#include "AudioOutput.h"
#include <QDebug>

AudioDeviceModel::AudioDeviceModel(QObject *parent) : QObject(parent)
{
    setObjectName("audioDevices");

    connect(this, &AudioDeviceModel::inputDeviceMuteChanged, this,
            &AudioDeviceModel::onDeviceMute);
    connect(this, &AudioDeviceModel::outputDeviceMuteChanged, this,
            &AudioDeviceModel::onDeviceMute);

    connect(this, &AudioDeviceModel::inputDeviceIndexChanged, this, [this]() {
        if ((0 <= _inputSystemIndex) && (_inputSystemIndex < _inputSystems.size())) {
            const auto &system = _inputSystems.at(_inputSystemIndex);
            g.s.inputSystemIndex = _inputSystemIndex;
            auto *air = AudioInputRegistrar::qmNew->value(system);
            if ((0 <= _inputDeviceIndex) && (_inputDeviceIndex < _inputDevices.size())) {
                const auto &device = _inputDevices.at(_inputDeviceIndex);
                air->setDeviceChoice(device, g.s);
                g.s.inputDeviceIndex = _inputDeviceIndex;
                qInfo() << "Current input device" << device;
            } else {
                qCritical() << "Invalid input device index" << _inputDeviceIndex;
            }
        } else {
            qCritical() << "Invalid input system index" << _inputSystemIndex;
        }
    });
    connect(this, &AudioDeviceModel::outputDeviceIndexChanged, this, [this]() {
        if ((0 <= _outputSystemIndex) && (_outputSystemIndex < _outputSystems.size())) {
            const auto &system = _outputSystems.at(_outputSystemIndex);
            g.s.outputSystemIndex = _outputSystemIndex;
            auto *aor = AudioOutputRegistrar::qmNew->value(system);
            if ((0 <= _outputDeviceIndex) && (_outputDeviceIndex < _outputDevices.size())) {
                const auto &device = _outputDevices.at(_outputDeviceIndex);
                aor->setDeviceChoice(device, g.s);
                g.s.outputDeviceIndex = _outputDeviceIndex;
                qInfo() << "Current output device" << device;
            } else {
                qCritical() << "Invalid output device index" << _outputDeviceIndex;
            }
        } else {
            qCritical() << "Invalid output system index" << _outputSystemIndex;
        }
    });

    _ticker.setSingleShot(false);
    _ticker.setInterval(TICKER_PERIOD_MS);
    connect(&_ticker, &QTimer::timeout, this, &AudioDeviceModel::onTickerTimeout);
}

void AudioDeviceModel::init(bool input)
{
    if (input && AudioInputRegistrar::qmNew) {
        qInfo() << "Init audio input";
        const auto &keys = AudioInputRegistrar::qmNew->keys();
        setInputSystems(keys);
        if (!_inputSystems.isEmpty()) {
            auto *air = AudioInputRegistrar::qmNew->value(_inputSystems.at(0));
            const auto &devs = air->getDeviceChoices();
            _inputDevices.clear();
            for (const auto &it: devs) {
                _inputDevices << it.first;
            }
            emit inputDevicesChanged();
        }
        //load settings
        setInputDeviceMute(g.s.bMute);
        setInputSystemIndex(g.s.inputSystemIndex);
        setInputDeviceIndex(g.s.inputDeviceIndex);
        //start ticker for audio bar
        _ticker.start();
    }
    if (!input && AudioOutputRegistrar::qmNew) {
        qInfo() << "Init audio output";
        const auto &keys = AudioOutputRegistrar::qmNew->keys();
        setOutputSystems(keys);
        if (!_outputSystems.isEmpty()) {
            auto *air = AudioOutputRegistrar::qmNew->value(_outputSystems.at(0));
            const auto &devs = air->getDeviceChoices();
            _outputDevices.clear();
            for (const auto &it: devs) {
                _outputDevices << it.first;
            }
            emit outputDevicesChanged();
        }
        //load settings
        setOutputDeviceMute(g.s.bDeaf);
        setOutputSystemIndex(g.s.outputSystemIndex);
        setOutputDeviceIndex(g.s.outputDeviceIndex);
    }
}

void AudioDeviceModel::onDeviceMute()
{
    g.s.bMute = _inputDeviceMute;
    g.s.bDeaf = _outputDeviceMute;
    if (g.sh) {
        g.sh->setSelfMuteDeafState(g.s.bMute, g.s.bDeaf);
    } else {
        qWarning() << "Cannot set self mute/deaf";
    }
}

void AudioDeviceModel::onTickerTimeout()
{
    AudioInputPtr ai = g.ai;
    if (!ai.get() || !ai->getSppPreprocess()) {
        return;
    }

    //Amplitude
    const auto amplitude = (96.0f + ai->dPeakCleanMic) / 96.0f;
    setMicValue(amplitude);
}
