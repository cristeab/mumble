// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "MainWindow.h"

#include "About.h"
#include "ACL.h"
#include "AudioInput.h"
#include "CELTCodec.h"
#include "OpusCodec.h"
#include "Cert.h"
#include "Channel.h"
#include "Connection.h"
#include "Database.h"
#include "DeveloperConsole.h"
#include "Log.h"
#include "Net.h"
#include "OverlayClient.h"
#include "Plugins.h"
#include "ServerHandler.h"
#include "User.h"
#include "UserModel.h"
#include "VersionCheck.h"
#include "ViewCert.h"
#include "../SignalCurry.h"
#include "Settings.h"
#include "SSLCipherInfo.h"
#include "SvgIcon.h"

#ifdef Q_OS_WIN
#include "TaskList.h"
#endif

#ifdef Q_OS_MAC
#include "AppNap.h"
#endif

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

MessageBoxEvent::MessageBoxEvent(QString m) : QEvent(static_cast<QEvent::Type>(MB_QEVENT)) {
	msg = m;
}

OpenURLEvent::OpenURLEvent(QUrl u) : QEvent(static_cast<QEvent::Type>(OU_QEVENT)) {
	url = u;
}

MainWindow::MainWindow(QWidget *p) : QMainWindow(p) {
#ifdef Q_OS_MAC
	if (QFile::exists(QLatin1String("skin:mumble.icns")))
		qiIcon.addFile(QLatin1String("skin:mumble.icns"));
	else
        SvgIcon::addSvgPixmapsToIcon(qiIcon, QLatin1String("skin:bubbles.svg"));
#else
	{
        SvgIcon::addSvgPixmapsToIcon(qiIcon, QLatin1String(":/bubbles.svg"));
	}

	// Set application icon except on MacOSX, where the window-icon
	// shown in the title-bar usually serves as a draggable version of the
	// current open document (i.e. you can copy the open document anywhere
	// simply by dragging this icon).
	qApp->setWindowIcon(qiIcon);
	
	// Set the icon on the MainWindow directly. This fixes the icon not
	// being set on the MainWindow in certain environments (Ex: GTK+).
	setWindowIcon(qiIcon);
#endif

#ifdef Q_OS_WIN
	uiNewHardware = 1;
#endif
	bSuppressAskOnQuit = false;
	restartOnQuit = false;
	bAutoUnmute = false;

	Channel::add(0, tr("Root"));

#if QT_VERSION < 0x050000
	cuContextUser = QWeakPointer<ClientUser>();
	cContextChannel = QWeakPointer<Channel>();
#endif

	qtReconnect = new QTimer(this);
	qtReconnect->setInterval(10000);
	qtReconnect->setSingleShot(true);
	qtReconnect->setObjectName(QLatin1String("Reconnect"));

	qmUser = new QMenu(tr("&User"), this);
	qmChannel = new QMenu(tr("&Channel"), this);

	qmDeveloper = new QMenu(tr("&Developer"), this);

	// Explicitely add actions to mainwindow so their shortcuts are available
	// if only the main window is visible (e.g. minimal mode)
    /*addActions(findChildren<QAction*>());

	qmDeveloper->addAction(qaDeveloperConsole);

	setOnTop(g.s.aotbAlwaysOnTop == Settings::OnTopAlways ||
	         (g.s.bMinimalView && g.s.aotbAlwaysOnTop == Settings::OnTopInMinimal) ||
             (!g.s.bMinimalView && g.s.aotbAlwaysOnTop == Settings::OnTopInNormal));*/

#ifdef NO_UPDATE_CHECK
	delete qaHelpVersionCheck;
#endif
}

void MainWindow::updateWindowTitle() {
	QString title;
	if (g.s.bMinimalView) {
        title = tr("Bubbles - Minimal View -- %1");
	} else {
        title = tr("Bubbles -- %1");
	}
	setWindowTitle(title.arg(QLatin1String(MUMBLE_RELEASE)));
}

// Sets whether or not to show the title bars on the MainWindow's
// dock widgets.
void MainWindow::setShowDockTitleBars(bool doShow) {
	dtbLogDockTitle->setEnabled(doShow);
	dtbChatDockTitle->setEnabled(doShow);
}

MainWindow::~MainWindow() {
	delete pmModel;
	delete Channel::get(0);
}

void MainWindow::msgBox(QString msg) {
	MessageBoxEvent *mbe=new MessageBoxEvent(msg);
	QApplication::postEvent(this, mbe);
}

