#include "RoomsModel.h"
#include "Channel.h"
#include <QQmlEngine>

RoomsModel::RoomsModel(QObject *parent) : QAbstractListModel(parent)
{
    qmlRegisterInterface<RoomsModel>("RoomsModel", 1);
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
    _userPosition.clear();
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
    //remove user from previous room
    if (_userPosition.isValid() && isValidIndex(_userPosition.roomIndex)) {
        _rooms[_userPosition.roomIndex].users.removeAt(_userPosition.userIndex);
    }

    for (int i = 0; i < _rooms.size(); ++i) {
        auto &roomInfo = _rooms[i];
        if (roomInfo.channel == channel) {
            emit layoutAboutToBeChanged();
            roomInfo.users << username;
            emit layoutChanged();
            _userPosition.roomIndex = i;
            _userPosition.userIndex = roomInfo.users.size() - 1;
            return;
        }
    }
    qWarning() << "Create new room" << username;
    RoomInfo roomInfo;
    roomInfo.channel = channel;
    roomInfo.name = channel->qsName;
    roomInfo.users << username;
    emit layoutAboutToBeChanged();
    _rooms << roomInfo;
    emit layoutChanged();
    _userPosition.roomIndex = _rooms.size() - 1;
    _userPosition.userIndex = roomInfo.users.size() - 1;
}
