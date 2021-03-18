#include "RoomsModel.h"
#include "Channel.h"
#include "ServerHandler.h"
#include "AudioInput.h"
#include "Global.h"

RoomsModel::RoomsModel(QObject *parent) : QAbstractListModel(parent)
{
    connect(this, &RoomsModel::microphoneOffChanged, this,
            &RoomsModel::onMicrophoneOffChanged);
    connect(this, &RoomsModel::speakerOffChanged, this,
            &RoomsModel::onSpeakerOffChanged);
}

int RoomsModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _rooms.size();
}

QVariant RoomsModel::data(const QModelIndex &index, int role) const
{
    if (!isValidIndex(index.row())) {
        qCritical() << "Invalid model index";
        return QVariant();
    }
    QVariant out;
    const auto &room = _rooms.at(index.row());
    switch (role) {
    case Name:
        out = room.name;
        break;
    case Users:
        out = room.users;
        break;
    default:
        qCritical() << "Unknown role" << role;
    }
    return out;
}

QHash<int,QByteArray> RoomsModel::roleNames() const
{
    static const auto roles = QHash<int, QByteArray> {
        { Name, "name" },
        { Users, "users" }
    };
    return roles;
}

void RoomsModel::clear()
{
    emit layoutAboutToBeChanged();
    _rooms.clear();
    emit layoutChanged();
}

void RoomsModel::append(const RoomInfo &roomInfo)
{
    emit layoutAboutToBeChanged();
    _rooms << roomInfo;
    emit layoutChanged();
}

Channel* RoomsModel::channel(int index) const
{
    Channel *ch = nullptr;
    if (isValidIndex(index)) {
        ch = _rooms.at(index).channel;
    }
    return ch;
}

void RoomsModel::insertUser(Channel *channel, const QString &username)
{
    const auto type = channelType(channel);
    if (ChannelType::Room == type) {
        //remove user from previous room
        for (auto &roomInfo: _rooms) {
            if (roomInfo.users.contains(username)) {
                roomInfo.users.removeAll(username);
                qDebug() << "Removed" << username << "from room" << roomInfo.name;
                break;
            }
        }
        //insert user in room
        for (int i = 0; i < _rooms.size(); ++i) {
            auto &roomInfo = _rooms[i];
            if (roomInfo.channel == channel) {
                emit layoutAboutToBeChanged();
                roomInfo.users << username;
                emit layoutChanged();
                setCurrentRoomIndex(INVALID_INDEX);//make sure the index is updated
                setCurrentRoomIndex(i);
                qDebug() << "Added" << username << "to room" << roomInfo.name;
                return;
            }
        }
        RoomInfo roomInfo;
        roomInfo.channel = channel;
        roomInfo.name = channel->qsName;
        roomInfo.users << username;
        emit layoutAboutToBeChanged();
        _rooms << roomInfo;
        emit layoutChanged();
        setCurrentRoomIndex(INVALID_INDEX);//make sure the index is updated
        setCurrentRoomIndex(_rooms.size() - 1);
        qDebug() << "Created for" << username << "new room" << roomInfo.name;
    } else {
        qWarning() << "Unknown channel type" << static_cast<int>(type);
    }
}

RoomsModel::ChannelType RoomsModel::channelType(Channel *channel)
{
    if (nullptr != channel) {
        if (nullptr == channel->cParent) {
            return ChannelType::Root;
        }
        if (nullptr == channel->cParent->cParent) {
            return ChannelType::Class;
        }
        if (nullptr == channel->cParent->cParent->cParent) {
            return ChannelType::Room;
        }
    }
    return ChannelType::Other;
}

void RoomsModel::onMicrophoneOffChanged()
{
    Global::get().s.bMute = _microphoneOff;
    if (Global::get().sh) {
        Global::get().sh->setSelfMuteDeafState(Global::get().s.bMute, Global::get().s.bDeaf);
    }
}

void RoomsModel::onSpeakerOffChanged()
{
    AudioInputPtr ai = Global::get().ai;
    if (ai) {
        ai->tIdle.restart();
    }
    Global::get().s.bDeaf = _speakerOff;
    if (Global::get().sh) {
        Global::get().sh->setSelfMuteDeafState(Global::get().s.bMute, Global::get().s.bDeaf);
    }
}
