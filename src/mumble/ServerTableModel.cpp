#include "MainWindow.h"
#include "ServerTableModel.h"
#include "Database.h"
#include "Global.h"
#include "ServerResolver.h"
#include "ServerHandler.h"
#include "UserModel.h"
#include "Channel.h"

#include <QRandomGenerator>
#include <QUdpSocket>
#include <QtEndian>
#include <QThread>

ServerTableModel::ServerTableModel(QObject *parent) : QAbstractTableModel(parent)
{
    setObjectName(QString::fromUtf8("servers"));

    _socket4 = new QUdpSocket(this);
    _socket6 = new QUdpSocket(this);
    _IPv4      = _socket4->bind(QHostAddress(QHostAddress::Any), 0);
    _IPv6      = _socket6->bind(QHostAddress(QHostAddress::AnyIPv6), 0);
    QObject::connect(_socket4, &QUdpSocket::readyRead, this, &ServerTableModel::udpReply);
    QObject::connect(_socket6, &QUdpSocket::readyRead, this, &ServerTableModel::udpReply);

    QObject::connect(&_pingTick, &QTimer::timeout, this, &ServerTableModel::timeTick);
    _pingTick.setInterval(TICK_PERIOD_MS);

    load();
}

int ServerTableModel::rowCount(const QModelIndex&) const
{
    return _servers.size();
}

int ServerTableModel::columnCount(const QModelIndex&) const
{
    return COLUMN_COUNT;
}

QVariant ServerTableModel::data(const QModelIndex &index, int role) const
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
            case NAME: {
                const auto name = _servers.at(row).name;
                out = !name.isEmpty() ? name : _servers.at(row).hostname;
            }
                break;
            case DELAY:
                out = QString(QString::fromUtf8("%1 ms")).arg(_servers.at(row).delayMs);
                break;
            case USERS:
                out = QString(QString::fromUtf8("%1/%2")).arg(_servers.at(row).currentUsers).arg(_servers.at(row).totalUsers);
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

QHash<int, QByteArray> ServerTableModel::roleNames() const
{
    return { { Qt::DisplayRole, "display" },
             { Qt::EditRole, "edit" } };
}

QVariant ServerTableModel::headerData(int section, Qt::Orientation orientation, int role) const
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

void ServerTableModel::resetServer()
{
    if (isValidIndex(_currentIndex)) {
        const auto &srv = _servers.at(_currentIndex);
        setHostname(srv.hostname);
        setPort(srv.port);
        setUsername(srv.username);
        setPassword(srv.password);
        setLabel(srv.name);
    } else {
        setHostname(QString());
        setPort(DEFAULT_PORT);
        setUsername(QString());
        setPassword(QString());
        setLabel(QString());
    }
}

void ServerTableModel::changeServer()
{
    emit layoutAboutToBeChanged();
    if (isValidIndex(_currentIndex)) {
        auto &server = _servers[_currentIndex];
        server.hostname = _hostname;
        server.port = _port;
        server.username = _username;
        server.password = _password;
        server.name = _label;
    } else {
        ServerItem server;
        server.hostname = _hostname;
        server.port = _port;
        server.username = _username;
        server.password = _password;
        server.name = _label;
        _servers.append(server);
    }
    lookUp();
    emit layoutChanged();
    save();
}

void ServerTableModel::removeServer()
{
    if (isValidIndex(_currentIndex)) {
        emit layoutAboutToBeChanged();
        _servers.removeAt(_currentIndex);
        emit layoutChanged();
        save();
        if (_currentIndex == _connectedServerIndex) {
            disconnectServer();
            setConnectedServerIndex(INVALID_INDEX);
        }
        setCurrentIndex(INVALID_INDEX);
    }
}

void ServerTableModel::startPingTick(bool start)
{
    qDebug() << "startPingTick" << start;
    if (start) {
        _pingTick.start();
    } else {
        _pingTick.stop();
    }
}

void ServerTableModel::load()
{
    if (nullptr == g.db) {
        qCritical() << "Cannot access db";
        return;
    }
    auto items = g.db->getFavorites();
    emit layoutAboutToBeChanged();
    for (const auto &it: qAsConst(items)) {
        ServerItem srvItem;
        srvItem.hostname = it.qsHostname;
        srvItem.address = it.qsUrl;
        srvItem.port = it.usPort;
        srvItem.username = it.qsUsername;
        srvItem.password = it.qsPassword;
        srvItem.name = it.qsName;
        _servers << srvItem;
    }
    emit layoutChanged();
    lookUp();
}

