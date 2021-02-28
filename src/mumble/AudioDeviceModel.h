#pragma once

#include "qmlhelpers.h"
#include <QList>
#include <QTimer>

class AudioDeviceModel : public QObject {
    Q_OBJECT

    QML_WRITABLE_PROPERTY(bool, inputDeviceMute, setInputDeviceMute, false)
    QML_WRITABLE_PROPERTY(bool, inputDeviceAutoConnect, setInputDeviceAutoConnect, true)

    QML_READABLE_PROPERTY(QStringList, inputSystems, setInputSystems, QStringList())
    QML_WRITABLE_PROPERTY(int, inputSystemIndex, setInputSystemIndex, INVALID_INDEX)
    QML_READABLE_PROPERTY(QStringList, inputDevices, setInputDevices, QStringList())
    QML_WRITABLE_PROPERTY(int, inputDeviceIndex, setInputDeviceIndex, INVALID_INDEX)

    QML_WRITABLE_PROPERTY(bool, outputDeviceMute, setOutputDeviceMute, false)
    QML_WRITABLE_PROPERTY(bool, outputDeviceAutoConnect, setOutputDeviceAutoConnect, true)

    QML_READABLE_PROPERTY(QStringList, outputSystems, setOutputSystems, QStringList())
    QML_WRITABLE_PROPERTY(int, outputSystemIndex, setOutputSystemIndex, INVALID_INDEX)
    QML_READABLE_PROPERTY(QStringList, outputDevices, setOutputDevices, QStringList())
    QML_WRITABLE_PROPERTY(int, outputDeviceIndex, setOutputDeviceIndex, INVALID_INDEX)

    QML_WRITABLE_PROPERTY(double, sliderBelowValue, setSliderBelowValue, 0.25)
    QML_WRITABLE_PROPERTY(double, sliderAboveValue, setSliderAboveValue, 0.75)
    QML_READABLE_PROPERTY(double, micValue, setMicValue, 0)

    QML_WRITABLE_PROPERTY(int, quality, setQuality, 32000)
    QML_WRITABLE_PROPERTY(int, frames, setFrames, 3)
    QML_READABLE_PROPERTY(QString, bitRateText, setBitRateText, "")
    QML_READABLE_PROPERTY(bool, bitRateAlarm, setBitRateAlarm, false)

    QML_WRITABLE_PROPERTY(int, volume, setVolume, 100)
    QML_WRITABLE_PROPERTY(int, outputDelay, setOutputDelay, 10)
    QML_WRITABLE_PROPERTY(int, attenuation, setAttenuation, 50)
    QML_WRITABLE_PROPERTY(bool, whileOtherUsersTalk, setWhileOtherUsersTalk, false)
    QML_WRITABLE_PROPERTY(bool, whileYouTalk, setWhileYouTalk, false)
public:
    AudioDeviceModel(QObject *parent = nullptr);
    Q_INVOKABLE void init(bool input);

    Q_INVOKABLE int framesPerPacket(int value) {
        return (1 == value) ? 1 : 2 * (value - 1);
    }

private:
    void onDeviceMute();
    void onTickerTimeout();
    void updateBitrate();
    enum { INVALID_INDEX = -1, TICKER_PERIOD_MS = 20 };
    QTimer _ticker;
};
