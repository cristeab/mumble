#pragma once

#include <QAbstractListModel>
#include <QList>

class Channel;

class RoomsModel : public QAbstractListModel
{
    Q_OBJECT
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

    explicit RoomsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    Q_INVOKABLE void clear();
    void append(const RoomInfo &roomInfo);
    Channel* channel(int index) const;
    void insertUser(Channel *channel, const QString &username);

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _rooms.count()));
    }
    QList<RoomInfo> _rooms;
    struct UserPosition {
        int roomIndex = -1;
        int userIndex = -1;
        bool isValid() const { return (0 <= roomIndex) && (0 <= userIndex); }
        void clear() { roomIndex = userIndex = -1; }
    } _userPosition;
};