#ifdef Q_OS_WIN
#if QT_VERSION >= 0x050000
bool MainWindow::nativeEvent(const QByteArray &, void *message, long *) {
	MSG *msg = reinterpret_cast<MSG *>(message);
#else
bool MainWindow::winEvent(MSG *msg, long *) {
#endif
	if (msg->message == WM_DEVICECHANGE && msg->wParam == DBT_DEVNODES_CHANGED)
		uiNewHardware++;

	return false;
}
#endif

void MainWindow::closeEvent(QCloseEvent *e) {
#ifndef Q_OS_MAC
	ServerHandlerPtr sh = g.sh;
	if (sh && sh->isRunning() && g.s.bAskOnQuit && !bSuppressAskOnQuit) {
        QMessageBox mb(QMessageBox::Warning, QLatin1String("Bubbles"), tr("Bubbles is currently connected to a server. Do you want to Close or Minimize it?"), QMessageBox::NoButton, this);
		QPushButton *qpbClose = mb.addButton(tr("Close"), QMessageBox::YesRole);
		QPushButton *qpbMinimize = mb.addButton(tr("Minimize"), QMessageBox::NoRole);
		QPushButton *qpbCancel = mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
		mb.setDefaultButton(qpbClose);
		mb.setEscapeButton(qpbCancel);
		mb.exec();
		if (mb.clickedButton() == qpbMinimize) {
            //showMinimized();
			e->ignore();
			return;
		} else if (mb.clickedButton() == qpbCancel) {
			e->ignore();
			return;
		}
	}
	sh.reset();
#endif
	g.uiSession = 0;
	g.pPermissions = ChanACL::None;

	if (g.s.bMinimalView) {
		g.s.qbaMinimalViewGeometry = saveGeometry();
		g.s.qbaMinimalViewState = saveState();
	} else {
		g.s.qbaMainWindowGeometry = saveGeometry();
		g.s.qbaMainWindowState = saveState();
	}

	g.bQuit = true;

	QMainWindow::closeEvent(e);
	
	qApp->exit(restartOnQuit ? MUMBLE_EXIT_CODE_RESTART : 0);
}

void MainWindow::hideEvent(QHideEvent *e) {
	if (g.ocIntercept) {
		QMetaObject::invokeMethod(g.ocIntercept, "hideGui", Qt::QueuedConnection);
		e->ignore();
		return;
	}
#ifndef Q_OS_MAC
#ifdef Q_OS_UNIX
	if (! qApp->activeModalWidget() && ! qApp->activePopupWidget())
#endif
		if (g.s.bHideInTray && qstiIcon->isSystemTrayAvailable() && e->spontaneous())
			QMetaObject::invokeMethod(this, "hide", Qt::QueuedConnection);
	QMainWindow::hideEvent(e);
#endif
}

void MainWindow::showEvent(QShowEvent *e) {
#ifndef Q_OS_MAC
#ifdef Q_OS_UNIX
	if (! qApp->activeModalWidget() && ! qApp->activePopupWidget())
#endif
		if (g.s.bHideInTray && qstiIcon->isSystemTrayAvailable() && e->spontaneous())
			QMetaObject::invokeMethod(this, "show", Qt::QueuedConnection);
#endif
	QMainWindow::showEvent(e);
}

void MainWindow::changeEvent(QEvent *e) {
	QWidget::changeEvent(e);

#ifdef Q_OS_MAC
	// On modern macOS/Qt combinations, the code below causes Mumble's
	// MainWindow to not be interactive after returning from being minimized.
	// (See issue mumble-voip/mumble#2171)
	// So, let's not do it on macOS.

#else
	if (isMinimized() && qstiIcon->isSystemTrayAvailable() && g.s.bHideInTray) {
		// Workaround http://qt-project.org/forums/viewthread/4423/P15/#50676
		QTimer::singleShot(0, this, SLOT(hide()));
	}
#endif
}

void MainWindow::keyPressEvent(QKeyEvent *e) {
	// Pressing F6 switches between the main
	// window's main widgets, making it easier
	// to navigate Mumble's MainWindow with only
	// a keyboard.
	if (e->key() == Qt::Key_F6) {
	} else {
		QMainWindow::keyPressEvent(e);
	}
}

void MainWindow::updateTrayIcon() {
	ClientUser *p=ClientUser::get(g.uiSession);

	if (g.s.bDeaf) {
		qstiIcon->setIcon(qiIconDeafSelf);
	} else if (p && p->bDeaf) {
		qstiIcon->setIcon(qiIconDeafServer);
	} else if (g.s.bMute) {
		qstiIcon->setIcon(qiIconMuteSelf);
	} else if (p && p->bMute) {
		qstiIcon->setIcon(qiIconMuteServer);
	} else if (p && p->bSuppress) {
		qstiIcon->setIcon(qiIconMuteSuppressed);
	} else if (g.s.bStateInTray && g.bPushToMute) {
		qstiIcon->setIcon(qiIconMutePushToMute);
	} else if (p && g.s.bStateInTray) {
		switch (p->tsState) {
			case Settings::Talking:
				qstiIcon->setIcon(qiTalkingOn);
				break;
			case Settings::Whispering:
				qstiIcon->setIcon(qiTalkingWhisper);
				break;
			case Settings::Shouting:
				qstiIcon->setIcon(qiTalkingShout);
				break;
			case Settings::Passive:
			default:
				qstiIcon->setIcon(qiTalkingOff);
				break;
		}
	} else {
		qstiIcon->setIcon(qiIcon);
	}
}

void MainWindow::updateTransmitModeComboBox() {
	switch (g.s.atTransmit) {
		case Settings::Continuous:
			qcbTransmitMode->setCurrentIndex(0);
			return;
		case Settings::VAD:
			qcbTransmitMode->setCurrentIndex(1);
			return;
		case Settings::PushToTalk:
			qcbTransmitMode->setCurrentIndex(2);
			return;
	}
}

QMenu *MainWindow::createPopupMenu() {
	if ((g.s.wlWindowLayout == Settings::LayoutCustom) && !g.s.bLockLayout) {
		return QMainWindow::createPopupMenu();
	}

	return NULL;
}

Channel *MainWindow::getContextMenuChannel() {
	if (cContextChannel)
		return cContextChannel.data();

	return NULL;
}

ClientUser *MainWindow::getContextMenuUser() {
	if (cuContextUser)
		return cuContextUser.data();

	return NULL;
}

bool MainWindow::handleSpecialContextMenu(const QUrl &url, const QPoint &pos_, bool focus) {
	if (url.scheme() == QString::fromLatin1("clientid")) {
		bool ok = false;
		QString maybeUserHash(url.host());
		if (maybeUserHash.length() == 40) {
			ClientUser *cu = pmModel->getUser(maybeUserHash);
			if (cu) {
				cuContextUser = cu;
				ok = true;
			}
		} else {
			QByteArray qbaServerDigest = QByteArray::fromBase64(url.path().remove(0, 1).toLatin1());
			cuContextUser = ClientUser::get(url.host().toInt(&ok, 10));
			ServerHandlerPtr sh = g.sh;
			ok = ok && sh && (qbaServerDigest == sh->qbaDigest);
		}
		if (ok && cuContextUser) {
			if (focus) {
			} else {
				qpContextPosition = QPoint();
				qmUser->exec(pos_, NULL);
			}
		}
		cuContextUser.clear();
	} else if (url.scheme() == QString::fromLatin1("channelid")) {
		bool ok;
		QByteArray qbaServerDigest = QByteArray::fromBase64(url.path().remove(0, 1).toLatin1());
		cContextChannel = Channel::get(url.host().toInt(&ok, 10));
		ServerHandlerPtr sh = g.sh;
		ok = ok && sh && (qbaServerDigest == sh->qbaDigest);
		if (ok) {
			if (focus) {
			} else {
				qpContextPosition = QPoint();
				qmChannel->exec(pos_, NULL);
			}
		}
		cContextChannel.clear();
	} else {
		return false;
	}
	return true;
}

QString MainWindow::getImagePath(QString filename) const {
	if (g.s.qsImagePath.isEmpty() || ! QDir(g.s.qsImagePath).exists()) {
#if QT_VERSION >= 0x050000
		g.s.qsImagePath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
#else
		g.s.qsImagePath = QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
#endif
	}
	if (filename.isEmpty()) {
		return g.s.qsImagePath;
	}
	return g.s.qsImagePath + QDir::separator() + filename;
}

void MainWindow::updateImagePath(QString filepath) const {
	QFileInfo fi(filepath);
	g.s.qsImagePath = fi.absolutePath();
}

static void recreateServerHandler() {
	ServerHandlerPtr sh = g.sh;
	if (sh && sh->isRunning()) {
		g.mw->on_qaServerDisconnect_triggered();
		sh->disconnect();
		sh->wait();
		QCoreApplication::instance()->processEvents();
	}

	g.sh.reset();
	while (sh && ! sh.unique())
		QThread::yieldCurrentThread();
	sh.reset();

	sh = ServerHandlerPtr(new ServerHandler());
	sh->moveToThread(sh.get());
	g.sh = sh;
	g.mw->connect(sh.get(), SIGNAL(connected()), g.mw, SLOT(serverConnected()));
	g.mw->connect(sh.get(), SIGNAL(disconnected(QAbstractSocket::SocketError, QString)), g.mw, SLOT(serverDisconnected(QAbstractSocket::SocketError, QString)));
	g.mw->connect(sh.get(), SIGNAL(error(QAbstractSocket::SocketError, QString)), g.mw, SLOT(resolverError(QAbstractSocket::SocketError, QString)));
}

void MainWindow::openUrl(const QUrl &url) {
	g.l->log(Log::Information, tr("Opening URL %1").arg(Qt::escape(url.toString())));
	if (url.scheme() == QLatin1String("file")) {
		QFile f(url.toLocalFile());
		if (! f.exists() || ! f.open(QIODevice::ReadOnly)) {
			g.l->log(Log::Warning, tr("File does not exist"));
			return;
		}
		f.close();

		QSettings *qs = new QSettings(f.fileName(), QSettings::IniFormat);
		qs->setIniCodec("UTF-8");
		if (qs->status() != QSettings::NoError) {
			g.l->log(Log::Warning, tr("File is not a configuration file."));
		} else {
			qSwap(qs, g.qs);
			g.s.load();
			qSwap(qs, g.qs);

			g.l->log(Log::Warning, tr("Settings merged from file."));
		}
		delete qs;
		return;
	}
	if (url.scheme() != QLatin1String("mumble")) {
        g.l->log(Log::Warning, tr("URL scheme is not 'mumble'"));
		return;
	}

	int major, minor, patch;
	int thismajor, thisminor, thispatch;
	MumbleVersion::get(&thismajor, &thisminor, &thispatch);

	// With no version parameter given assume the link refers to our version
	major = 1;
	minor = 2;
	patch = 0;

#if QT_VERSION >= 0x050000
	QUrlQuery query(url);
	QString version = query.queryItemValue(QLatin1String("version"));
#else
	QString version = url.queryItemValue(QLatin1String("version"));
#endif
	MumbleVersion::get(&major, &minor, &patch, version);

	if ((major < 1) || // No pre 1.2.0
	        (major == 1 && minor <= 1) ||
	        (major > thismajor) || // No future version
	        (major == thismajor && minor > thisminor) ||
	        (major == thismajor && minor == thisminor && patch > thispatch)) {
		g.l->log(Log::Warning, tr("This version of Mumble can't handle URLs for Mumble version %1.%2.%3").arg(major).arg(minor).arg(patch));
		return;
	}

	QString host = url.host();
	unsigned short port = static_cast<unsigned short>(url.port(DEFAULT_MUMBLE_PORT));
	QString user = url.userName();
	QString pw = url.password();
	qsDesiredChannel = url.path();
	QString name;

#if QT_VERSION >= 0x050000
	if (query.hasQueryItem(QLatin1String("title")))
		name = query.queryItemValue(QLatin1String("title"));
#else
	if (url.hasQueryItem(QLatin1String("title")))
		name = url.queryItemValue(QLatin1String("title"));
#endif

	if (g.sh && g.sh->isRunning()) {
		QString oHost, oUser, oPw;
		unsigned short oport;
		g.sh->getConnectionInfo(oHost, oport, oUser, oPw);
		ClientUser *p = ClientUser::get(g.uiSession);

		if ((user.isEmpty() || (p && p->iId >= 0) || (user == oUser)) &&
		        (host.isEmpty() || ((host == oHost) && (port == oport)))) {
			findDesiredChannel();
			return;
		}
	}

	g.db->fuzzyMatch(name, user, pw, host, port);

	if (user.isEmpty()) {
		bool ok;
		user = QInputDialog::getText(this, tr("Connecting to %1").arg(url.toString()), tr("Enter username"), QLineEdit::Normal, g.s.qsUsername, &ok);
		if (! ok || user.isEmpty())
			return;
		g.s.qsUsername = user;
	}

	if (name.isEmpty())
		name = QString::fromLatin1("%1@%2").arg(user).arg(host);

	recreateServerHandler();

	g.s.qsLastServer = name;
	rtLast = MumbleProto::Reject_RejectType_None;
	bRetryServer = true;
	g.l->log(Log::Information, tr("Connecting to server %1.").arg(Log::msgColor(Qt::escape(host), Log::Server)));
	g.sh->setConnectionInfo(host, port, user, pw);
	g.sh->start(QThread::TimeCriticalPriority);
}

/**
 * This function tries to join a desired channel on connect. It gets called
 * directly after server syncronization is completed.
 * @see void MainWindow::msgServerSync(const MumbleProto::ServerSync &msg)
 */
void MainWindow::findDesiredChannel() {
	bool found = false;
	QStringList qlChans = qsDesiredChannel.split(QLatin1String("/"));
	Channel *chan = Channel::get(0);
	QString str = QString();
	while (chan && qlChans.count() > 0) {
		QString elem = qlChans.takeFirst().toLower();
		if (elem.isEmpty())
			continue;
		if (str.isNull())
			str = elem;
		else
			str = str + QLatin1String("/") + elem;
		foreach(Channel *c, chan->qlChannels) {
			if (c->qsName.toLower() == str) {
				str = QString();
				found = true;
				chan = c;
				break;
			}
		}
	}
	if (found) {
		if (chan != ClientUser::get(g.uiSession)->cChannel) {
			g.sh->joinChannel(g.uiSession, chan->iId);
		}
	}
}

void MainWindow::setOnTop(bool top) {
	Qt::WindowFlags wf = windowFlags();
	if (wf.testFlag(Qt::WindowStaysOnTopHint) != top) {
		if (top)
			wf |= Qt::WindowStaysOnTopHint;
		else
			wf &= ~Qt::WindowStaysOnTopHint;
		setWindowFlags(wf);
		show();
	}
}

void MainWindow::on_Reconnect_timeout() {
	if (g.sh->isRunning())
		return;
	g.l->log(Log::Information, tr("Reconnecting."));
	g.sh->start(QThread::TimeCriticalPriority);
}

void MainWindow::on_qaSelfRegister_triggered() {
	ClientUser *p = ClientUser::get(g.uiSession);
	if (!p)
		return;

	QMessageBox::StandardButton result;
	result = QMessageBox::question(this, tr("Register yourself as %1").arg(p->qsName), tr("<p>You are about to register yourself on this server. This action cannot be undone, and your username cannot be changed once this is done. You will forever be known as '%1' on this server.</p><p>Are you sure you want to register yourself?</p>").arg(Qt::escape(p->qsName)), QMessageBox::Yes|QMessageBox::No);

	if (result == QMessageBox::Yes)
		g.sh->registerUser(p->uiSession);
}

void MainWindow::qcbTransmitMode_activated(int index) {
	switch (index) {
		case 0: // Continuous
			g.s.atTransmit = Settings::Continuous;
			g.l->log(Log::Information, tr("Transmit Mode set to Continuous"));
			return;

		case 1: // Voice Activity
			g.s.atTransmit = Settings::VAD;
			g.l->log(Log::Information, tr("Transmit Mode set to Voice Activity"));
			return;

		case 2: // Push-to-Talk
			g.s.atTransmit = Settings::PushToTalk;
			g.l->log(Log::Information, tr("Transmit Mode set to Push-to-Talk"));
			return;
	}
}

void MainWindow::on_qaServerDisconnect_triggered() {
	if (qtReconnect->isActive()) {
		qtReconnect->stop();
	}
	if (g.sh && g.sh->isRunning())
		g.sh->disconnect();
}

static const QString currentCodec() {
	if (g.bOpus)
		return QLatin1String("Opus");

	int v = g.bPreferAlpha ? g.iCodecAlpha : g.iCodecBeta;
	CELTCodec* cc = g.qmCodecs.value(v);
	if (cc)
		return QString::fromLatin1("CELT %1").arg(cc->version());
	else
		return QString::fromLatin1("CELT %1").arg(QString::number(v, 16));
}

void MainWindow::on_qaServerInformation_triggered() {
	ConnectionPtr c = g.sh->cConnection;

	if (! c)
		return;

	CryptState &cs = c->csCrypt;
	QSslCipher qsc = g.sh->qscCipher;

	QString qsVersion=tr("<h2>Version</h2><p>Protocol %1</p>").arg(MumbleVersion::toString(g.sh->uiVersion));

	if (g.sh->qsRelease.isEmpty() || g.sh->qsOS.isEmpty() || g.sh->qsOSVersion.isEmpty()) {
		qsVersion.append(tr("<p>No build information or OS version available</p>"));
	} else {
		qsVersion.append(tr("<p>%1 (%2)<br />%3</p>")
		                 .arg(Qt::escape(g.sh->qsRelease), Qt::escape(g.sh->qsOS), Qt::escape(g.sh->qsOSVersion)));
	}

	QString host, uname, pw;
	unsigned short port;

	g.sh->getConnectionInfo(host,port,uname,pw);

	const SSLCipherInfo *ci = SSLCipherInfoLookupByOpenSSLName(qsc.name().toLatin1().constData());
	
	QString cipherDescription;
	if (ci && ci->message_authentication && ci->encryption && ci->key_exchange_verbose && ci->rfc_name) {
		if (QString::fromLatin1(ci->message_authentication) == QLatin1String("AEAD")) {
			// Authenticated Encryption with Associated Data
			// See https://en.wikipedia.org/wiki/Authenticated_encryption
			cipherDescription = tr(
			    "The connection is encrypted and authenticated "
			    "using %1 and uses %2 as the key exchange mechanism (%3)").arg(
			        QString::fromLatin1(ci->encryption),
			        QString::fromLatin1(ci->key_exchange_verbose),
			        QString::fromLatin1(ci->rfc_name));
		} else {
			cipherDescription = tr(
			    "The connection is encrypted using %1, with %2 "
			    "for message authentication and %3 as the key "
			    "exchange mechanism (%4)").arg(
			        QString::fromLatin1(ci->encryption),
			        QString::fromLatin1(ci->message_authentication),
			        QString::fromLatin1(ci->key_exchange_verbose),
			        QString::fromLatin1(ci->rfc_name));
		}
	}
	if (cipherDescription.isEmpty()) {
		cipherDescription = tr("The connection is secured by the cipher suite that OpenSSL identifies as %1").arg(qsc.name());
	}

	QString cipherPFSInfo;
	if (ci) {
		if (ci->forward_secret) {
			cipherPFSInfo = tr("<p>The connection provides perfect forward secrecy</p>");
		} else {
			cipherPFSInfo = tr("<p>The connection does not provide perfect forward secrecy</p>");
		}
	}

	QString qsControl = tr(
	            "<h2>Control channel</h2>"
	            "<p>The connection uses %1</p>"
	            "%2"
	            "%3"
	            "<p>%4 ms average latency (%5 deviation)</p>"
	            "<p>Remote host %6 (port %7)</p>").arg(
	                  Qt::escape(c->sessionProtocolString()),
	                  cipherDescription,
	                  cipherPFSInfo,
	                  QString::fromLatin1("%1").arg(boost::accumulators::mean(g.sh->accTCP), 0, 'f', 2),
	                  QString::fromLatin1("%1").arg(sqrt(boost::accumulators::variance(g.sh->accTCP)),0,'f',2),
	                  Qt::escape(host),
	                  QString::number(port));
	if (g.uiMaxUsers) {
		qsControl += tr("<p>Connected users: %1/%2</p>").arg(ModelItem::c_qhUsers.count()).arg(g.uiMaxUsers);
	}

	QString qsVoice, qsCrypt, qsAudio;

	qsAudio=tr("<h2>Audio bandwidth</h2><p>Maximum %1 kbit/s<br />Current %2 kbit/s<br />Codec: %3</p>").arg(g.iMaxBandwidth / 1000.0,0,'f',1).arg(g.iAudioBandwidth / 1000.0,0,'f',1).arg(currentCodec());

    QMessageBox qmb(QMessageBox::Information, tr("Bubbles Server Information"), qsVersion + qsControl + qsVoice + qsCrypt + qsAudio, QMessageBox::Ok, this);
	qmb.setDefaultButton(QMessageBox::Ok);
	qmb.setEscapeButton(QMessageBox::Ok);

	QPushButton *qp = qmb.addButton(tr("&View Certificate"), QMessageBox::ActionRole);
	int res = qmb.exec();
	if ((res == 0) && (qmb.clickedButton() == qp)) {
		ViewCert vc(g.sh->qscCert, this);
		vc.exec();
	}
}

void MainWindow::on_qaServerTexture_triggered() {
	QPair<QByteArray, QImage> choice = openImageFile();
	if (choice.first.isEmpty())
		return;

	const QImage &img = choice.second;

	if ((img.height() <= 1024) && (img.width() <= 1024))
		g.sh->setUserTexture(g.uiSession, choice.first);
}

void MainWindow::on_qaServerTextureRemove_triggered() {
	g.sh->setUserTexture(g.uiSession, QByteArray());
}

void MainWindow::on_qaUserMute_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	MumbleProto::UserState mpus;
	mpus.set_session(p->uiSession);
	if (p->bMute || p->bSuppress) {
		if (p->bMute)
			mpus.set_mute(false);
		if (p->bSuppress)
			mpus.set_suppress(false);
	} else {
		mpus.set_mute(true);
	}
	g.sh->sendMessage(mpus);
}

