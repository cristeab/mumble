#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>
#include <QList>
#include <QQmlEngine>

class Channel;

class RoomsModel : public QAbstractListModel
{
    Q_OBJECT

    QML_INTERFACE
    QML_READABLE_PROPERTY(int, currentRoomIndex, setCurrentRoomIndex, INVALID_INDEX)

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
    enum class ChannelType { Root, Class, Room, Other };

    explicit RoomsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    Q_INVOKABLE void clear();
    void append(const RoomInfo &roomInfo);
    Channel* channel(int index) const;
    void insertUser(Channel *channel, const QString &username);

    static ChannelType channelType(Channel *channel);

private:
    enum { INVALID_INDEX = -1 };
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _rooms.count()));
    }
    QList<RoomInfo> _rooms;
};

Q_DECLARE_INTERFACE(RoomsModel, "com.bubbles.RoomsModel")