void ServerTableModel::save()
{
    if (nullptr == g.db) {
        qCritical() << "Cannot access db";
        return;
    }
    QList<FavoriteServer> favs;
    for (const auto &it: qAsConst(_servers)) {
        FavoriteServer favSrv;
        favSrv.qsHostname = it.hostname;
        favSrv.qsPassword = it.password;
        favSrv.qsUrl = it.address;
        favSrv.usPort = it.port;
        favSrv.qsUsername = it.username;
        favSrv.qsName = it.name;
        favs << favSrv;
    }
    g.db->setFavorites(favs);
}

void ServerTableModel::timeTick()
{
    for (const auto &it: qAsConst(_servers)) {
        const auto host = QHostAddress(it.address);
        if (!host.isNull()) {
            sendPing(host, it.port);
        }
    }
}

void ServerTableModel::sendPing(const QHostAddress &host, unsigned short port)
{
    char blob[16];

    ServerAddress addr(HostAddress(host), port);

    quint64 uiRand = 0;
    if (_pingRand.contains(addr)) {
        uiRand = _pingRand.value(addr);
    } else {
        uiRand = QRandomGenerator::global()->generate64() << 32;
        _pingRand.insert(addr, uiRand);
    }

    memset(blob, 0, sizeof(blob));
    *reinterpret_cast< quint64 * >(blob + 8) = _pingTimer.elapsed() ^ uiRand;

    if (_IPv4 && host.protocol() == QAbstractSocket::IPv4Protocol) {
        _socket4->writeDatagram(blob + 4, 12, host, port);
    } else if (_IPv6 && host.protocol() == QAbstractSocket::IPv6Protocol) {
        _socket6->writeDatagram(blob + 4, 12, host, port);
    } else {
        return;
    }

    const QSet< ServerItem * > &qs = _pings.value(addr);
    for (auto *si: qs) {
        //test the number of received packets
        if ((si->sent - si->recv) > GRACE_PINGS) {
            setStats(si, 0, 0, 0);
            si->sent = 0;
            si->recv = 0;
        }

        ++si->sent;
    }
}

void ServerTableModel::udpReply()
{
    QUdpSocket *sock = qobject_cast< QUdpSocket * >(sender());

    while (sock->hasPendingDatagrams()) {
        char blob[64];

        QHostAddress host;
        unsigned short port;

        const qint64 len = sock->readDatagram(blob + 4, 24, &host, &port);
        if (len == 24) {
            if (host.scopeId() == QLatin1String("0"))
                host.setScopeId(QLatin1String(""));

            ServerAddress address(HostAddress(host), port);

            if (_pings.contains(address)) {
                quint32 *ping = reinterpret_cast< quint32 * >(blob + 4);
                quint64 *ts   = reinterpret_cast< quint64 * >(blob + 8);

                quint64 elapsedUs = _pingTimer.elapsed() - (*ts ^ _pingRand.value(address));

                for (auto *si: _pings.value(address)) {
                    ++si->recv;
                    si->version    = qFromBigEndian(ping[0]);
                    quint32 users    = qFromBigEndian(ping[3]);
                    quint32 maxusers = qFromBigEndian(ping[4]);
                    si->bandwidth  = qFromBigEndian(ping[5]);

                    if (!si->pingSort)
                        si->pingSort = _pingCache.value(UnresolvedServerAddress(si->hostname, si->port));
                    setStats(si, static_cast< double >(elapsedUs), users, maxusers);
                }
            }
        }
    }
}

void ServerTableModel::lookUp()
{
    for (int i = 0; i < _servers.size(); ++i) {
        const auto &srv = _servers[i];
        qDebug() << "lookup" << srv.hostname << srv.address;
        if (!srv.hostname.isEmpty()) {
            const QHostAddress addr(srv.hostname);
            if (!addr.isNull()) {
                qInfo() << "Server hostname is an IP address";
                _servers[i].address = srv.hostname;
                pingServer(&_servers[i]);
            } else {
                QHostInfo::lookupHost(srv.hostname, this, [this, i](const QHostInfo &hi) {
                    if (!isValidIndex(i)) {
                        return;
                    }
                    const auto addrList = hi.addresses();
                    for (const auto &it: addrList) {
                        if (!it.isNull()) {
                            const auto &addr = it.toString();
                            if (!addr.isEmpty()) {
                                qDebug() << "Got address for host" << hi.hostName() << addr;
                                _servers[i].address = addr;
                                pingServer(&_servers[i]);
                                break;
                            }
                        }
                    }
                });
            }
        } else {
            qCritical() << "Found server with empty hostname" << i;
        }
    }
}