void MainWindow::on_qaUserDeaf_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	MumbleProto::UserState mpus;
	mpus.set_session(p->uiSession);
	mpus.set_deaf(! p->bDeaf);
	g.sh->sendMessage(mpus);
}

void MainWindow::on_qaSelfPrioritySpeaker_triggered() {
	ClientUser *p = ClientUser::get(g.uiSession);
	if (!p)
		return;

	MumbleProto::UserState mpus;
	mpus.set_session(p->uiSession);
	mpus.set_priority_speaker(! p->bPrioritySpeaker);
	g.sh->sendMessage(mpus);
}

void MainWindow::on_qaUserPrioritySpeaker_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	MumbleProto::UserState mpus;
	mpus.set_session(p->uiSession);
	mpus.set_priority_speaker(! p->bPrioritySpeaker);
	g.sh->sendMessage(mpus);
}

void MainWindow::on_qaUserRegister_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	unsigned int session = p->uiSession;

	QMessageBox::StandardButton result;

	if (session == g.uiSession)
		result = QMessageBox::question(this, tr("Register yourself as %1").arg(p->qsName), tr("<p>You are about to register yourself on this server. This action cannot be undone, and your username cannot be changed once this is done. You will forever be known as '%1' on this server.</p><p>Are you sure you want to register yourself?</p>").arg(Qt::escape(p->qsName)), QMessageBox::Yes|QMessageBox::No);
	else
		result = QMessageBox::question(this, tr("Register user %1").arg(p->qsName), tr("<p>You are about to register %1 on the server. This action cannot be undone, the username cannot be changed, and as a registered user, %1 will have access to the server even if you change the server password.</p><p>From this point on, %1 will be authenticated with the certificate currently in use.</p><p>Are you sure you want to register %1?</p>").arg(Qt::escape(p->qsName)), QMessageBox::Yes|QMessageBox::No);

	if (result == QMessageBox::Yes) {
		p = ClientUser::get(session);
		if (! p)
			return;
		g.sh->registerUser(p->uiSession);
	}
}

