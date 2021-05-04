#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>
#include <QHash>

class Channel;

class RoomsModel : public QAbstractListModel
{
    Q_OBJECT

    QML_INTERFACE
    QML_READABLE_PROPERTY(int, currentRoomIndex, setCurrentRoomIndex, INVALID_INDEX)
    QML_WRITABLE_PROPERTY(bool, microphoneOff, setMicrophoneOff, false)
    QML_WRITABLE_PROPERTY(bool, speakerOff, setSpeakerOff, false)

public:
    enum RoomRoles {
        Name = Qt::UserRole+1,
        Users
    };
    struct RoomInfo {
        Channel *channel = nullptr;
        QString name;
        QStringList users;
    };
    enum class ChannelType { Root, School, Class, Room, Other };

    explicit RoomsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    Q_INVOKABLE void clear();
    void append(const RoomInfo &roomInfo, const QHash<QString, unsigned int> &sessions);
    Channel* channel(int index) const;
    void insertUser(Channel *channel, const QString &username, unsigned int session);
    void removeUser(const QString &username);
    void updateRooms(int currentRoomIndex);

    static ChannelType channelType(Channel *channel);
    unsigned int userSession(const QString &username) const;

signals:
    void forceLayout();

private:
    enum { INVALID_INDEX = -1 };
    void onMicrophoneOffChanged();
    void onSpeakerOffChanged();
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _rooms.count()));
    }
    QList<RoomInfo> _rooms;
    QHash<QString, unsigned int> _sessions;
};

Q_DECLARE_INTERFACE(RoomsModel, "com.bubbles.RoomsModel")
