#pragma once

#include <QAbstractListModel>
#include <qqml.h>
#include <QVector>

class Servers : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

public:
    struct ServerItem {
        QString name;
        int delayMs = 0;
        int currentUsers = 0;
        int totalUsers = 0;
    };

    explicit Servers(QObject *parent = nullptr);
    void init();

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
    QVector<ServerItem> _servers;
};