void MainWindow::on_qaUserFriendAdd_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	g.db->addFriend(p->qsName, p->qsHash);
	pmModel->setFriendName(p, p->qsName);
}

void MainWindow::on_qaUserFriendUpdate_triggered() {
	on_qaUserFriendAdd_triggered();
}

void MainWindow::on_qaUserFriendRemove_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	g.db->removeFriend(p->qsHash);
	pmModel->setFriendName(p, QString());
}

void MainWindow::on_qaUserKick_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	unsigned int session = p->uiSession;

	bool ok;
	QString reason = QInputDialog::getText(this, tr("Kicking user %1").arg(p->qsName), tr("Enter reason"), QLineEdit::Normal, QString(), &ok);

	p = ClientUser::get(session);
	if (!p)
		return;

	if (ok) {
		g.sh->kickBanUser(p->uiSession, reason, false);
	}
}

void MainWindow::on_qaUserBan_triggered() {
	ClientUser *p = getContextMenuUser();
	if (!p)
		return;

	unsigned int session = p->uiSession;

	bool ok;
	QString reason = QInputDialog::getText(this, tr("Banning user %1").arg(p->qsName), tr("Enter reason"), QLineEdit::Normal, QString(), &ok);
	p = ClientUser::get(session);
	if (!p)
		return;

	if (ok) {
		g.sh->kickBanUser(p->uiSession, reason, true);
	}
}

