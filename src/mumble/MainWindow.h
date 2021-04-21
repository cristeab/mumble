// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_MAINWINDOW_H_
#define MUMBLE_MUMBLE_MAINWINDOW_H_

#include <QtCore/QtGlobal>
#if QT_VERSION >= 0x050000
# include <QtCore/QPointer>
# include <QtWidgets/QMainWindow>
# include <QtWidgets/QSystemTrayIcon>
#else
# include <QtCore/QWeakPointer>
# include <QtGui/QMainWindow>
# include <QtGui/QSystemTrayIcon>
#endif

#include <QtNetwork/QAbstractSocket>

#include "CustomElements.h"
#include "Message.h"
#include "Mumble.pb.h"
#include "MUComboBox.h"
#include "Channel.h"

#define MB_QEVENT (QEvent::User + 939)
#define OU_QEVENT (QEvent::User + 940)

class ServerHandler;
class TextToSpeech;
class UserModel;

struct ShortcutTarget;

class MessageBoxEvent : public QEvent {
	public:
		QString msg;
		MessageBoxEvent(QString msg);
};

class OpenURLEvent : public QEvent {
	public:
		QUrl url;
		OpenURLEvent(QUrl url);
};

class MainWindow : public QMainWindow, public MessageHandler {
		friend class UserModel;
        friend class ServerTableModel;
	private:
		Q_OBJECT
		Q_DISABLE_COPY(MainWindow)
	public:
		UserModel *pmModel;
		QSystemTrayIcon *qstiIcon;
		QMenu *qmUser;
		QMenu *qmChannel;
		QMenu *qmDeveloper;
		QMenu *qmTray;
		QIcon qiIcon, qiIconMutePushToMute, qiIconMuteSelf, qiIconMuteServer, qiIconDeafSelf, qiIconDeafServer, qiIconMuteSuppressed;
		QIcon qiTalkingOn, qiTalkingWhisper, qiTalkingShout, qiTalkingOff;

		DockTitleBar *dtbLogDockTitle, *dtbChatDockTitle;

		MumbleProto::Reject_RejectType rtLast;
		bool bRetryServer;
		QString qsDesiredChannel;

		bool bSuppressAskOnQuit;
		/// Restart the client after shutdown
		bool restartOnQuit;
		bool bAutoUnmute;

		/// Contains the cursor whose position is immediately before the image to
		/// save when activating the "Save Image As..." context menu item.
		QTextCursor qtcSaveImageCursor;

#if QT_VERSION >= 0x050000
		QPointer<Channel> cContextChannel;
		QPointer<ClientUser> cuContextUser;
#else
		QWeakPointer<Channel> cContextChannel;
		QWeakPointer<ClientUser> cuContextUser;
#endif

		QPoint qpContextPosition;

		void recheckTTS();
		void msgBox(QString msg);
		void setOnTop(bool top);
		void setShowDockTitleBars(bool doShow);
		void updateTrayIcon();
		void updateTransmitModeComboBox();
		QPair<QByteArray, QImage> openImageFile();

#ifdef Q_OS_WIN
#if QT_VERSION >= 0x050000
		bool nativeEvent(const QByteArray &eventType, void *message, long *result) Q_DECL_OVERRIDE;
#else
		bool winEvent(MSG *, long *) Q_DECL_OVERRIDE;
#endif
		unsigned int uiNewHardware;
#endif
	protected:
		QTimer *qtReconnect;

		QList<QAction *> qlServerActions;
		QList<QAction *> qlChannelActions;
		QList<QAction *> qlUserActions;

		QHash<ShortcutTarget, int> qmCurrentTargets;
		QHash<QList<ShortcutTarget>, int> qmTargets;
		QMap<int, int> qmTargetUse;
		int iTargetCounter;

		MUComboBox *qcbTransmitMode;
		QAction *qaTransmitMode;
		QAction *qaTransmitModeSeparator;

		void updateWindowTitle();
		void customEvent(QEvent *evt) Q_DECL_OVERRIDE;
		void findDesiredChannel();
		void closeEvent(QCloseEvent *e) Q_DECL_OVERRIDE;
		void hideEvent(QHideEvent *e) Q_DECL_OVERRIDE;
		void showEvent(QShowEvent *e) Q_DECL_OVERRIDE;
		void changeEvent(QEvent* e) Q_DECL_OVERRIDE;
		void keyPressEvent(QKeyEvent *e) Q_DECL_OVERRIDE;

