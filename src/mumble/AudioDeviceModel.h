#pragma once

#include "qmlhelpers.h"
#include <QList>

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

public:
    AudioDeviceModel(QObject *parent = nullptr);
    Q_INVOKABLE void init(bool input);

private:
    void onDeviceMute();
    enum { INVALID_INDEX = -1 };
};