void MainWindow::on_qaUserCommentReset_triggered() {
	ClientUser *p = getContextMenuUser();

	if (!p)
		return;

	unsigned int session = p->uiSession;

    int ret = QMessageBox::question(this, QLatin1String("Bubbles"),
	                                tr("Are you sure you want to reset the comment of user %1?").arg(Qt::escape(p->qsName)),
	                                QMessageBox::Yes, QMessageBox::No);
	if (ret == QMessageBox::Yes) {
		g.sh->setUserComment(session, QString());
	}
}

void MainWindow::on_qaUserTextureReset_triggered() {
	ClientUser *p = getContextMenuUser();

	if (!p)
		return;

	unsigned int session = p->uiSession;

    int ret = QMessageBox::question(this, QLatin1String("Bubbles"),
	                                tr("Are you sure you want to reset the avatar of user %1?").arg(Qt::escape(p->qsName)),
	                                QMessageBox::Yes, QMessageBox::No);
	if (ret == QMessageBox::Yes) {
		g.sh->setUserTexture(session, QByteArray());
	}
}

void MainWindow::on_qaUserInformation_triggered() {
	ClientUser *p = getContextMenuUser();

	if (!p)
		return;

	g.sh->requestUserStats(p->uiSession, false);
}

void MainWindow::on_qaHide_triggered() {
	hide();
}

void MainWindow::on_qaQuit_triggered() {
	bSuppressAskOnQuit = true;
	this->close();
}

/// Handles Backtab/Shift-Tab for qteChat, which allows
/// users to move focus to the previous widget in
/// MainWindow.
void MainWindow::on_qteChat_backtabPressed() {
	focusPreviousChild();
}

void MainWindow::on_qaChannelJoin_triggered() {
	Channel *c = getContextMenuChannel();

	if (c) {
		g.sh->joinChannel(g.uiSession, c->iId);
	}
}

void MainWindow::on_qaChannelRemove_triggered() {
	int ret;
	Channel *c = getContextMenuChannel();
	if (! c)
		return;

	int id = c->iId;

    ret=QMessageBox::question(this, QLatin1String("Bubbles"), tr("Are you sure you want to delete %1 and all its sub-channels?").arg(Qt::escape(c->qsName)), QMessageBox::Yes, QMessageBox::No);

	c = Channel::get(id);
	if (!c)
		return;

	if (ret == QMessageBox::Yes) {
		g.sh->removeChannel(c->iId);
	}
}

void MainWindow::on_qaChannelACL_triggered() {
	Channel *c = getContextMenuChannel();
	if (! c)
		c = Channel::get(0);
	int id = c->iId;

	if (! c->qbaDescHash.isEmpty() && c->qsDesc.isEmpty()) {
		c->qsDesc = QString::fromUtf8(g.db->blob(c->qbaDescHash));
		if (c->qsDesc.isEmpty()) {
			MumbleProto::RequestBlob mprb;
			mprb.add_channel_description(id);
			g.sh->sendMessage(mprb);
		}
	}

	g.sh->requestACL(id);
}