		QMenu *createPopupMenu() Q_DECL_OVERRIDE;

		bool handleSpecialContextMenu(const QUrl &url, const QPoint &pos_, bool focus = false);
		Channel* getContextMenuChannel();
		ClientUser* getContextMenuUser();

	public slots:
		void on_qaServerDisconnect_triggered();
		void on_qaServerInformation_triggered();
		void on_qaServerTexture_triggered();
		void on_qaServerTextureRemove_triggered();
		void on_qaSelfRegister_triggered();
		void qcbTransmitMode_activated(int index);
		void on_qaUserCommentReset_triggered();
		void on_qaUserTextureReset_triggered();
		void on_qaUserKick_triggered();
		void on_qaUserBan_triggered();
		void on_qaUserMute_triggered();
		void on_qaUserDeaf_triggered();
		void on_qaSelfPrioritySpeaker_triggered();
		void on_qaUserPrioritySpeaker_triggered();
		void on_qaUserRegister_triggered();
		void on_qaUserInformation_triggered();
		void on_qaUserFriendAdd_triggered();
		void on_qaUserFriendRemove_triggered();
		void on_qaUserFriendUpdate_triggered();
		void on_qaChannelJoin_triggered();
		void on_qaChannelRemove_triggered();
		void on_qaChannelACL_triggered();
		void on_qaChannelLink_triggered();
		void on_qaChannelUnlink_triggered();
		void on_qaChannelUnlinkAll_triggered();
		void on_qaChannelCopyURL_triggered();
		void on_qaAudioReset_triggered();
		void on_qaDeveloperConsole_triggered();
		void on_qaHelpWhatsThis_triggered();
		void on_qaHelpAbout_triggered();
		void on_qaHelpAboutQt_triggered();
		void on_qaHelpVersionCheck_triggered();
		void on_qaQuit_triggered();
		void on_qaHide_triggered();
		void on_qteChat_backtabPressed();
		void on_qteLog_anchorClicked(const QUrl &);
		void on_PushToTalk_triggered(bool, QVariant);
		void on_PushToMute_triggered(bool, QVariant);
		void on_VolumeUp_triggered(bool, QVariant);
		void on_VolumeDown_triggered(bool, QVariant);
		void addTarget(ShortcutTarget *);
		void removeTarget(ShortcutTarget *);
		void on_gsCycleTransmitMode_triggered(bool, QVariant);
		void on_gsTransmitModePushToTalk_triggered(bool, QVariant);
		void on_gsTransmitModeContinuous_triggered(bool, QVariant);
		void on_gsTransmitModeVAD_triggered(bool, QVariant);
		void on_gsSendTextMessage_triggered(bool, QVariant);
		void on_Reconnect_timeout();
		void serverConnected();
		void serverDisconnected(QAbstractSocket::SocketError, QString reason);
		void resolverError(QAbstractSocket::SocketError, QString reason);
		void viewCertificate(bool);
		void openUrl(const QUrl &url);
		void updateTarget();
		/// Handles state changes like talking mode changes and mute/unmute
		/// or priority speaker flag changes for the gui user
		void userStateChanged();
		void trayAboutToShow();
		void pttReleased();
		void whisperReleased(QVariant scdata);
		void onResetAudio();
		/// Returns the path to the user's image directory, optionally with a
		/// filename included.
		QString getImagePath(QString filename = QString()) const;
		/// Updates the user's image directory to the given path (any included
		/// filename is discarded).
		void updateImagePath(QString filepath) const;

    signals:
        //signals used by the new UI
        void serverDisconnectedEvent(MumbleProto::Reject_RejectType rtLast,
                                     const QString &reason);
        void userModelChanged();
        void channelJoined(Channel *channel, const QString &userName, unsigned int session);
        void showDialog(const QString &title, const QString &msg);
        void channelAllowedChanged(int id, bool allowed);
        void userDisconnected(const QString &username);

	public:
		MainWindow(QWidget *parent);
		~MainWindow() Q_DECL_OVERRIDE;

		// From msgHandler. Implementation in Messages.cpp
#define MUMBLE_MH_MSG(x) void msg##x(const MumbleProto:: x &);
		MUMBLE_MH_ALL
#undef MUMBLE_MH_MSG
		void removeContextAction(const MumbleProto::ContextActionModify &msg);
};

#endif
