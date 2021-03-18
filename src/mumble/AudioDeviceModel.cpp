#include "MainWindow.h"
#include "Global.h"
#include "ServerHandler.h"
#include "AudioDeviceModel.h"
#include "AudioInput.h"
#include "AudioOutput.h"
#include "NetworkConfig.h"
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
            Global::get().s.inputSystemIndex = _inputSystemIndex;
            auto *air = AudioInputRegistrar::qmNew->value(system);
            if ((0 <= _inputDeviceIndex) && (_inputDeviceIndex < _inputDevices.size())) {
                const auto &device = _inputDevices.at(_inputDeviceIndex);
                air->setDeviceChoice(device, Global::get().s);
                Global::get().s.inputDeviceIndex = _inputDeviceIndex;
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
            Global::get().s.outputSystemIndex = _outputSystemIndex;
            auto *aor = AudioOutputRegistrar::qmNew->value(system);
            if ((0 <= _outputDeviceIndex) && (_outputDeviceIndex < _outputDevices.size())) {
                const auto &device = _outputDevices.at(_outputDeviceIndex);
                aor->setDeviceChoice(device, Global::get().s);
                Global::get().s.outputDeviceIndex = _outputDeviceIndex;
                qInfo() << "Current output device" << device;
            } else {
                qCritical() << "Invalid output device index" << _outputDeviceIndex;
            }
        } else {
            qCritical() << "Invalid output system index" << _outputSystemIndex;
        }
    });

    connect(this, &AudioDeviceModel::sliderBelowValueChanged, this, [this]() {
        Global::get().s.fVADmin = _sliderBelowValue;
    });
    connect(this, &AudioDeviceModel::sliderAboveValueChanged, this, [this]() {
        Global::get().s.fVADmax = _sliderAboveValue;
    });
    connect(this, &AudioDeviceModel::framesChanged, this, [this]() {
        Global::get().s.iFramesPerPacket = framesPerPacket(_frames);
        updateBitrate();
    });
    connect(this, &AudioDeviceModel::qualityChanged, this, [this]() {
        Global::get().s.iQuality = _quality;
        updateBitrate();
    });

    connect(this, &AudioDeviceModel::volumeChanged, this, [this]() {
        Global::get().s.fVolume = _volume / 100.0;
    });
    connect(this, &AudioDeviceModel::outputDelayChanged, this, [this]() {
        Global::get().s.iOutputDelay = _outputDelay;
    });
    connect(this, &AudioDeviceModel::attenuationChanged, this, [this]() {
        Global::get().s.fOtherVolume = 1.0f - _attenuation / 100.0f;
    });
    connect(this, &AudioDeviceModel::whileOtherUsersTalkChanged, this, [this]() {
        Global::get().s.bAttenuateOthers = _whileOtherUsersTalk;
    });
    connect(this, &AudioDeviceModel::whileYouTalkChanged, this, [this]() {
        Global::get().s.bAttenuateOthersOnTalk = _whileYouTalk;
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
        setInputDeviceMute(Global::get().s.bMute);
        setInputSystemIndex(Global::get().s.inputSystemIndex);
        setInputDeviceIndex(Global::get().s.inputDeviceIndex);
        setSliderBelowValue(Global::get().s.fVADmin);
        setSliderAboveValue(Global::get().s.fVADmax);
        setFrames((1 == Global::get().s.iFramesPerPacket) ? 1 : (1 + Global::get().s.iFramesPerPacket / 2));
        setQuality(Global::get().s.iQuality);
        updateBitrate();
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
        setOutputDeviceMute(Global::get().s.bDeaf);
        setOutputSystemIndex(Global::get().s.outputSystemIndex);
        setOutputDeviceIndex(Global::get().s.outputDeviceIndex);
        setVolume(std::round(100 * Global::get().s.fVolume));
        setOutputDelay(Global::get().s.iOutputDelay);
        setAttenuation(std::round((1.0f - Global::get().s.fOtherVolume) * 100.0f + 0.5f));
        setWhileOtherUsersTalk(Global::get().s.bAttenuateOthers);
        setWhileYouTalk(Global::get().s.bAttenuateOthersOnTalk);
    }
}

void AudioDeviceModel::onDeviceMute()
{
    Global::get().s.bMute = _inputDeviceMute;
    Global::get().s.bDeaf = _outputDeviceMute;
    if (Global::get().sh) {
        Global::get().sh->setSelfMuteDeafState(Global::get().s.bMute, Global::get().s.bDeaf);
    } else {
        qWarning() << "Cannot set self mute/deaf";
    }
}

void AudioDeviceModel::onTickerTimeout()
{
    AudioInputPtr ai = Global::get().ai;
    if (!ai.get() || !ai->getSppPreprocess()) {
        return;
    }

    //Amplitude
    const auto amplitude = (96.0f + ai->dPeakCleanMic) / 96.0f;
    setMicValue(amplitude);
}

void AudioDeviceModel::updateBitrate()
{
    // 50 packets, in bits, IP + UDP + Crypt + type + seq + frameheader
    int overhead = 100 * 8 * (20 + 8 + 4 + 1 + 2 + _frames);
    // TCP is 12 more bytes than UDP
    if (NetworkConfig::TcpModeEnabled()) {
        overhead += 100 * 8 * 12;
    }

    int posrate = Global::get().s.bTransmitPosition ? 12 : 0;
    posrate = posrate * 100 * 8;

    overhead = overhead / _frames;
    posrate  = posrate / _frames;

    const int total = _quality + overhead + posrate;
    setBitRateAlarm(Global::get().uiSession && (total > Global::get().iMaxBandwidth));

    setBitRateText(tr("%1 kbit/s (Audio %2, Position %4, Overhead %3)")
                    .arg(total / 1000.0, 0, 'f', 1)
                    .arg(_quality / 1000.0, 0, 'f', 1)
                    .arg(overhead / 1000.0, 0, 'f', 1)
                    .arg(posrate / 1000.0, 0, 'f', 1));
}