void MainWindow::on_qaChannelLink_triggered() {
	Channel *c = ClientUser::get(g.uiSession)->cChannel;
	Channel *l = getContextMenuChannel();
	if (! l)
		l = Channel::get(0);

	g.sh->addChannelLink(c->iId, l->iId);
}

void MainWindow::on_qaChannelUnlink_triggered() {
	Channel *c = ClientUser::get(g.uiSession)->cChannel;
	Channel *l = getContextMenuChannel();
	if (! l)
		l = Channel::get(0);

	g.sh->removeChannelLink(c->iId, l->iId);
}

void MainWindow::on_qaChannelUnlinkAll_triggered() {
	Channel *c = ClientUser::get(g.uiSession)->cChannel;

	MumbleProto::ChannelState mpcs;
	mpcs.set_channel_id(c->iId);
	foreach(Channel *l, c->qsPermLinks)
		mpcs.add_links_remove(l->iId);
	g.sh->sendMessage(mpcs);
}

void MainWindow::on_qaChannelCopyURL_triggered() {
	Channel *c = getContextMenuChannel();
	QString host, uname, pw, channel;
	unsigned short port;

	if (!c)
		return;

	g.sh->getConnectionInfo(host, port, uname, pw);
	// walk back up the channel list to build the URL.
	while (c->cParent != NULL) {
		channel.prepend(c->qsName);
		channel.prepend(QLatin1String("/"));
		c = c->cParent;
	}
}

void MainWindow::userStateChanged() {
	if (g.s.bStateInTray) {
		updateTrayIcon();
	}
	
	ClientUser *user = ClientUser::get(g.uiSession);
	if (user == NULL) {
		g.bAttenuateOthers = false;
		g.prioritySpeakerActiveOverride = false;
		
		return;
	}

	switch (user->tsState) {
		case Settings::Talking:
		case Settings::Whispering:
		case Settings::Shouting:
			g.bAttenuateOthers = g.s.bAttenuateOthersOnTalk;

			g.prioritySpeakerActiveOverride =
			        g.s.bAttenuateUsersOnPrioritySpeak
			        && user->bPrioritySpeaker;
			
			break;
		case Settings::Passive:
		default:
			g.bAttenuateOthers = false;
			g.prioritySpeakerActiveOverride = false;
			break;
	}
}

void MainWindow::on_qaAudioReset_triggered() {
	AudioInputPtr ai = g.ai;
	if (ai)
		ai->bResetProcessor = true;
}

void MainWindow::on_qaDeveloperConsole_triggered() {
	g.c->show();
}

void MainWindow::on_qaHelpWhatsThis_triggered() {
	QWhatsThis::enterWhatsThisMode();
}

void MainWindow::on_qaHelpAbout_triggered() {
	AboutDialog adAbout(this);
	adAbout.exec();
}