void ServerTableModel::pingServer(ServerItem *srv)
{
    if (nullptr != srv) {
        ServerAddress addr(QHostAddress(srv->address), srv->port);
        _pings[addr].insert(srv);
    }
}

void ServerTableModel::setStats(ServerItem *si, double delayUs, int users, int totalUsers)
{
    emit layoutAboutToBeChanged();
    si->delayMs = delayUs / 1000;
    si->currentUsers = users;
    si->totalUsers = totalUsers;
    emit layoutChanged();
    //force current index update
    const int cur = _currentIndex;
    setCurrentIndex(INVALID_INDEX);
    setCurrentIndex(cur);
}

bool ServerTableModel::connectServer()
{
    qInfo() << "Connect" << _currentIndex;
    if (!isValidIndex(_currentIndex)) {
        qCritical() << "Invalid index" << _currentIndex;
        return false;
    }

    //disconnect previous server
    if (_currentIndex == _connectedServerIndex) {
        qDebug() << "Cannot connect: nothing to do";
        return true;//should not happen
    }
    if (isValidIndex(_connectedServerIndex)) {
        qInfo() << "Disconnect current" << _connectedServerIndex;
        if (!disconnectServer()) {
            return false;
        }
    }

    recreateServerHandler();
    const auto &srv = _servers.at(_currentIndex);
    g.sh->setConnectionInfo(srv.address, srv.port, srv.username, srv.password);
    g.sh->start(QThread::TimeCriticalPriority);
    setConnectedServerIndex(_currentIndex);
    setCurrentUsername(srv.username);
    qDebug() << "Connected server index" << _connectedServerIndex;

    return true;
}

void ServerTableModel::recreateServerHandler()
{
    ServerHandlerPtr sh = g.sh;
    if (sh && sh->isRunning() && (nullptr != g.mw)) {
        g.mw->on_qaServerDisconnect_triggered();
        sh->disconnect();
        sh->wait();
        QCoreApplication::instance()->processEvents();
    }

    g.sh.reset();
    while (sh && !sh.unique()) {
        QThread::yieldCurrentThread();
    }
    sh.reset();

    sh = ServerHandlerPtr(new ServerHandler());
    sh->moveToThread(sh.get());
    g.sh = sh;
    if (nullptr != g.mw) {
        g.mw->connect(sh.get(), SIGNAL(connected()), g.mw, SLOT(serverConnected()));
        g.mw->connect(sh.get(), SIGNAL(disconnected(QAbstractSocket::SocketError, QString)), g.mw,
                      SLOT(serverDisconnected(QAbstractSocket::SocketError, QString)));
        g.mw->connect(sh.get(), SIGNAL(error(QAbstractSocket::SocketError, QString)), g.mw,
                      SLOT(resolverError(QAbstractSocket::SocketError, QString)));
    }
}

bool ServerTableModel::disconnectServer()
{
    qInfo() << "Disconnect";
    if (g.sh && g.sh->isRunning()) {
        g.sh->disconnect();
    } else {
        qWarning() << "Cannot disconnect: nothing to do";
    }
    setConnectedServerIndex(INVALID_INDEX);
    setConnectedClassIndex(INVALID_INDEX);
    qDebug() << "Connected server index" << _connectedServerIndex;
    if (nullptr != _roomsModel) {
        _roomsModel->clear();
    }
    setCurrentUsername(QString());
    return true;
}

void ServerTableModel::onLineEditDlgAccepted()
{
    if (_dlgIsPassword) {
        setPassword(_dlgText);
    } else {
        setUsername(_dlgText);
    }
    if (isValidIndex(_currentIndex)) {
        auto &server = _servers[_currentIndex];
        server.username = _username;
        server.password = _password;
        save();
    } else {
        qWarning() << "Invalid index" << _currentIndex;
    }

    if (!g.s.bSuppressIdentity) {
        g.db->setPassword(_hostname, _port, _username, _password);
    }
    g.sh->setConnectionInfo(_hostname, _port, _username, _password);
    g.mw->on_Reconnect_timeout();
}

