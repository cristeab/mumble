#include "Servers.h"

Servers::Servers(QObject *parent) : QAbstractTableModel(parent)
{
    setObjectName("servers");
}

int Servers::rowCount(const QModelIndex&) const
{
    return _servers.size();
}

int Servers::columnCount(const QModelIndex&) const
{
    return COLUMN_COUNT;
}

QVariant Servers::data(const QModelIndex &index, int role) const
{
    QVariant out;
    const auto row = index.row();
    const auto col = index.column();
    switch (role) {
    case Qt::DisplayRole:
        //[[fallthrough]];
    case Qt::EditRole:
        if (isValidIndex(row)) {
            switch (col) {
            case NAME:
                out = _servers.at(row).name;
                break;
            case DELAY:
                out = QString("%1 ms").arg(_servers.at(row).delayMs);
                break;
            case USERS:
                out = QString("%1/%2").arg(_servers.at(row).currentUsers).arg(_servers.at(row).totalUsers);
                break;
            default:
                qWarning() << "Unknown column index" << col;
            }
        }
        break;
    default:
        ;
    }
    return out;
}

QHash<int, QByteArray> Servers::roleNames() const
{
    return { { Qt::DisplayRole, "display" },
             { Qt::EditRole, "edit" } };
}

QVariant Servers::headerData(int section, Qt::Orientation orientation, int role) const
{
    QString hdr;
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case NAME:
            hdr = tr("Server Name");
            break;
        case DELAY:
            hdr = tr("Delay");
            break;
        case USERS:
            hdr = tr("Users");
            break;
        default:
            ;
        }
    }
    return hdr;
}

void Servers::changeServer(int index)
{
    emit layoutAboutToBeChanged();
    if (isValidIndex(index)) {
        auto &server = _servers[index];
        server.address = _address;
        server.port = _port;
        server.username = _username;
        server.name = _label;
    } else {
        ServerItem server;
        server.address = _address;
        server.port = _port;
        server.username = _username;
        server.name = _label;
        _servers.append(server);
    }
    emit layoutChanged();
}