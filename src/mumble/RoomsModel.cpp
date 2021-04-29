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

void RoomsModel::append(const RoomInfo &roomInfo, const QHash<QString, unsigned int> &sessions)
{
    emit layoutAboutToBeChanged();
    _rooms << roomInfo;
    emit layoutChanged();
    QHashIterator<QString, unsigned int> it(sessions);
    while (it.hasNext()) {
        it.next();
        if (!_sessions.contains(it.key())) {
            _sessions[it.key()] = it.value();
        } else if (_sessions[it.key()] != it.value()) {
            qWarning() << it.key() << "already has session" << _sessions[it.key()] << "new" << it.value();
            _sessions[it.key()] = it.value();
        }
    }
}

Channel* RoomsModel::channel(int index) const
{
    Channel *ch = nullptr;
    if (isValidIndex(index)) {
        ch = _rooms.at(index).channel;
    }
    return ch;
}

void RoomsModel::insertUser(Channel *channel, const QString &username, unsigned int session)
{
    qInfo() << "Insert user" << username << session;
    const auto type = channelType(channel);
    if (ChannelType::Room == type) {
        //insert or replace session
        _sessions[username] = session;
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
        qWarning() << "No room" << channel->qsName << "has been found for" << username;
    } else {
        qWarning() << "Unknown channel type" << static_cast<int>(type);
    }
}

void RoomsModel::removeUser(const QString &username)
{
    if (username.isEmpty()) {
        qWarning() << "Empty username";
        return;//nothing to do
    }
    for (auto &roomInfo: _rooms) {
        if (roomInfo.users.contains(username)) {
            emit layoutAboutToBeChanged();
            roomInfo.users.removeAll(username);
            emit layoutChanged();
            _sessions.remove(username);
            qDebug() << "Removed" << username << "from room" << roomInfo.name;
            break;
        }
    }
}

void RoomsModel::updateRooms(int currentRoomIndex)
{
    emit layoutAboutToBeChanged();
    if (-1 < currentRoomIndex) {
        setCurrentRoomIndex(currentRoomIndex);
    }
    emit layoutChanged();
}

RoomsModel::ChannelType RoomsModel::channelType(Channel *channel)
{
    if (nullptr != channel) {
        if (nullptr == channel->cParent) {
            return ChannelType::Root;
        }
        if (nullptr == channel->cParent->cParent) {
            return ChannelType::School;
        }
        if (nullptr == channel->cParent->cParent->cParent) {
            return ChannelType::Class;
        }
        if (nullptr == channel->cParent->cParent->cParent->cParent) {
            return ChannelType::Room;
        }
    }
    return ChannelType::Other;
}

unsigned int RoomsModel::userSession(const QString &username) const
{
    const auto ok = _sessions.contains(username);
    const auto s = ok ? _sessions.value(username) : g.uiSession;
    if (!ok) {
        qWarning() << "Cannot find session for" << username;
    }
    return s;
}

void RoomsModel::onMicrophoneOffChanged()
{
    g.s.bMute = _microphoneOff;
    if (g.sh) {
        g.sh->setSelfMuteDeafState(g.s.bMute, g.s.bDeaf);
    }
}

void RoomsModel::onSpeakerOffChanged()
{
    AudioInputPtr ai = g.ai;
    if (ai) {
        ai->tIdle.restart();
    }
    g.s.bDeaf = _speakerOff;
    if (g.sh) {
        g.sh->setSelfMuteDeafState(g.s.bMute, g.s.bDeaf);
    }
}