void MainWindow::on_qaHelpAboutQt_triggered() {
	QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_qaHelpVersionCheck_triggered() {
	new VersionCheck(false, this);
}

void MainWindow::on_PushToTalk_triggered(bool down, QVariant) {
	g.iPrevTarget = 0;
	if (down) {
		g.uiDoublePush = g.tDoublePush.restart();
		g.iPushToTalk++;
	} else if (g.iPushToTalk > 0) {
		QTimer::singleShot(static_cast<int>(g.s.pttHold), this, SLOT(pttReleased()));
	}
}

void MainWindow::pttReleased() {
	if (g.iPushToTalk > 0) {
		g.iPushToTalk--;
	}
}

void MainWindow::on_PushToMute_triggered(bool down, QVariant) {
	g.bPushToMute = down;
	updateTrayIcon();
}

void MainWindow::on_VolumeUp_triggered(bool down, QVariant) {
	if (down) {
		float vol = g.s.fVolume + 0.1f;
		if (vol > 2.0f) {
			vol = 2.0f;
		}
		g.s.fVolume = vol;
	}
}

void MainWindow::on_VolumeDown_triggered(bool down, QVariant) {
	if (down) {
		float vol = g.s.fVolume - 0.1f;
		if (vol < 0.0f) {
			vol = 0.0f;
		}
		g.s.fVolume = vol;
	}
}

void MainWindow::updateTarget() {
	g.iPrevTarget = g.iTarget;

	if (qmCurrentTargets.isEmpty()) {
		g.bCenterPosition = false;
		g.iTarget = 0;
	} else {
		bool center = false;
		QList<ShortcutTarget> ql;
		foreach(const ShortcutTarget &st, qmCurrentTargets.keys()) {
			ShortcutTarget nt;
			center = center || st.bForceCenter;
			nt.bUsers = st.bUsers;
			if (st.bUsers) {
				foreach(const QString &hash, st.qlUsers) {
					ClientUser *p = pmModel->getUser(hash);
					if (p)
						nt.qlSessions.append(p->uiSession);
				}
				if (! nt.qlSessions.isEmpty())
					ql << nt;
			}
		}
		if (ql.isEmpty()) {
			g.iTarget = -1;
		} else {
			++iTargetCounter;

			int idx = qmTargets.value(ql);
			if (idx == 0) {
				QMap<int, int> qm;
				QMap<int, int>::const_iterator i;
				for (i=qmTargetUse.constBegin(); i != qmTargetUse.constEnd(); ++i) {
					qm.insert(i.value(), i.key());
				}

				i = qm.constBegin();
				idx = i.value();



				MumbleProto::VoiceTarget mpvt;
				mpvt.set_id(idx);

				foreach(const ShortcutTarget &st, ql) {
					MumbleProto::VoiceTarget_Target *t = mpvt.add_targets();
					if (st.bUsers) {
						foreach(unsigned int uisession, st.qlSessions)
							t->add_session(uisession);
					} else {
						t->set_channel_id(st.iChannel);
						if (st.bChildren)
							t->set_children(true);
						if (st.bLinks)
							t->set_links(true);
						if (! st.qsGroup.isEmpty())
							t->set_group(u8(st.qsGroup));
					}
				}
				g.sh->sendMessage(mpvt);

				qmTargets.insert(ql, idx);

				++i;
				++i;
				int oldidx = i.value();
				if (oldidx) {
					QHash<QList<ShortcutTarget>, int>::iterator mi;
					for (mi = qmTargets.begin(); mi != qmTargets.end(); ++mi) {
						if (mi.value() == oldidx) {
							qmTargets.erase(mi);

							mpvt.Clear();
							mpvt.set_id(oldidx);
							g.sh->sendMessage(mpvt);

							break;
						}
					}
				}
			}
			qmTargetUse.insert(idx, iTargetCounter);
			g.bCenterPosition = center;
			g.iTarget = idx;
		}
	}
}

/* Add and remove ShortcutTargets from the qmCurrentTargets Map, which counts
 * the number of push-to-talk events for a given ShortcutTarget.  If this number
 * reaches 0, the ShortcutTarget is removed from qmCurrentTargets.
 */
void MainWindow::addTarget(ShortcutTarget *st)
{
	if (qmCurrentTargets.contains(*st))
		qmCurrentTargets[*st] += 1;
	else
		qmCurrentTargets[*st] = 1;
}

void MainWindow::removeTarget(ShortcutTarget *st)
{
	if (!qmCurrentTargets.contains(*st))
		return;

	if (qmCurrentTargets[*st] == 1)
		qmCurrentTargets.remove(*st);
	else
		qmCurrentTargets[*st] -= 1;
}

void MainWindow::on_gsCycleTransmitMode_triggered(bool down, QVariant)
{
	if (down) 
	{
		QString qsNewMode;

		switch (g.s.atTransmit)
		{
			case Settings::Continuous:
				g.s.atTransmit = Settings::VAD;
				g.l->log(Log::Information, tr("Transmit Mode set to Voice Activity"));
				break;
			case Settings::VAD:
				g.s.atTransmit = Settings::PushToTalk;
				g.l->log(Log::Information, tr("Transmit Mode set to Push-to-Talk"));
				break;
			case Settings::PushToTalk:
				g.s.atTransmit = Settings::Continuous;
				g.l->log(Log::Information, tr("Transmit Mode set to Continuous"));
				break;
		}
	}

	updateTransmitModeComboBox();
}

void MainWindow::on_gsTransmitModePushToTalk_triggered(bool down, QVariant)
{
	if (down) {
		g.s.atTransmit = Settings::PushToTalk;
		g.l->log(Log::Information, tr("Transmit Mode set to Push-to-Talk"));
	}

	updateTransmitModeComboBox();
}

void MainWindow::on_gsTransmitModeContinuous_triggered(bool down, QVariant)
{
	if (down) {
		g.s.atTransmit = Settings::Continuous;
		g.l->log(Log::Information, tr("Transmit Mode set to Continuous"));
	}

	updateTransmitModeComboBox();
}

void MainWindow::on_gsTransmitModeVAD_triggered(bool down, QVariant)
{
	if (down) {
		g.s.atTransmit = Settings::VAD;
		g.l->log(Log::Information, tr("Transmit Mode set to Voice Activity"));
	}

	updateTransmitModeComboBox();
}

void MainWindow::on_gsSendTextMessage_triggered(bool down, QVariant scdata) {
	if (!down || !g.sh || !g.sh->isRunning() || g.uiSession == 0) {
		return;
	}

	QString qsText = scdata.toString();
	if (qsText.isEmpty()) {
		return;
	}

	Channel *c = ClientUser::get(g.uiSession)->cChannel;
	g.sh->sendChannelTextMessage(c->iId, qsText, false);
	g.l->log(Log::TextMessage, tr("To %1: %2").arg(Log::formatChannel(c), qsText), tr("Message to channel %1").arg(c->qsName), true);
}

void MainWindow::whisperReleased(QVariant scdata) {
	if (g.iPushToTalk <= 0)
		return;

	ShortcutTarget st = scdata.value<ShortcutTarget>();

	g.iPushToTalk--;

	removeTarget(&st);
	updateTarget();
}

void MainWindow::onResetAudio()
{
	qWarning("MainWindow: Start audio reset");
	Audio::stop();
	Audio::start();
	qWarning("MainWindow: Audio reset complete");
}

void MainWindow::viewCertificate(bool) {
	ViewCert vc(g.sh->qscCert, this);
	vc.exec();
}

/**
 * This function prepares the UI for receiving server data. It gets called once the
 * connection to the server is established but before the server Sync is complete.
 */
void MainWindow::serverConnected() {
	g.uiSession = 0;
	g.pPermissions = ChanACL::None;
	g.iCodecAlpha = 0x8000000b;
	g.bPreferAlpha = true;
#ifdef USE_OPUS
	g.bOpus = true;
#else
	g.bOpus = false;
#endif
	g.iCodecBeta = 0;

#ifdef Q_OS_MAC
	// Suppress AppNap while we're connected to a server.
	MUSuppressAppNap(true);
#endif

	g.l->clearIgnore();
	g.l->setIgnore(Log::UserJoin);
	g.l->setIgnore(Log::OtherSelfMute);
	QString host, uname, pw;
	unsigned short port;
	g.sh->getConnectionInfo(host, port, uname, pw);
	g.l->log(Log::ServerConnected, tr("Connected."));

	Channel *root = Channel::get(0);
	pmModel->renameChannel(root, tr("Root"));
	pmModel->setCommentHash(root, QByteArray());
	root->uiPermissions = 0;

	g.bAllowHTML = true;
	g.uiMessageLength = 5000;
	g.uiImageLength = 131072;
	g.uiMaxUsers = 0;

	if (g.s.bMute || g.s.bDeaf) {
		g.sh->setSelfMuteDeafState(g.s.bMute, g.s.bDeaf);
	}

#ifdef Q_OS_WIN
	TaskList::addToRecentList(g.s.qsLastServer, uname, host, port);
#endif
}

void MainWindow::serverDisconnected(QAbstractSocket::SocketError err, QString reason) {
    qDebug() << "serverDisconnected" << reason;

    g.uiSession = 0;
	g.pPermissions = ChanACL::None;
	g.bAttenuateOthers = false;
	updateTrayIcon();

#ifdef Q_OS_MAC
	// Remove App Nap suppression now that we're disconnected.
	MUSuppressAppNap(false);
#endif

	QString uname, pw, host;
	unsigned short port;
	g.sh->getConnectionInfo(host, port, uname, pw);

	QSet<QAction *> qs;
	qs += qlServerActions.toSet();
	qs += qlChannelActions.toSet();
	qs += qlUserActions.toSet();

	foreach(QAction *a, qs)
		delete a;

	qlServerActions.clear();
	qlChannelActions.clear();
	qlUserActions.clear();

	pmModel->removeAll();

	if (! g.sh->qlErrors.isEmpty()) {
		foreach(QSslError e, g.sh->qlErrors)
			g.l->log(Log::Warning, tr("SSL Verification failed: %1").arg(Qt::escape(e.errorString())));
		if (! g.sh->qscCert.isEmpty()) {
			QSslCertificate c = g.sh->qscCert.at(0);
			QString basereason;
			QString actual_digest = QString::fromLatin1(c.digest(QCryptographicHash::Sha1).toHex());
			QString digests_section = tr("<li>Server certificate digest (SHA-1):\t%1</li>").arg(ViewCert::prettifyDigest(actual_digest));
			QString expected_digest = g.db->getDigest(host, port);
			if (! expected_digest.isNull()) {
				basereason = tr("<b>WARNING:</b> The server presented a certificate that was different from the stored one.");
				digests_section.append(tr("<li>Expected certificate digest (SHA-1):\t%1</li>").arg(ViewCert::prettifyDigest(expected_digest)));
			} else {
				basereason = tr("Server presented a certificate which failed verification.");
			}
			QStringList qsl;
			foreach(QSslError e, g.sh->qlErrors)
				qsl << QString::fromLatin1("<li>%1</li>").arg(Qt::escape(e.errorString()));

            QMessageBox qmb(QMessageBox::Warning, QLatin1String("Bubbles"),
			                tr("<p>%1</p><ul>%2</ul><p>The specific errors with this certificate are:</p><ol>%3</ol>"
			                   "<p>Do you wish to accept this certificate anyway?<br />(It will also be stored so you won't be asked this again.)</p>"
			                  ).arg(basereason).arg(digests_section).arg(qsl.join(QString())), QMessageBox::Yes | QMessageBox::No, this);

			qmb.setDefaultButton(QMessageBox::No);
			qmb.setEscapeButton(QMessageBox::No);

			QPushButton *qp = qmb.addButton(tr("&View Certificate"), QMessageBox::ActionRole);
			forever {
				int res = qmb.exec();

				if ((res == 0) && (qmb.clickedButton() == qp)) {
					ViewCert vc(g.sh->qscCert, this);
					vc.exec();
					continue;
				} else if (res == QMessageBox::Yes) {
					g.db->setDigest(host, port, QString::fromLatin1(c.digest(QCryptographicHash::Sha1).toHex()));
					on_Reconnect_timeout();
				}
				break;
			}
		}
	} else if (err == QAbstractSocket::SslHandshakeFailedError) {
        QMessageBox::warning(this, tr("SSL Version mismatch"), tr("This server is using an older encryption standard, and is no longer supported by modern versions of Bubbles."), QMessageBox::Ok);
	} else {
		bool ok = false;


		if (! reason.isEmpty()) {
			g.l->log(Log::ServerDisconnected, tr("Server connection failed: %1.").arg(Qt::escape(reason)));
		}  else {
			g.l->log(Log::ServerDisconnected, tr("Disconnected from server."));
		}

		Qt::WindowFlags wf = 0;
#ifdef Q_OS_MAC
		wf = Qt::Sheet;
#endif

        emit serverDisconnectedEvent(rtLast, reason);
	}
    qstiIcon->setToolTip(tr("Bubbles -- %1").arg(QLatin1String(MUMBLE_RELEASE)));
	AudioInput::setMaxBandwidth(-1);
}

void MainWindow::resolverError(QAbstractSocket::SocketError, QString reason) {
	if (! reason.isEmpty()) {
		g.l->log(Log::ServerDisconnected, tr("Server connection failed: %1.").arg(Qt::escape(reason)));
	}  else {
		g.l->log(Log::ServerDisconnected, tr("Server connection failed."));
	}

	if (g.s.bReconnect) {
		if (bRetryServer) {
			qtReconnect->start();
		}
	}
}

void MainWindow::trayAboutToShow() {
	bool top = false;

	QPoint p = qstiIcon->geometry().center();
	if (p.isNull()) {
		p = QCursor::pos();
	}

	QDesktopWidget dw;

	if (dw.screenNumber(p) >= 0) {
		QRect qr = dw.screenGeometry(p);

		if (p.y() < (qr.height() / 2))
			top = true;

		qmTray->clear();
	}
}

void MainWindow::customEvent(QEvent *evt) {
	if (evt->type() == MB_QEVENT) {
		MessageBoxEvent *mbe=static_cast<MessageBoxEvent *>(evt);
		g.l->log(Log::Information, mbe->msg);
		return;
	} else if (evt->type() == OU_QEVENT) {
		OpenURLEvent *oue=static_cast<OpenURLEvent *>(evt);
		openUrl(oue->url);
		return;
	} else if (evt->type() != SERVERSEND_EVENT) {
		return;
	}

	ServerHandlerMessageEvent *shme=static_cast<ServerHandlerMessageEvent *>(evt);

#ifdef QT_NO_DEBUG
#define MUMBLE_MH_MSG(x) case MessageHandler:: x : { \
		MumbleProto:: x msg; \
		if (msg.ParseFromArray(shme->qbaMsg.constData(), shme->qbaMsg.size())) \
			msg##x(msg); \
		break; \
	}
#else
#define MUMBLE_MH_MSG(x) case MessageHandler:: x : { \
		MumbleProto:: x msg; \
		if (msg.ParseFromArray(shme->qbaMsg.constData(), shme->qbaMsg.size())) { \
			printf("%s:\n", #x); \
			msg.PrintDebugString(); \
			msg##x(msg); \
		} \
		break; \
	}
#endif
	switch (shme->uiType) {
			MUMBLE_MH_ALL
	}


#undef MUMBLE_MH_MSG
}