void ServerTableModel::onServerDisconnectedEvent(MumbleProto::Reject_RejectType rtLast,
                                                 const QString &reason)
{
    qDebug() << "onServerDisconnectedEvent" << rtLast << reason;

    QString uname, pw, host;
    unsigned short port;
    g.sh->getConnectionInfo(host, port, uname, pw);
    setHostname(host);
    setPort(port);
    setUsername(uname);
    setPassword(pw);

    switch (rtLast) {
    case MumbleProto::Reject_RejectType_InvalidUsername:
        setDlgTitle(tr("Invalid username"));
        setDlgTextLabel(tr("You connected with an invalid username, please try another one."));
        setDlgText(_username);
        setDlgIsPassword(false);
        break;
    case MumbleProto::Reject_RejectType_UsernameInUse:
        setDlgTitle(tr("Username in use"));
        setDlgTextLabel(tr("That username is already in use, please try another username."));
        setDlgText(_username);
        setDlgIsPassword(false);
        break;
    case MumbleProto::Reject_RejectType_WrongUserPW:
        setDlgTitle(tr("Wrong certificate or password"));
        setDlgTextLabel(tr("Wrong certificate or password for registered user. If you are\n"
                                                  "certain this user is protected by a password please retry.\n"
                                                  "Otherwise abort and check your certificate and username."));
        setDlgText(_password);
        setDlgIsPassword(true);
        break;
    case MumbleProto::Reject_RejectType_WrongServerPW:
        setDlgTitle(tr("Wrong password"));
        setDlgTextLabel(tr("Wrong server password for unregistered user account, please try again."));
        setDlgText(_password);
        setDlgIsPassword(true);
        break;
    default:
        ;
    }
    if (g.s.bReconnect && !reason.isEmpty()) {
        g.mw->qaServerDisconnect->setEnabled(true);
        if (g.mw->bRetryServer) {
            g.mw->qtReconnect->start();
        }
    }
}

void ServerTableModel::onUserModelChanged()
{
    auto *userModel = g.mw->pmModel;
    if (nullptr != userModel) {
        _schoolNameList.clear();
        _schoolModelItems.clear();
        const auto *rootItem = userModel->rootItem();
        for (auto *child: rootItem->qlChildren) {
            _schoolNameList << child->cChan->qsName;
            _schoolModelItems << child;
        }
        emit schoolNameListChanged();
        if (1 == _schoolNameList.size()) {
            emit schoolsAvailable();
            qDebug() << "onUserModelChanged";
        }
    } else {
        qWarning() << "Cannot get user model";
    }
}

void ServerTableModel::onChannelJoined(Channel *channel, const QString &userName, unsigned int session)
{
    qDebug() << "onChannelJoined" << channel << userName << session;
    const auto type = RoomsModel::channelType(channel);
    switch (type) {
    case RoomsModel::ChannelType::Room: {
        auto name = userName.isEmpty() ? _username : userName;
        _roomsModel->insertUser(channel, name, session);
    }
        break;
    case RoomsModel::ChannelType::Class:
        if (!_classNameList.contains(channel->qsName)) {
            _classNameList << channel->qsName;
            _classNameList.sort();
            emit classNameListChanged();
        } else {
            qDebug() << "Class already exists" << channel->qsName;
        }
        break;
    case RoomsModel::ChannelType::School:
        if (!_schoolNameList.contains(channel->qsName)) {
            _schoolNameList << channel->qsName;
            _schoolNameList.sort();
            emit schoolNameListChanged();
        } else {
            qDebug() << "School already exists" << channel->qsName;
        }
        break;
    default:
        qWarning() << "Unknown channel type" << static_cast<int>(type);
    }
    if (RoomsModel::ChannelType::Room != type) {
       onUserDisconnected(userName);
    }
}

void ServerTableModel::onChannelAllowedChanged(int id, bool allowed)
{
    if ((0 < _currentChannelId) && (_currentChannelId == id)) {
        qInfo() << "Got permissions for" << id << allowed;
        _currentChannelId = -1;
        emit currentChannelAllowedChanged(allowed);
    }
}

bool ServerTableModel::gotoSchool(int index)
{
    bool rc = false;
    if ((0 <= index) && (index < _schoolModelItems.size())) {
        const auto *rootItem = _schoolModelItems.at(index);
        if (nullptr != rootItem) {
            _channelActionIndex = index;
            auto *ch =rootItem->cChan;
            isAllowed(ch);
            rc = true;
        } else {
            qWarning() << "Root item is NULL";
        }
    } else {
        qCritical() << "Invalid index" << index;
    }
    return rc;
}

bool ServerTableModel::gotoClass(int index)
{
    bool rc = false;
    if ((0 <= index) && (index < _classModelItems.size())) {
        const auto *rootItem = _classModelItems.at(index);
        if (nullptr != rootItem) {
            _channelActionIndex = index;
            auto *ch =rootItem->cChan;
            isAllowed(ch);
            rc = true;
        } else {
            qWarning() << "Root item is NULL";
        }
    } else {
        qCritical() << "Invalid index" << index;
    }
    return rc;
}

