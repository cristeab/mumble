#include "MainWindow.h"
#include "ServerTableModel.h"
#include "Database.h"
#include "Global.h"
#include "ServerResolver.h"
#include "ServerHandler.h"

#include <QRandomGenerator>
#include <QUdpSocket>
#include <QtEndian>
#include <QThread>

ServerTableModel::ServerTableModel(QObject *parent) : QAbstractTableModel(parent)
{
    setObjectName("servers");

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
        setLabel(srv.name);
    } else {
        setHostname("");
        setPort(DEFAULT_PORT);
        setUsername("");
        setLabel("");
    }
}

void ServerTableModel::changeServer()
{
    emit layoutAboutToBeChanged();
    if (isValidIndex(_currentIndex)) {
        stopDns(&_servers[_currentIndex]);
        auto &server = _servers[_currentIndex];
        server.hostname = _hostname;
        server.port = _port;
        server.username = _username;
        server.name = _label;
        startDns(&_servers[_currentIndex]);
    } else {
        ServerItem server;
        server.hostname = _hostname;
        server.port = _port;
        server.username = _username;
        server.name = _label;
        _servers.append(server);
        startDns(&_servers.last());
    }
    emit layoutChanged();
    save();
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
    for (const auto &it: items) {
        ServerItem srvItem;
        srvItem.name = it.qsName;
        srvItem.address = it.qsUrl;
        srvItem.port = it.usPort;
        srvItem.username = it.qsUsername;
        _servers << srvItem;
        startDns(&_servers.last());
    }
    emit layoutChanged();
}

void ServerTableModel::save()
{
    if (nullptr == g.db) {
        qCritical() << "Cannot access db";
        return;
    }
    QList<FavoriteServer> favs;
    for (const auto &it: _servers) {
        FavoriteServer favSrv;
        favSrv.qsName = it.name;
        favSrv.qsUrl = it.address;
        favSrv.usPort = it.port;
        favSrv.qsUsername = it.username;
        favs << favSrv;
    }
    g.db->setFavorites(favs);
}

void ServerTableModel::startDns(ServerItem *si)
{
    if (!_allowHostLookup) {
        return;
    }

    QString hostname    = si->hostname.toLower();
    unsigned short port = si->port;
    UnresolvedServerAddress unresolved(hostname, port);

    if (si->address.isEmpty()) {
        // Determine if qsHostname is an IP address
        // or a hostname. If it is an IP address, we
        // can treat it as resolved as-is.
        QHostAddress qha(si->hostname);
        bool hostnameIsIPAddress = !qha.isNull();
        if (hostnameIsIPAddress) {
            si->address = si->hostname;
        }
    }

    if (!si->address.isEmpty()) {
        ServerAddress addr(QHostAddress(si->address), si->port);
        _pings[addr].insert(si);
        return;
    }
#ifdef USE_ZEROCONF
    if (_allowZeroconf && si->hostname.isEmpty() && !si->zeroconfRecord.serviceName.isEmpty()) {
        if (!qlBonjourActive.contains(si->zeroconfRecord)) {
            g.zeroconf->startResolver(si->zeroconfRecord);
            qlBonjourActive.append(si->zeroconfRecord);
        }
        return;
    }
#endif
    if (!_dnsWait.contains(unresolved)) {
        _dnsLookup.prepend(unresolved);
    }
    _dnsWait[unresolved].insert(si);
}

void ServerTableModel::stopDns(ServerItem *si)
{
    if (!_allowHostLookup) {
        return;
    }

    ServerAddress addr(QHostAddress(si->address), si->port);
    if (_pings.contains(addr)) {
        _pings[addr].remove(si);
        if (_pings[addr].isEmpty()) {
            _pings.remove(addr);
            _pingRand.remove(addr);
        }
    }

    QString hostname    = si->hostname.toLower();
    unsigned short port = si->port;
    UnresolvedServerAddress unresolved(hostname, port);

    if (_dnsWait.contains(unresolved)) {
        _dnsWait[unresolved].remove(si);
        if (_dnsWait[unresolved].isEmpty()) {
            _dnsWait.remove(unresolved);
            _dnsLookup.removeAll(unresolved);
        }
    }
}