void MainWindow::on_qteLog_anchorClicked(const QUrl &url) {
	if (!handleSpecialContextMenu(url, QCursor::pos(), true)) {
#ifdef Q_OS_MAC
		// Clicking a link can cause the user's default browser to pop up while
		// we're intercepting all events. This can be very confusing (because
		// the user can't click on anything before they dismiss the overlay
		// by hitting their toggle hotkey), so let's disallow clicking links
		// when embedded into the overlay for now.
		if (g.ocIntercept)
			return;
#endif
		if (url.scheme() != QLatin1String("file")
		        && url.scheme() != QLatin1String("qrc")
		        && !url.isRelative())
			QDesktopServices::openUrl(url);
	}
}

/**
 * Presents a file open dialog, opens the selected picture and returns it.
 * @return Pair consisting of the raw file contents and the image. Unitialized on error or cancel.
 */
QPair<QByteArray, QImage> MainWindow::openImageFile() {
	QPair<QByteArray, QImage> retval;

	QString fname = QFileDialog::getOpenFileName(this, tr("Choose image file"), getImagePath(), tr("Images (*.png *.jpg *.jpeg)"));

	if (fname.isNull())
		return retval;

	QFile f(fname);
	if (! f.open(QIODevice::ReadOnly)) {
		QMessageBox::warning(this, tr("Failed to load image"), tr("Could not open file for reading."));
		return retval;
	}

	updateImagePath(fname);

	QByteArray qba = f.readAll();
	f.close();

	QBuffer qb(&qba);
	qb.open(QIODevice::ReadOnly);

	QImageReader qir;
	qir.setAutoDetectImageFormat(false);
	qir.setDevice(&qb);

	QImage img = qir.read();
	if (img.isNull()) {
		QMessageBox::warning(this, tr("Failed to load image"), tr("Image format not recognized."));
		return retval;
	}

	retval.first = qba;
	retval.second = img;

	return retval;
}
