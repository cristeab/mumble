#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>
#include <qqml.h>
#include <QVector>

class Servers : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

    enum { DEFAULT_PORT = 64738 };

    QML_WRITABLE_PROPERTY(QString, address, setAddress, "")
    QML_WRITABLE_PROPERTY(int, port, setPort, DEFAULT_PORT)
    QML_WRITABLE_PROPERTY(QString, username, setUsername, "")
    QML_WRITABLE_PROPERTY(QString, label, setLabel, "")

public:
    struct ServerItem {
        QString name;
        int delayMs = 0;
        int currentUsers = 0;
        int totalUsers = 0;
        //internal values
        QString address;
        int port = 0;
        QString username;
    };

    explicit Servers(QObject *parent = nullptr);

    Q_INVOKABLE void resetServer(int index) {
        if (isValidIndex(index)) {
            const auto &srv = _servers.at(index);
            setAddress(srv.address);
            setPort(srv.port);
            setUsername(srv.username);
            setLabel(srv.name);
        } else {
            setAddress("");
            setPort(DEFAULT_PORT);
            setUsername("");
            setLabel("");
        }
    }
    Q_INVOKABLE void changeServer(int index);

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    enum { NAME = 0, DELAY, USERS, COLUMN_COUNT };
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _servers.count()));
    }
    void load();
    void save();

    QVector<ServerItem> _servers;
};
