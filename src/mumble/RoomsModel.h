#pragma once

#include <QAbstractListModel>
#include <QList>

class RoomsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum RoomRoles {
        Name = Qt::UserRole+1,
        Users
    };
    struct RoomInfo {
        QString name;
        QStringList users;
    };

    explicit RoomsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QHash<int,QByteArray> roleNames() const;

    void clear();
    void append(const RoomInfo &roomInfo);

    void fillDummy();

private:
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _rooms.count()));
    }
    QList<RoomInfo> _rooms;
};
