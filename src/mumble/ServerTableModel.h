#pragma once

#include "qmlhelpers.h"
#include "UnresolvedServerAddress.h"
#include "ServerAddress.h"
#include "Timer.h"
#include "Mumble.pb.h"
#include "RoomsModel.h"

#include <QAbstractListModel>
#include <qqml.h>
#include <QVector>
#include <QTimer>
#include <QHostAddress>

#ifdef USE_ZEROCONF
#	include "BonjourRecord.h"
#	include <dns_sd.h>
#endif

class ModelItem;

class QUdpSocket;

class ServerTableModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

    enum { DEFAULT_PORT = 64738 };

    QML_WRITABLE_PROPERTY(QString, hostname, setHostname, QString())
    QML_WRITABLE_PROPERTY(int, port, setPort, DEFAULT_PORT)
    QML_WRITABLE_PROPERTY(QString, username, setUsername, QString())
    QML_READABLE_PROPERTY(QString, password, setPassword, QString())
    QML_WRITABLE_PROPERTY(QString, label, setLabel, QString())

    QML_WRITABLE_PROPERTY(int, currentIndex, setCurrentIndex, 0)
    QML_WRITABLE_PROPERTY(int, connectedServerIndex, setConnectedServerIndex, INVALID_INDEX)

    QML_WRITABLE_PROPERTY(int, currentClassIndex, setCurrentClassIndex, INVALID_INDEX)
    QML_WRITABLE_PROPERTY(int, connectedClassIndex, setConnectedClassIndex, INVALID_INDEX)

    QML_WRITABLE_PROPERTY(QString, dlgTitle, setDlgTitle, QString())
    QML_WRITABLE_PROPERTY(QString, dlgTextLabel, setDlgTextLabel, QString())
    QML_WRITABLE_PROPERTY(QString, dlgText, setDlgText, QString())
    QML_WRITABLE_PROPERTY(bool, dlgIsPassword, setDlgIsPassword, false)

    QML_WRITABLE_PROPERTY(QStringList, schoolNameList, setSchoolNameList, QStringList())
    QML_WRITABLE_PROPERTY(QStringList, classNameList, setClassNameList, QStringList())
    QML_CONSTANT_PROPERTY_PTR(RoomsModel, roomsModel)
    QML_READABLE_PROPERTY(QString, currentSchoolName, setCurrentSchoolName, QString())
    QML_READABLE_PROPERTY(QString, currentClassName, setCurrentClassName, QString())

public:
    struct ServerItem {
        QString name;
        double delayMs = 0;
        int currentUsers = 0;
        int totalUsers = 0;
        //internal values
        QString hostname;
        QString address;//should be resolved from hostname
        int port = 0;
        QString username;
        QString password;
        uint32_t version = 0;
        uint32_t bandwidth = 0;
        uint32_t pingSort = 0;
        uint32_t sent = 0;
        uint32_t recv = 0;
#ifdef USE_ZEROCONF
        QString zeroconfHost;
        BonjourRecord zeroconfRecord;
#endif
    };

    explicit ServerTableModel(QObject *parent = nullptr);

    Q_INVOKABLE void resetServer();
    Q_INVOKABLE void changeServer();
    Q_INVOKABLE void removeServer();
    Q_INVOKABLE void startPingTick(bool start);
    Q_INVOKABLE bool isReachable(int row) {
        return isValidIndex(row) ? (0 < _servers.at(row).totalUsers) : false;
    }
    Q_INVOKABLE bool connectServer();
    Q_INVOKABLE bool disconnectServer();
    Q_INVOKABLE void onLineEditDlgAccepted();

    Q_INVOKABLE bool gotoSchool(int index);
    Q_INVOKABLE bool gotoClass(int index);
    Q_INVOKABLE bool joinRoom(int index);
    Q_INVOKABLE bool gotoSchoolInternal();
    Q_INVOKABLE bool gotoClassInternal();
    Q_INVOKABLE bool joinRoomInternal();

    Q_INVOKABLE QString currentServerName() const {
        return isValidIndex(_currentIndex) ? _servers.at(_currentIndex).name : QString();
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    int columnCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void onServerDisconnectedEvent(MumbleProto::Reject_RejectType rtLast,
                                   const QString &reason);
    void onUserModelChanged();
    void onChannelJoined(Channel *channel, const QString &userName);

signals:
    void schoolsAvailable();
    void channelAllowedChanged(bool allowed);

private:
    enum { NAME = 0, DELAY, USERS, COLUMN_COUNT };
    enum { TICK_PERIOD_MS = 500, TICK_THRESHOLD_US = 1000000ULL, GRACE_PINGS = 4,
           INVALID_INDEX = -1 };
    bool isValidIndex(int index) const {
        return ((index >= 0) && (index < _servers.count()));
    }
    void load();
    void save();
    void timeTick();
    void sendPing(const QHostAddress &host, unsigned short port);
    void udpReply();
    void lookUp();
    void setStats(ServerItem *si, double delayUs, int users, int totalUsers);
    static void recreateServerHandler();
    void isAllowed(Channel *ch);

    QList<ServerItem> _servers;
    QTimer _pingTick;

    QHash< ServerAddress, quint64 > _pingRand;
    QHash< ServerAddress, QSet< ServerItem * > > _pings;
    QMap< UnresolvedServerAddress, unsigned int > _pingCache;
    Timer _pingTimer;
    Timer _currentTimer;
    bool _IPv4 = false;
    bool _IPv6 = false;
    QUdpSocket *_socket4 = nullptr;
    QUdpSocket *_socket6 = nullptr;
    QList<ModelItem*> _classModelItems;
    QList<ModelItem*> _schoolModelItems;
    int _channelActionIndex = -1;

#ifdef USE_ZEROCONF
protected:
    QList< BonjourRecord > qlBonjourActive;
public slots:
    void onUpdateLanList(const QList< BonjourRecord > &);

    void onResolved(const BonjourRecord, const QString, const uint16_t);
    void onLanResolveError(const BonjourRecord);
#endif
};