void ServerTableModel::timeTick()
{
    if (_allowHostLookup) {
        // Start DNS Lookup of first unknown hostname
        foreach (const UnresolvedServerAddress &unresolved, _dnsLookup) {
            if (_dnsActive.contains(unresolved)) {
                continue;
            }

            _dnsLookup.removeAll(unresolved);
            _dnsLookup.append(unresolved);

            _dnsActive.insert(unresolved);
            ServerResolver *sr = new ServerResolver();
            QObject::connect(sr, &ServerResolver::resolved, this, &ServerTableModel::lookedUp);
            sr->resolve(unresolved.hostname, unresolved.port);
            break;
        }
    }

    ServerItem *si = nullptr;
    if ((_currentTimer.elapsed() >= TICK_THRESHOLD_US) && isValidIndex(_currentIndex)) {
        si = &_servers[_currentIndex];
    }

    if (si) {
        QString hostname    = si->hostname.toLower();
        unsigned short port = si->port;
        UnresolvedServerAddress unresolved(hostname, port);

        if (si->address.isEmpty()) {
            if (!hostname.isEmpty()) {
                _dnsLookup.removeAll(unresolved);
                _dnsLookup.prepend(unresolved);
            }
            si = nullptr;
        }
    }

    for (const auto &it: _servers) {
        sendPing(QHostAddress(it.address), it.port);
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

                quint64 elapsed = _pingTimer.elapsed() - (*ts ^ _pingRand.value(address));

                for (auto *si: _pings.value(address)) {
                    ++si->recv;
                    si->version    = qFromBigEndian(ping[0]);
                    quint32 users    = qFromBigEndian(ping[3]);
                    quint32 maxusers = qFromBigEndian(ping[4]);
                    si->bandwidth  = qFromBigEndian(ping[5]);

                    if (!si->pingSort)
                        si->pingSort = _pingCache.value(UnresolvedServerAddress(si->hostname, si->port));
                    setStats(si, static_cast< double >(elapsed), users, maxusers);
                }
            }
        }
    }
}

void ServerTableModel::lookedUp()
{
    ServerResolver *sr = qobject_cast< ServerResolver * >(QObject::sender());
    sr->deleteLater();

    QString hostname    = sr->hostname().toLower();
    unsigned short port = sr->port();
    UnresolvedServerAddress unresolved(hostname, port);

    _dnsActive.remove(unresolved);

    // An error occurred, or no records were found.
    if (sr->records().size() == 0) {
        return;
    }

    QSet< ServerAddress > qs;
    foreach (ServerResolverRecord record, sr->records()) {
        foreach (const HostAddress &ha, record.addresses()) { qs.insert(ServerAddress(ha, record.port())); }
    }

    _dnsLookup.removeAll(unresolved);
    _dnsCache.insert(unresolved, qs.values());
    _dnsWait.remove(unresolved);

    for (const auto &addr: qs) {
        sendPing(addr.host.toAddress(), addr.port);
    }
}

void ServerTableModel::setStats(ServerItem *si, double delay, int users, int totalUsers)
{
    emit layoutAboutToBeChanged();
    si->delayMs = delay;
    si->currentUsers = users;
    si->totalUsers = totalUsers;
    emit layoutChanged();
    emit refreshRowChanged();
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

    if (nullptr != g.mw) {
        auto *m = g.mw->pmModel;
        if (nullptr != m) {
            qInfo() << "Got User model";
        } else {
            qWarning() << "User model not available";
        }
    } else {
        qWarning() << "MW";
    }

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
    return true;
}

void ServerTableModel::onLineEditDlgAccepted()
{
    if (_dlgIsPassword) {
        setPassword(_dlgText);
    } else {
        setUsername(_dlgText);
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