bool ServerTableModel::joinRoom(int index, const QString &username)
{
    qDebug() << "Join room" << index << username;
    auto *ch = _roomsModel->channel(index);
    bool rc = false;
    if (nullptr != ch) {
        _channelActionIndex = index;
        const auto session = _roomsModel->userSession(username);
        g.sh->joinChannel(session, ch->iId);//make sure the error message is shown
        isAllowed(ch);
        rc = true;
        qInfo() << "Connected class" << index;
    } else {
        qCritical() << "Cannot join room: invalid index" << index;
    }
    return rc;
}

void ServerTableModel::isAllowed(Channel *ch)
{
    if (nullptr != ch) {
        ChanACL::Permissions p = static_cast<ChanACL::Permissions>(ch->uiPermissions);
        if (p) {
            qInfo() << "Got channel permissions" << ch->iId;
            const bool allowed = p & (ChanACL::Write | ChanACL::Enter);
            emit currentChannelAllowedChanged(allowed);
        } else {
            qInfo() << "Request channel permissions" << ch->iId;
            _currentChannelId = ch->iId;
            g.sh->requestChannelPermissions(ch->iId);
            if (ch->iId == 0) {
                p = g.pPermissions;
            } else {
                p = ChanACL::All;
            }
            ch->uiPermissions = p;
        }
    }
}

bool ServerTableModel::gotoSchoolInternal()
{
    bool rc = false;
    if ((0 <= _channelActionIndex) && (_channelActionIndex < _schoolModelItems.size())) {
        const auto *rootItem = _schoolModelItems.at(_channelActionIndex);
        if (nullptr != rootItem) {
            _classModelItems.clear();
            _classNameList.clear();
            for (auto *child: rootItem->qlChildren) {
                const auto type = RoomsModel::channelType(child->cChan);
                if (RoomsModel::ChannelType::Class == type) {
                    _classModelItems << child;
                    _classNameList << child->cChan->qsName;
                } else {
                    qWarning() << "Unknown channel type" << static_cast<int>(type);
                }
            }
            emit classNameListChanged();
            setCurrentSchoolName(_schoolNameList.at(_channelActionIndex));
            rc = true;
        } else {
            qWarning() << "Root item is NULL";
        }
    } else {
        qCritical() << "Invalid index" << _channelActionIndex;
    }
    _channelActionIndex = -1;
    return rc;
}

bool ServerTableModel::gotoClassInternal()
{
    bool rc = false;
    if ((0 <= _channelActionIndex) && (_channelActionIndex < _classModelItems.size())) {
        const auto *rootItem = _classModelItems.at(_channelActionIndex);
        if (nullptr != rootItem) {
            _roomsModel->clear();
            for (auto *child: rootItem->qlChildren) {
                const auto type = RoomsModel::channelType(child->cChan);
                if (RoomsModel::ChannelType::Room == type) {
                    RoomsModel::RoomInfo roomInfo;
                    roomInfo.channel = child->cChan;
                    roomInfo.name = child->cChan->qsName;
                    QHash<QString, unsigned int> sessions;
                    for (auto *user: qAsConst(child->qlChildren)) {
                        if ((nullptr != user) && (nullptr != user->pUser)) {
                            const auto username = user->pUser->qsName;
                            roomInfo.users << username;
                            sessions[username] = user->pUser->uiSession;
                        }
                    }
                    _roomsModel->append(roomInfo, sessions);
                } else {
                    qWarning() << "Unknown channel type" << static_cast<int>(type);
                }
            }
            setCurrentClassName(_classNameList.at(_channelActionIndex));
            rc = true;
        } else {
            qWarning() << "Root item is NULL";
        }
    } else {
        qCritical() << "Invalid index" << _channelActionIndex;
    }
    _channelActionIndex = -1;
    return rc;
}

bool ServerTableModel::joinRoomInternal()
{
    qDebug() << "Join room" << _channelActionIndex;
    bool rc = false;
    auto *ch = _roomsModel->channel(_channelActionIndex);
    if (nullptr != ch) {
        _roomsModel->updateRooms(_channelActionIndex);
        rc = true;
        qInfo() << "Connected room" << _channelActionIndex;
    } else {
        qCritical() << "Cannot join room: invalid index" << _channelActionIndex;
    }
    _channelActionIndex = -1;
    return rc;
}

void ServerTableModel::onUserDisconnected(const QString &username)
{
    if (!username.isEmpty()) {
        _roomsModel->removeUser(username);
    }
}
