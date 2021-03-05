// Copyright 2007-2021 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

/// This file contains callback methods for receiving messages (based on Google's ProtoBuf) from the Server.
/// Further details on what exactly is contained in the "message objects" that are parameters to all functions
/// in this file, can be found in the src/Mumble.proto file.

#include "ACLEditor.h"
#include "About.h"
#include "AudioInput.h"
#include "AudioStats.h"
#include "AudioWizard.h"
#include "BanEditor.h"
#include "Channel.h"
#include "ConnectDialog.h"
#include "Connection.h"
#include "Database.h"
#include "Log.h"
#include "MainWindow.h"
#include "GlobalShortcut.h"
#ifdef USE_OVERLAY
#	include "Overlay.h"
#endif
#include "ChannelListener.h"
#include "Plugins.h"
#include "ServerHandler.h"
#include "TalkingUI.h"
#include "User.h"
#include "UserEdit.h"
#include "UserInformation.h"
#include "UserModel.h"
#include "Utils.h"
#include "VersionCheck.h"
#include "ViewCert.h"
#include "crypto/CryptState.h"
#include "Global.h"

#include <QTextDocumentFragment>

#define ACTOR_INIT                           \
	ClientUser *pSrc = nullptr;              \
	if (msg.has_actor())                     \
		pSrc = ClientUser::get(msg.actor()); \
	Q_UNUSED(pSrc);

#define VICTIM_INIT                                                                \
	ClientUser *pDst = ClientUser::get(msg.session());                             \
	if (!pDst) {                                                                   \
		qWarning("MainWindow: Message for nonexistent victim %d.", msg.session()); \
		return;                                                                    \
	}

#define SELF_INIT                                                                           \
	ClientUser *pSelf = ClientUser::get(Global::get().uiSession);                                       \
	if (!pSelf) {                                                                           \
		qWarning("MainWindow: Received message outside of session (sid %d).", Global::get().uiSession); \
		return;                                                                             \
	}

/// The authenticate message is being used by the client to send the authentication credentials to the server. Therefore
/// the server won't send this message type to the client which is why this implementation does nothing.
void MainWindow::msgAuthenticate(const MumbleProto::Authenticate &) {
}

/// This message is being received after this client has queried for the ban list (probably by using the BanEditor).
///
/// @param msg The message object containing information about the ban list
void MainWindow::msgBanList(const MumbleProto::BanList &msg) {
	if (banEdit) {
		banEdit->reject();
		delete banEdit;
		banEdit = nullptr;
	}
	banEdit = new BanEditor(msg, this);
	banEdit->show();
}

/// This message is being received whenever the server rejects the connection of this client.
///
/// @param msg The message object containing the information about why the connection was rejected
void MainWindow::msgReject(const MumbleProto::Reject &msg) {
	rtLast = msg.type();

	QString reason;

	switch (rtLast) {
		case MumbleProto::Reject_RejectType_InvalidUsername:
			reason = tr("Invalid username");
			break;
		case MumbleProto::Reject_RejectType_UsernameInUse:
			reason = tr("Username in use");
			break;
		case MumbleProto::Reject_RejectType_WrongUserPW:
			reason = tr("Wrong certificate or password");
			break;
		case MumbleProto::Reject_RejectType_WrongServerPW:
			reason = tr("Wrong password");
			break;
		case MumbleProto::Reject_RejectType_AuthenticatorFail:
			reason = tr("Your account information can not be verified currently. Please try again later");
			break;
		default:
			reason = u8(msg.reason()).toHtmlEscaped();
			break;
	}

	Global::get().l->log(Log::ServerDisconnected, tr("Server connection rejected: %1.").arg(reason));
	Global::get().l->setIgnore(Log::ServerDisconnected, 1);
}

/// This message is being received when the server has authenticated the user and finished synchronizing the server
/// state. The message contains the session ID (user ID) for this client that gets assigned to Global::uiSession. It
/// also contains information about the maximum bandwidth the user should use and the user's permissions in the root
/// channel. Furthermore the message may contain a welcome message that is logged to Mumble's console if present.
///
/// @param msg The message object with the respective information
void MainWindow::msgServerSync(const MumbleProto::ServerSync &msg) {
	const ClientUser *user = ClientUser::get(msg.session());
	if (!user) {
		Global::get().l->log(Log::CriticalError, tr("Server sync protocol violation. No user profile received."));
		Global::get().sh->disconnect();
		return;
	}
	Global::get().uiSession = msg.session();

	Global::get().sh->sendPing(); // Send initial ping to establish UDP connection

	Global::get().pPermissions = ChanACL::Permissions(static_cast< unsigned int >(msg.permissions()));
	Global::get().l->clearIgnore();
	if (msg.has_welcome_text()) {
		QString str = u8(msg.welcome_text());
		if (!str.isEmpty()) {
			Global::get().l->log(Log::Information, tr("Welcome message: %1").arg(str));
		}
	}
	pmModel->ensureSelfVisible();
	pmModel->recheckLinks();

	// Reset the mechanism for using and recycling target IDs for setting up
	// VoiceTargets
	qmTargetUse.clear();
	qmTargets.clear();
	const int uniqueTargetIDCount = 5;
	for (int i = 1; i < uniqueTargetIDCount + 1; ++i) {
		qmTargetUse.insert(i, i);
	}
	iTargetCounter = 100;

	AudioInput::setMaxBandwidth(msg.max_bandwidth());

	findDesiredChannel();

	QString host, uname, pw;
	unsigned short port;

	Global::get().sh->getConnectionInfo(host, port, uname, pw);

	QList< Shortcut > sc = Global::get().db->getShortcuts(Global::get().sh->qbaDigest);
	if (!sc.isEmpty()) {
		for (int i = 0; i < sc.count(); ++i) {
			Shortcut &s = sc[i];
            if (nullptr != Global::get().mw) {
			    s.iIndex    = Global::get().mw->gsWhisper->idx;
            }
		}
		Global::get().s.qlShortcuts << sc;
		GlobalShortcutEngine::engine->bNeedRemap = true;
	}


	connect(user, SIGNAL(talkingStateChanged()), this, SLOT(userStateChanged()));
	connect(user, SIGNAL(muteDeafStateChanged()), this, SLOT(userStateChanged()));
	connect(user, SIGNAL(prioritySpeakerStateChanged()), this, SLOT(userStateChanged()));
	connect(user, SIGNAL(recordingStateChanged()), this, SLOT(userStateChanged()));

	qstiIcon->setToolTip(tr("Mumble: %1").arg(Channel::get(0)->qsName.toHtmlEscaped()));

	// Update QActions and menus
	on_qmServer_aboutToShow();
	on_qmSelf_aboutToShow();
	qmChannel_aboutToShow();
	qmUser_aboutToShow();
	on_qmConfig_aboutToShow();

	updateTrayIcon();

	// Set-up all ChannelListeners and their volume adjustments as before for this server
	QList< int > localListeners = Global::get().db->getChannelListeners(Global::get().sh->qbaDigest);

	if (!localListeners.isEmpty()) {
		ChannelListener::setInitialServerSyncDone(false);
		Global::get().sh->startListeningToChannels(localListeners);
	} else {
		// If there are no listeners, then no synchronization is needed in the first place
		ChannelListener::setInitialServerSyncDone(true);
	}

	{
		// Since we are only loading the adjustments from the database, we don't really want to consider the adjustments
		// to have "changed" by this action. Furthermore we are setting the volume adjustments before the listeners
		// officially exist. Therefore some code that would receive the change-event would try to get the respective listener
		// and fail due to it not existing yet.
		// Therefore we block all signals while setting the volume adjustments.
		const QSignalBlocker blocker(ChannelListener::get());

		QHash< int, float > volumeMap = Global::get().db->getChannelListenerLocalVolumeAdjustments(Global::get().sh->qbaDigest);
		QHashIterator< int, float > it(volumeMap);
		while (it.hasNext()) {
			it.next();
			ChannelListener::setListenerLocalVolumeAdjustment(it.key(), it.value());
		}
	}


	Global::get().sh->setServerSynchronized(true);

	emit serverSynchronized();
}

/// This message is being received when the server informs this client about server configuration details. This contains
/// things like the maximum bandwidth, the welcome text, whether HTML in messages is allowed, information about message
/// lengths as well as the maximum amount of users that may be connected to this server.
///
/// @param msg The message object
void MainWindow::msgServerConfig(const MumbleProto::ServerConfig &msg) {
	if (msg.has_welcome_text()) {
		QString str = u8(msg.welcome_text());
		if (!str.isEmpty()) {
			Global::get().l->log(Log::Information, tr("Welcome message: %1").arg(str));
		}
	}
	if (msg.has_max_bandwidth())
		AudioInput::setMaxBandwidth(msg.max_bandwidth());
	if (msg.has_allow_html())
		Global::get().bAllowHTML = msg.allow_html();
	if (msg.has_message_length())
		Global::get().uiMessageLength = msg.message_length();
	if (msg.has_image_message_length())
		Global::get().uiImageLength = msg.image_message_length();
	if (msg.has_max_users())
		Global::get().uiMaxUsers = msg.max_users();
}

/// This message is being received when the server denied the permission to perform a requested action. This function
/// basically informs the user about this denial by printing a message to Mumble's console.
///
/// @param msg The message object containing further details as to why and what Permission has been denied
void MainWindow::msgPermissionDenied(const MumbleProto::PermissionDenied &msg) {
    qDebug() << "msgPermissionDenied" << msg.type();
	switch (msg.type()) {
		case MumbleProto::PermissionDenied_DenyType_Permission: {
			VICTIM_INIT;
			SELF_INIT;
			Channel *c = Channel::get(msg.channel_id());
			if (!c)
				return;
			ChanACL::Permissions permission = static_cast< ChanACL::Permissions >(msg.permission());
			QString pname                   = ChanACL::permName(permission);

			if ((permission == ChanACL::Perm::Enter) && c->hasEnterRestrictions.load()) {
				Global::get().l->log(
					Log::PermissionDenied,
					tr("Unable to %1 into %2 - Adding the respective access (password) token might grant you access.")
						.arg(Log::msgColor(pname, Log::Privilege))
						.arg(Log::formatChannel(c)));
			} else {
				if (pDst == pSelf)
					Global::get().l->log(Log::PermissionDenied, tr("You were denied %1 privileges in %2.")
														.arg(Log::msgColor(pname, Log::Privilege))
														.arg(Log::formatChannel(c)));
				else
					Global::get().l->log(Log::PermissionDenied, tr("%3 was denied %1 privileges in %2.")
														.arg(Log::msgColor(pname, Log::Privilege))
														.arg(Log::formatChannel(c))
														.arg(Log::formatClientUser(pDst, Log::Target)));
			}
		} break;
		case MumbleProto::PermissionDenied_DenyType_SuperUser: {
			Global::get().l->log(Log::PermissionDenied, tr("Denied: Cannot modify SuperUser."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_ChannelName: {
			Global::get().l->log(Log::PermissionDenied, tr("Denied: Invalid channel name."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_TextTooLong: {
			Global::get().l->log(Log::PermissionDenied, tr("Denied: Text message too long."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_H9K: {
			if (Global::get().bHappyEaster) {
				bool bold                             = Global::get().s.bDeaf;
				bool bold2                            = Global::get().s.bTTS;
				Global::get().s.bDeaf                             = false;
				Global::get().s.bTTS                              = true;
				quint32 oflags                        = Global::get().s.qmMessages.value(Log::PermissionDenied);
				Global::get().s.qmMessages[Log::PermissionDenied] = (oflags | Settings::LogTTS) & (~Settings::LogSoundfile);
				Global::get().l->log(Log::PermissionDenied,
						 QString::fromUtf8(Global::get().ccHappyEaster + 39).arg(Global::get().s.qsUsername.toHtmlEscaped()));
				Global::get().s.qmMessages[Log::PermissionDenied] = oflags;
				Global::get().s.bDeaf                             = bold;
				Global::get().s.bTTS                              = bold2;
                if (nullptr != Global::get().mw) {
				    Global::get().mw->setWindowIcon(QIcon(QString::fromUtf8(Global::get().ccHappyEaster)));
				    Global::get().mw->setStyleSheet(QString::fromUtf8(Global::get().ccHappyEaster + 82));
                }
				qWarning() << "Happy Easter";
			}
		} break;
		case MumbleProto::PermissionDenied_DenyType_TemporaryChannel: {
			Global::get().l->log(Log::PermissionDenied, tr("Denied: Operation not permitted in temporary channel."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_MissingCertificate: {
			VICTIM_INIT;
			SELF_INIT;
			if (pDst == pSelf)
				Global::get().l->log(Log::PermissionDenied, tr("You need a certificate to perform this operation."));
			else
				Global::get().l->log(Log::PermissionDenied,
						 tr("%1 does not have a certificate.").arg(Log::formatClientUser(pDst, Log::Target)));
		} break;
		case MumbleProto::PermissionDenied_DenyType_UserName: {
			if (msg.has_name())
				Global::get().l->log(Log::PermissionDenied, tr("Invalid username: %1.").arg(u8(msg.name()).toHtmlEscaped()));
			else
				Global::get().l->log(Log::PermissionDenied, tr("Invalid username."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_ChannelFull: {
			Global::get().l->log(Log::PermissionDenied, tr("Channel is full."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_NestingLimit: {
			Global::get().l->log(Log::PermissionDenied, tr("Channel nesting limit reached."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_ChannelCountLimit: {
			Global::get().l->log(Log::PermissionDenied,
					 tr("Channel count limit reached. Need to delete channels before creating new ones."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_ChannelListenerLimit: {
			Global::get().l->log(Log::PermissionDenied, tr("No more listeners allowed in this channel."));
		} break;
		case MumbleProto::PermissionDenied_DenyType_UserListenerLimit: {
			Global::get().l->log(Log::PermissionDenied,
					 tr("You are not allowed to listen to more channels than you currently are."));
		} break;
		default: {
			if (msg.has_reason())
				Global::get().l->log(Log::PermissionDenied, tr("Denied: %1.").arg(u8(msg.reason()).toHtmlEscaped()));
			else
				Global::get().l->log(Log::PermissionDenied, tr("Permission denied."));
		} break;
	}
}

/// This message is not used (anymore). Thus the implementation does nothing
void MainWindow::msgUDPTunnel(const MumbleProto::UDPTunnel &) {
}

/// This message is being received when the server informs this client about changed users. This might be because there
/// is a new user or because an existing user changed somehow (this includes things like a changed ID, changed name,
/// changed priority speaker status, changed channel, etc.). This function will match the local user representation
/// (UserModel) to these changes.
///
/// @param msg The message object containing the respective information
void MainWindow::msgUserState(const MumbleProto::UserState &msg) {
	ACTOR_INIT;
	ClientUser *pSelf = ClientUser::get(Global::get().uiSession);
	ClientUser *pDst  = ClientUser::get(msg.session());
	Channel *channel  = nullptr;

	if (msg.has_channel_id()) {
		channel = Channel::get(msg.channel_id());
		if (!channel) {
			qWarning("msgUserState(): unknown channel.");
			channel = Channel::get(0);
		}
	}

	// User just connected
	if (!pDst) {
		if (!msg.has_name()) {
			return;
		}

		pDst = pmModel->addUser(msg.session(), u8(msg.name()));

		connect(pDst, &ClientUser::talkingStateChanged, Global::get().talkingUI, &TalkingUI::on_talkingStateChanged);
		connect(pDst, &ClientUser::muteDeafStateChanged, Global::get().talkingUI, &TalkingUI::on_muteDeafStateChanged);

		if (channel) {
			pmModel->moveUser(pDst, channel);
		}

		if (msg.has_hash()) {
			pmModel->setHash(pDst, u8(msg.hash()));
		}

		if (pSelf) {
			if (pDst->cChannel == pSelf->cChannel) {
                qInfo() << pDst->qsName << "connected and entered channel";
                //g.l->log(Log::ChannelJoinConnect,
                //		 tr("%1 connected and entered channel.").arg(Log::formatClientUser(pDst, Log::Source)));
			} else {
                qInfo() << pDst->qsName << "connected";
                //g.l->log(Log::UserJoin, tr("%1 connected.").arg(Log::formatClientUser(pDst, Log::Source)));
			}
		}
	}

	if (msg.has_user_id()) {
		pmModel->setUserId(pDst, msg.user_id());
	}

	if (channel) {
		Channel *oldChannel = pDst->cChannel;
		if (channel != oldChannel) {
			pmModel->moveUser(pDst, channel);

			if (Global::get().talkingUI) {
				// Pass the pointer as QObject in order to avoid having to register ClientUser as a QMetaType
				QMetaObject::invokeMethod(Global::get().talkingUI, "on_channelChanged", Qt::QueuedConnection,
										  Q_ARG(QObject *, pDst));
			}

			if (pSelf) {
				if (pDst == pSelf) {
                    if (nullptr != Global::get().mw) {
					    Global::get().mw->updateChatBar();
                    }
					qsDesiredChannel = channel->getPath();
				}

				if (pDst == pSelf) {
					if (pSrc == pSelf) {
                        qInfo() << "You joined" << channel->qsName;
                        emit channelJoined(channel, "");
                        //g.l->log(Log::SelfChannelJoin, tr("You joined %1.").arg(Log::formatChannel(channel)));
					} else {
                        qInfo() << "You were moved to" << channel->qsName << "by" << pSrc->qsName;
                        emit channelJoined(channel, "");
                        //g.l->log(Log::SelfChannelJoinOther, tr("You were moved to %1 by %2.")
                        //										.arg(Log::formatChannel(channel))
                        //										.arg(Log::formatClientUser(pSrc, Log::Source)));
					}
				} else if (pSrc == pSelf) {
					if (channel == pSelf->cChannel) {
                        qInfo() << "You moved" << pDst->qsName << "to" << channel->qsName;
                        emit channelJoined(channel, "");
                        //g.l->log(Log::ChannelJoin, tr("You moved %1 to %2.")
                        //							   .arg(Log::formatClientUser(pDst, Log::Target))
                        //							   .arg(Log::formatChannel(channel)));
					} else {
                        qInfo() << "You moved" << pDst->qsName << "to" << channel->qsName;
                        emit channelJoined(channel, "");
                        //g.l->log(Log::ChannelLeave, tr("You moved %1 to %2.")
                        //								.arg(Log::formatClientUser(pDst, Log::Target))
                        //								.arg(Log::formatChannel(channel)));
					}
				} else if ((channel == pSelf->cChannel) || (oldChannel == pSelf->cChannel)) {
					if (pDst == pSrc) {
						if (channel == pSelf->cChannel) {
                            qInfo() << pDst->qsName << "entered channel";
                            emit channelJoined(channel, pDst->qsName);
                            //g.l->log(Log::ChannelJoin,
                            //		 tr("%1 entered channel.").arg(Log::formatClientUser(pDst, Log::Target)));
						} else {
                            qInfo() << pDst->qsName << "moved to" << channel->qsName;
                            emit channelJoined(channel, pDst->qsName);
                            //g.l->log(Log::ChannelLeave, tr("%1 moved to %2.")
                            //								.arg(Log::formatClientUser(pDst, Log::Target))
                            //								.arg(Log::formatChannel(channel)));
						}
					} else {
						if (channel == pSelf->cChannel) {
                            qInfo() << pDst->qsName << "moved in from" << oldChannel->qsName << "by" << pSrc->qsName;
                            emit channelJoined(channel, pDst->qsName);
                            //g.l->log(Log::ChannelJoin, tr("%1 moved in from %2 by %3.")
                            //							   .arg(Log::formatClientUser(pDst, Log::Target))
                            //							   .arg(Log::formatChannel(oldChannel))
                            //							   .arg(Log::formatClientUser(pSrc, Log::Source)));
						} else {
                            qInfo() << pDst->qsName << "moved to" << channel->qsName << "by" << pSrc->qsName;
                            emit channelJoined(channel, pDst->qsName);
                            //g.l->log(Log::ChannelLeave, tr("%1 moved to %2 by %3.")
                            //								.arg(Log::formatClientUser(pDst, Log::Target))
                            //								.arg(Log::formatChannel(channel))
                            //								.arg(Log::formatClientUser(pSrc, Log::Source)));
						}
					}
				}

				if ((channel == pSelf->cChannel) && pDst->bRecording) {
                    qInfo() << pDst->qsName << "is recording";
                    //g.l->log(Log::Recording, tr("%1 is recording").arg(Log::formatClientUser(pDst, Log::Target)));
				}
			}
		}
	}

	// Handle channel listening
	for (int i = 0; i < msg.listening_channel_add_size(); i++) {
		Channel *c = Channel::get(msg.listening_channel_add(i));

		if (!c) {
			qWarning("msgUserState(): Invalid channel ID encountered");
			continue;
		}

		ChannelListener::addListener(pDst, c);
		emit userAddedChannelListener(pDst, c);

		QString logMsg;
		if (pDst == pSelf) {
			logMsg = tr("You started listening to %1").arg(Log::formatChannel(c));

			// Since ChannelListeners are sent out in bulks (all in a single message), the fact that we received
			// a message that contains information about a ChannelListener of the local user means that we have
			// succecssfully told the server that we are listening to the respective channels. Even if this message
			// here has nothing to do with the actual initial synchronization, this means that we have been connected
			// to the server long enough for the synchronization to be done.
			ChannelListener::setInitialServerSyncDone(true);
		} else if (pSelf && pSelf->cChannel == c) {
			logMsg = tr("%1 started listening to your channel").arg(Log::formatClientUser(pDst, Log::Target));
		}

		if (!logMsg.isEmpty()) {
            qInfo() << logMsg;
            //g.l->log(Log::ChannelListeningAdd, logMsg);
		}
	}
	for (int i = 0; i < msg.listening_channel_remove_size(); i++) {
		Channel *c = Channel::get(msg.listening_channel_remove(i));

		if (!c) {
			qWarning("msgUserState(): Invalid channel ID encountered");
			continue;
		}

		ChannelListener::removeListener(pDst, c);
		emit userRemovedChannelListener(pDst, c);

		QString logMsg;
		if (pDst == pSelf) {
			logMsg = tr("You stopped listening to %1").arg(Log::formatChannel(c));
		} else if (pSelf && pSelf->cChannel == c) {
			logMsg = tr("%1 stopped listening to your channel").arg(Log::formatClientUser(pDst, Log::Target));
		}

		if (!logMsg.isEmpty()) {
            qInfo() << logMsg;
            //g.l->log(Log::ChannelListeningRemove, logMsg);
		}
	}

	if (msg.has_name()) {
		QString oldName = pDst->qsName;
		QString newName = u8(msg.name());
		pmModel->renameUser(pDst, newName);
		if (!oldName.isNull() && oldName != newName) {
			if (pSrc != pDst) {
                qInfo() << oldName << "renamed to" << pDst->qsName << "by" << pSrc->qsName;
                //g.l->log(Log::UserRenamed, tr("%1 renamed to %2 by %3.")
                //							   .arg(Log::formatClientUser(pDst, Log::Target, oldName))
                //							   .arg(Log::formatClientUser(pDst, Log::Target))
                //							   .arg(Log::formatClientUser(pSrc, Log::Source)));
			} else {
                qInfo() << oldName << "renamed to" << pDst->qsName;
                //g.l->log(Log::UserRenamed, tr("%1 renamed to %2.")
                //							   .arg(Log::formatClientUser(pDst, Log::Target, oldName),
                //									Log::formatClientUser(pDst, Log::Target)));
			}
		}
	}

	if (!pDst->qsHash.isEmpty()) {
		const QString &name = Global::get().db->getFriend(pDst->qsHash);
		if (!name.isEmpty())
			pmModel->setFriendName(pDst, name);
		if (Global::get().db->isLocalMuted(pDst->qsHash))
			pDst->setLocalMute(true);
		if (Global::get().db->isLocalIgnored(pDst->qsHash))
			pDst->setLocalIgnore(true);
		if (Global::get().db->isLocalIgnoredTTS(pDst->qsHash))
			pDst->setLocalIgnoreTTS(true);
		pDst->setLocalVolumeAdjustment(Global::get().db->getUserLocalVolume(pDst->qsHash));
		pDst->setLocalNickname(Global::get().db->getUserLocalNickname(pDst->qsHash));
	}

	if (msg.has_self_deaf() || msg.has_self_mute()) {
		if (msg.has_self_mute())
			pDst->setSelfMute(msg.self_mute());
		if (msg.has_self_deaf())
			pDst->setSelfDeaf(msg.self_deaf());

		if (pSelf && pDst != pSelf
			&& ((pDst->cChannel == pSelf->cChannel) || pDst->cChannel->allLinks().contains(pSelf->cChannel))) {
			if (pDst->bSelfMute && pDst->bSelfDeaf)
                qInfo() << pDst->qsName  << "is now muted and deafened";
                //g.l->log(Log::OtherSelfMute,
                //		 tr("%1 is now muted and deafened.").arg(Log::formatClientUser(pDst, Log::Target)));
			else if (pDst->bSelfMute)
                qInfo() << pDst->qsName << "is now muted";
                //g.l->log(Log::OtherSelfMute, tr("%1 is now muted.").arg(Log::formatClientUser(pDst, Log::Target)));
			else
                qInfo() << pDst->qsName << "is now unmuted";
                //g.l->log(Log::OtherSelfMute, tr("%1 is now unmuted.").arg(Log::formatClientUser(pDst, Log::Target)));
		}
	}

	if (msg.has_recording()) {
		pDst->setRecording(msg.recording());

		// Do nothing during initial sync
		if (pSelf) {
			if (pDst == pSelf) {
				if (pDst->bRecording) {
                    qInfo() << "Recording started";
                    //g.l->log(Log::Recording, tr("Recording started"));
				} else {
                    qInfo() << "Recording stopped";
                    //g.l->log(Log::Recording, tr("Recording stopped"));
				}
			} else if (pDst->cChannel->allLinks().contains(pSelf->cChannel)) {
				if (pDst->bRecording) {
                    qInfo() << pDst->qsName << "started recording";
                    //g.l->log(Log::Recording, tr("%1 started recording.").arg(Log::formatClientUser(pDst, Log::Source)));
				} else {
                    qInfo() << pDst->qsName << "stopped recording";
                    //g.l->log(Log::Recording, tr("%1 stopped recording.").arg(Log::formatClientUser(pDst, Log::Source)));
				}
			}
		}
	}

	if (msg.has_priority_speaker()) {
		if (pSelf
			&& ((pDst->cChannel == pSelf->cChannel) || (pDst->cChannel->allLinks().contains(pSelf->cChannel))
				|| (pSrc == pSelf))) {
			if ((pSrc == pDst) && (pSrc == pSelf)) {
				if (pDst->bPrioritySpeaker) {
                    qInfo() << "You revoked your priority speaker status";
                    //g.l->log(Log::YouMuted, tr("You revoked your priority speaker status."));
				} else {
                    qInfo() << "You assumed priority speaker status.";
                    //g.l->log(Log::YouMuted, tr("You assumed priority speaker status."));
				}
			} else if ((pSrc != pSelf) && (pDst == pSelf)) {
				if (pDst->bPrioritySpeaker) {
                    qInfo() << pSrc->qsName << "revoked your priority speaker statu";
                    //g.l->log(
                    //	Log::YouMutedOther,
                    //	tr("%1 revoked your priority speaker status.").arg(Log::formatClientUser(pSrc, Log::Source)));
				} else {
                    qInfo() << pSrc->qsName << "gave you priority speaker status";
                    //g.l->log(Log::YouMutedOther,
                    //		 tr("%1 gave you priority speaker status.").arg(Log::formatClientUser(pSrc, Log::Source)));
				}
			} else if ((pSrc == pSelf) && (pSrc != pDst)) {
				if (pDst->bPrioritySpeaker) {
                    qInfo() << "You revoked priority speaker status for" << pSrc->qsName;
                    //g.l->log(Log::YouMutedOther, tr("You revoked priority speaker status for %1.")
                    //								 .arg(Log::formatClientUser(pDst, Log::Target)));
				} else {
                    qInfo() << "You gave priority speaker status to" << pDst->qsName;
                    //g.l->log(
                    //	Log::YouMutedOther,
                    //	tr("You gave priority speaker status to %1.").arg(Log::formatClientUser(pDst, Log::Target)));
				}
			} else if ((pSrc == pDst) && (pSrc != pSelf)) {
				if (pDst->bPrioritySpeaker) {
                    qInfo() << pSrc->qsName << "revoked own priority speaker status";
                    //g.l->log(
                    //	Log::OtherMutedOther,
                    //	tr("%1 revoked own priority speaker status.").arg(Log::formatClientUser(pSrc, Log::Source)));
				} else {
                    qInfo() << pSrc->qsName << "assumed priority speaker status";
                    //g.l->log(Log::OtherMutedOther,
                    //		 tr("%1 assumed priority speaker status.").arg(Log::formatClientUser(pSrc, Log::Source)));
				}
			} else if ((pSrc != pSelf) && (pDst != pSelf)) {
				if (pDst->bPrioritySpeaker) {
                    qInfo() << pSrc->qsName << "revoked priority speaker status for" << pDst->qsName;
                    //g.l->log(Log::OtherMutedOther, tr("%1 revoked priority speaker status for %2.")
                    //								   .arg(Log::formatClientUser(pSrc, Log::Source),
                    //										Log::formatClientUser(pDst, Log::Target)));
				} else if (!pDst->bPrioritySpeaker) {
                    qInfo() << pSrc->qsName << "gave priority speaker status to" << pDst->qsName;
                    //g.l->log(Log::OtherMutedOther, tr("%1 gave priority speaker status to %2.")
                    //								   .arg(Log::formatClientUser(pSrc, Log::Source),
                    //										Log::formatClientUser(pDst, Log::Target)));
				}
			}
		}

		pDst->setPrioritySpeaker(msg.priority_speaker());
	}

	if (msg.has_deaf() || msg.has_mute() || msg.has_suppress()) {
		if (msg.has_mute())
			pDst->setMute(msg.mute());
		if (msg.has_deaf())
			pDst->setDeaf(msg.deaf());
		if (msg.has_suppress())
			pDst->setSuppress(msg.suppress());

		if (pSelf
			&& ((pDst->cChannel == pSelf->cChannel) || (pDst->cChannel->allLinks().contains(pSelf->cChannel))
				|| (pSrc == pSelf))) {
			if (pDst == pSelf) {
				if (msg.has_mute() && msg.has_deaf() && pDst->bMute && pDst->bDeaf) {
                    qInfo() << "You were muted and deafened by" << pSrc->qsName;
                    //g.l->log(Log::YouMuted,
                    //		 tr("You were muted and deafened by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
				} else if (msg.has_mute() && msg.has_deaf() && !pDst->bMute && !pDst->bDeaf) {
                    qInfo() << "You were unmuted and undeafened by" << pSrc->qsName;
                    //g.l->log(
                    //	Log::YouMuted,
                    //	tr("You were unmuted and undeafened by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
				} else {
					if (msg.has_mute()) {
						if (pDst->bMute)
                            qInfo() << "You were muted by" << pSrc->qsName;
                            //g.l->log(Log::YouMuted,
                            //		 tr("You were muted by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
						else
                            qInfo() << "You were unmuted by" << pSrc->qsName;
                            //g.l->log(Log::YouMuted,
                            //		 tr("You were unmuted by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
					}

					if (msg.has_deaf()) {
						if (!pDst->bDeaf)
                            qInfo() << "You were undeafened by" << pSrc->qsName;
                            //g.l->log(Log::YouMuted,
                            //		 tr("You were undeafened by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
					}
				}

				if (msg.has_suppress()) {
					if (pDst->bSuppress)
                        qInfo() << "You were suppressed";
                        //g.l->log(Log::YouMuted, tr("You were suppressed."));
					else {
						if (msg.has_channel_id())
                            qInfo() << "You were unsuppressed";
                            //g.l->log(Log::YouMuted, tr("You were unsuppressed."));
						else
                            qInfo() << "You were unsuppressed by" << pSrc->qsName;
                            //g.l->log(Log::YouMuted,
                            //		 tr("You were unsuppressed by %1.").arg(Log::formatClientUser(pSrc, Log::Source)));
					}
				}

				updateTrayIcon();
			} else if (pSrc == pSelf) {
				if (msg.has_mute() && msg.has_deaf() && pDst->bMute && pDst->bDeaf) {
                    qInfo() << "You muted and deafened" << pDst->qsName;
                    //g.l->log(Log::YouMutedOther,
                    //		 tr("You muted and deafened %1.").arg(Log::formatClientUser(pDst, Log::Target)));
				} else if (msg.has_mute() && msg.has_deaf() && !pDst->bMute && !pDst->bDeaf) {
                    qInfo() << "You unmuted and undeafened" << pDst->qsName;
                    //g.l->log(Log::YouMutedOther,
                    //		 tr("You unmuted and undeafened %1.").arg(Log::formatClientUser(pDst, Log::Target)));
				} else {
					if (msg.has_mute()) {
						if (pDst->bMute)
                            qInfo() << "You muted" << pDst->qsName;
                            //g.l->log(Log::YouMutedOther,
                            //		 tr("You muted %1.").arg(Log::formatClientUser(pDst, Log::Target)));
						else
                            qInfo() << "You unmuted" << pDst->qsName;
                            //g.l->log(Log::YouMutedOther,
                            //		 tr("You unmuted %1.").arg(Log::formatClientUser(pDst, Log::Target)));
					}

					if (msg.has_deaf()) {
						if (!pDst->bDeaf)
                            qInfo() << "You undeafened" << pDst->qsName;
                            //g.l->log(Log::YouMutedOther,
                            //		 tr("You undeafened %1.").arg(Log::formatClientUser(pDst, Log::Target)));
					}
				}

				if (msg.has_suppress()) {
					if (!msg.has_channel_id()) {
						if (pDst->bSuppress)
                            qInfo() << "You suppressed" << pDst->qsName;
                            //g.l->log(Log::YouMutedOther,
                            //		 tr("You suppressed %1.").arg(Log::formatClientUser(pDst, Log::Target)));
						else
                            qInfo() << "You unsuppressed" << pDst->qsName;
                            //g.l->log(Log::YouMutedOther,
                            //		 tr("You unsuppressed %1.").arg(Log::formatClientUser(pDst, Log::Target)));
					}
				}
			} else {
				if (msg.has_mute() && msg.has_deaf() && pDst->bMute && pDst->bDeaf) {
                    qInfo() << pDst->qsName << "muted and deafened by" << pSrc->qsName;
                    //g.l->log(Log::OtherMutedOther, tr("%1 muted and deafened by %2.")
                    //								   .arg(Log::formatClientUser(pDst, Log::Target),
                    //										Log::formatClientUser(pSrc, Log::Source)));
				} else if (msg.has_mute() && msg.has_deaf() && !pDst->bMute && !pDst->bDeaf) {
                    qInfo() << pDst->qsName << "unmuted and undeafened by" << pSrc->qsName;
                    //g.l->log(Log::OtherMutedOther, tr("%1 unmuted and undeafened by %2.")
                    //								   .arg(Log::formatClientUser(pDst, Log::Target),
                    //										Log::formatClientUser(pSrc, Log::Source)));
				} else {
					if (msg.has_mute()) {
						if (pDst->bMute)
                            qInfo() << pDst->qsName << "muted by" << pSrc->qsName;
                            //g.l->log(Log::OtherMutedOther, tr("%1 muted by %2.")
                            //								   .arg(Log::formatClientUser(pDst, Log::Target),
                            //										Log::formatClientUser(pSrc, Log::Source)));
						else
                            qInfo() << pDst->qsName << "unmuted by" << pSrc->qsName;
                            //g.l->log(Log::OtherMutedOther, tr("%1 unmuted by %2.")
                            //								   .arg(Log::formatClientUser(pDst, Log::Target),
                            //										Log::formatClientUser(pSrc, Log::Source)));
					}

					if (msg.has_deaf()) {
						if (!pDst->bDeaf)
                            qInfo() << pDst->qsName << "undeafened by" << pSrc->qsName;
                            //g.l->log(Log::OtherMutedOther, tr("%1 undeafened by %2.")
                            //								   .arg(Log::formatClientUser(pDst, Log::Target),
                            //										Log::formatClientUser(pSrc, Log::Source)));
					}
				}

				if (msg.has_suppress()) {
					if (!msg.has_channel_id()) {
						if (pDst->bSuppress)
                            qInfo() << pDst->qsName << "suppressed by" << pSrc->qsName;
                            //g.l->log(Log::OtherMutedOther, tr("%1 suppressed by %2.")
                            //								   .arg(Log::formatClientUser(pDst, Log::Target),
                            //										Log::formatClientUser(pSrc, Log::Source)));
						else
                            qInfo() << pDst->qsName << "unsuppressed by" << pSrc->qsName;
                            //g.l->log(Log::OtherMutedOther, tr("%1 unsuppressed by %2.")
                            //								   .arg(Log::formatClientUser(pDst, Log::Target),
                            //										Log::formatClientUser(pSrc, Log::Source)));
					}
				}
			}
		}
	}

	if (msg.has_texture_hash()) {
		pDst->qbaTextureHash = blob(msg.texture_hash());
		pDst->qbaTexture     = QByteArray();
#ifdef USE_OVERLAY
		Global::get().o->verifyTexture(pDst);
#endif
	}
	if (msg.has_texture()) {
		pDst->qbaTexture = blob(msg.texture());
		if (pDst->qbaTexture.isEmpty()) {
			pDst->qbaTextureHash = QByteArray();
		} else {
			pDst->qbaTextureHash = sha1(pDst->qbaTexture);
			Global::get().db->setBlob(pDst->qbaTextureHash, pDst->qbaTexture);
		}
#ifdef USE_OVERLAY
		Global::get().o->verifyTexture(pDst);
#endif
	}
	if (msg.has_comment_hash())
		pmModel->setCommentHash(pDst, blob(msg.comment_hash()));
	if (msg.has_comment())
		pmModel->setComment(pDst, u8(msg.comment()));
}

/// This message is being received when a user was removed. This might be because the user disconnected or because
/// of a kick/ban. The affected user might be the local user.
/// This function will update the local user representation (UserModel) to match these removals and potentially inform
/// the local user about a kick/ban.
///
/// @param msg The message object containing further information
void MainWindow::msgUserRemove(const MumbleProto::UserRemove &msg) {
	VICTIM_INIT;
	ACTOR_INIT;
	SELF_INIT;

	QString reason = u8(msg.reason()).toHtmlEscaped();

	if (pDst == pSelf) {
		bRetryServer = false;
		if (msg.ban())
            qInfo() << "You were kicked and banned from the server by" << pSrc->qsName << ":" << reason;
            //g.l->log(Log::YouKicked, tr("You were kicked and banned from the server by %1: %2.")
            //							 .arg(Log::formatClientUser(pSrc, Log::Source))
            //							 .arg(reason));
		else
            qInfo() << "You were kicked from the server by" << pSrc->qsName << ":" << reason;
            //g.l->log(Log::YouKicked, tr("You were kicked from the server by %1: %2.")
            //							 .arg(Log::formatClientUser(pSrc, Log::Source))
            //							 .arg(reason));
	} else if (pSrc) {
		if (msg.ban())
            qInfo() << pDst->qsName << "was kicked and banned from the server by" << pSrc->qsName << ":" << reason;
            //g.l->log((pSrc == pSelf) ? Log::YouKicked : Log::UserKicked,
            //		 tr("%3 was kicked and banned from the server by %1: %2.")
            //			 .arg(Log::formatClientUser(pSrc, Log::Source))
            //			 .arg(reason)
            //			 .arg(Log::formatClientUser(pDst, Log::Target)));
		else
            qInfo() << pDst->qsName << "was kicked from the server by" << pSrc->qsName << ":" << reason;
            //g.l->log((pSrc == pSelf) ? Log::YouKicked : Log::UserKicked,
            //		 tr("%3 was kicked from the server by %1: %2.")
            //			 .arg(Log::formatClientUser(pSrc, Log::Source))
            //			 .arg(reason)
            //			 .arg(Log::formatClientUser(pDst, Log::Target)));
	} else {
		if (pDst->cChannel == pSelf->cChannel || pDst->cChannel->allLinks().contains(pSelf->cChannel)) {
            qInfo() << pDst->qsName << "left channel and disconnected";
            //g.l->log(Log::ChannelLeaveDisconnect,
            //		 tr("%1 left channel and disconnected.").arg(Log::formatClientUser(pDst, Log::Source)));
		} else {
            qInfo() << pDst->qsName << "disconnected";
            //g.l->log(Log::UserLeave, tr("%1 disconnected.").arg(Log::formatClientUser(pDst, Log::Source)));
		}
	}
	if (pDst != pSelf)
		pmModel->removeUser(pDst);

	QMetaObject::invokeMethod(Global::get().talkingUI, "on_clientDisconnected", Qt::QueuedConnection,
							  Q_ARG(unsigned int, pDst->uiSession));
}

/// This message is being received when the server informs the local client about channel properties (either during
/// connection/login to the server or whenever these properties changed).
///
/// @param msg The message object containing the details about the channel properties
void MainWindow::msgChannelState(const MumbleProto::ChannelState &msg) {
	if (!msg.has_channel_id())
		return;

	Channel *c = Channel::get(msg.channel_id());
	Channel *p = msg.has_parent() ? Channel::get(msg.parent()) : nullptr;

	if (!c) {
		// Addresses channel does not exist so create it
		if (p && msg.has_name()) {
			c             = pmModel->addChannel(msg.channel_id(), p, u8(msg.name()));
			c->bTemporary = msg.temporary();
			p             = nullptr; // No need to move it later

			ServerHandlerPtr sh = Global::get().sh;
			if (sh)
				c->bFiltered = g.db->isChannelFiltered(sh->qbaDigest, c->iId);
            emit userModelChanged();
		} else {
			qWarning("Server attempted state change on nonexistent channel");
			return;
		}
	}

	if (p) {
		// Channel move
		Channel *pp = p;
		while (pp) {
			if (pp == c) {
				qWarning("Server asked to move a channel into itself or one of its children");
				return;
			}

			pp = pp->cParent;
		}
		pmModel->moveChannel(c, p);
	}

	if (msg.has_name())
		pmModel->renameChannel(c, u8(msg.name()));

	if (msg.has_description_hash())
		pmModel->setCommentHash(c, blob(msg.description_hash()));
	if (msg.has_description())
		pmModel->setComment(c, u8(msg.description()));

	if (msg.has_position()) {
		pmModel->repositionChannel(c, msg.position());
	}

	if (msg.links_size()) {
		QList< Channel * > ql;
		pmModel->unlinkAll(c);
		for (int i = 0; i < msg.links_size(); ++i) {
			Channel *l = Channel::get(msg.links(i));
			if (l)
				ql << l;
		}
		if (!ql.isEmpty())
			pmModel->linkChannels(c, ql);
	}
	if (msg.links_remove_size()) {
		QList< Channel * > ql;
		for (int i = 0; i < msg.links_remove_size(); ++i) {
			Channel *l = Channel::get(msg.links_remove(i));
			if (l)
				ql << l;
		}
		if (!ql.isEmpty())
			pmModel->unlinkChannels(c, ql);
	}
	if (msg.links_add_size()) {
		QList< Channel * > ql;
		for (int i = 0; i < msg.links_add_size(); ++i) {
			Channel *l = Channel::get(msg.links_add(i));
			if (l)
				ql << l;
		}
		if (!ql.isEmpty())
			pmModel->linkChannels(c, ql);
	}

	if (msg.has_max_users()) {
		c->uiMaxUsers = msg.max_users();
	}

	bool updateUI = false;

	if (msg.has_is_enter_restricted()) {
		c->hasEnterRestrictions.store(msg.is_enter_restricted());
		updateUI = true;
	}

	if (msg.has_can_enter()) {
		c->localUserCanEnter.store(msg.can_enter());
		updateUI = true;
	}

	if (updateUI) {
		// Passing nullptr to this function will make it do not much except fire a dataChanged event
		// which leads to the UI being updated (reflecting the changes that just took effect).
		this->pmModel->toggleChannelFiltered(nullptr);
	}
}

void MainWindow::msgChannelRemove(const MumbleProto::ChannelRemove &msg) {
	Channel *c = Channel::get(msg.channel_id());
	if (c && (c->iId != 0)) {
		if (c->bFiltered) {
			ServerHandlerPtr sh = Global::get().sh;
			if (sh)
				Global::get().db->setChannelFiltered(sh->qbaDigest, c->iId, false);
			c->bFiltered = false;
		}
		if (!pmModel->removeChannel(c, true)) {
            qCritical() << "Protocol violation. Server sent remove for occupied channel.";
            //g.l->log(Log::CriticalError, tr("Protocol violation. Server sent remove for occupied channel."));
			Global::get()..sh->disconnect();
			return;
		}
	}
}

/// This message is being received because the local client received a text message that should be displayed to
/// the user - which is what this function does.
///
/// @param msg The message object that contains information about the received text message
void MainWindow::msgTextMessage(const MumbleProto::TextMessage &msg) {
	ACTOR_INIT;
	QString target;

	// Silently drop the message if this user is set to "ignore"
	if (pSrc && pSrc->bLocalIgnore)
		return;

	const QString &plainName = pSrc ? pSrc->qsName : tr("Server", "message from");
	const QString &name      = pSrc ? Log::formatClientUser(pSrc, Log::Source) : tr("Server", "message from");
	bool privateMessage      = false;

	if (msg.tree_id_size() > 0) {
		target += tr("Tree");
	} else if (msg.channel_id_size() > 0) {
		target += tr("Channel");
	} else if (msg.session_size() > 0) {
		target += tr("Private");
		privateMessage = true;
	}

	// If NoScope or NoAuthor is selected generate a new string to pass to TTS
	const QString overrideTTS = [&]() {
		if (!Global::get().s.bTTSNoScope && !Global::get().s.bTTSNoAuthor) {
			return QString();
		}
		const QString plainMessage = QTextDocumentFragment::fromHtml(u8(msg.message())).toPlainText();
		if (Global::get().s.bTTSNoScope && Global::get().s.bTTSNoAuthor) {
			return plainMessage;
		}
		const QString prefixTTS = Global::get().s.bTTSNoScope ? plainName : target;
		return tr("%1: %2").arg(prefixTTS).arg(plainMessage);
	}();

	const QString prefixMessage = target.isEmpty() ? name : tr("(%1) %2").arg(target).arg(name);

	Global::get().l->log(privateMessage ? Log::PrivateTextMessage : Log::TextMessage,
			 tr("%1: %2").arg(prefixMessage).arg(u8(msg.message())), tr("Message from %1").arg(plainName), false,
			 overrideTTS, pSrc ? pSrc->bLocalIgnoreTTS : false);
}

/// This message is being received when the server informs the client about the access control list (ACL) for
/// a channel or multiple channels. It seems like this message will only be received after having queried it.
///
/// @param msg The message object holding the ACL and further details
void MainWindow::msgACL(const MumbleProto::ACL &msg) {
	if (aclEdit) {
		aclEdit->reject();
		delete aclEdit;
		aclEdit = nullptr;
	}
	if (Channel::get(msg.channel_id())) {
		aclEdit = new ACLEditor(msg.channel_id(), msg, this);
		aclEdit->show();
	}
}

/// This message is being received when the server informs the local client about user information. This message will
/// only be received after being explicitly queried by the local client.
///
/// @param msg The message object with the respective information
void MainWindow::msgQueryUsers(const MumbleProto::QueryUsers &msg) {
	if (aclEdit)
		aclEdit->returnQuery(msg);
}

/// Pings are a method to check the server-client connection. This implementation does nothing.
void MainWindow::msgPing(const MumbleProto::Ping &) {
}

void MainWindow::msgCryptSetup(const MumbleProto::CryptSetup &msg) {
	ConnectionPtr c = Global::get().sh->cConnection;
	if (!c)
		return;
	if (msg.has_key() && msg.has_client_nonce() && msg.has_server_nonce()) {
		const std::string &key          = msg.key();
		const std::string &client_nonce = msg.client_nonce();
		const std::string &server_nonce = msg.server_nonce();
		if (!c->csCrypt->setKey(key, client_nonce, server_nonce)) {
			qWarning("Messages: Cipher resync failed: Invalid key/nonce from the server!");
		}
	} else if (msg.has_server_nonce()) {
		const std::string &server_nonce = msg.server_nonce();
		if (server_nonce.size() == AES_BLOCK_SIZE) {
			c->csCrypt->uiResync++;
			if (!c->csCrypt->setDecryptIV(server_nonce)) {
				qWarning("Messages: Cipher resync failed: Invalid nonce from the server!");
			}
		}
	} else {
		MumbleProto::CryptSetup mpcs;
		mpcs.set_client_nonce(c->csCrypt->getEncryptIV());
		Global::get().sh->sendMessage(mpcs);
	}
}

/// This messages is only sent by the client if it wants to instantiate a context action. Thus this implementation
/// does nothing.
void MainWindow::msgContextAction(const MumbleProto::ContextAction &) {
}

/// This message is being received if the server wants to instruct the client to add or remove a given context action.
///
/// @param msg The message object with further details about the respective context action
///
/// @see MainWindow::removeContextAction
void MainWindow::msgContextActionModify(const MumbleProto::ContextActionModify &msg) {
	if (msg.has_operation() && msg.operation() == MumbleProto::ContextActionModify_Operation_Remove) {
		removeContextAction(msg);
		return;
	}

	if (msg.has_operation() && msg.operation() != MumbleProto::ContextActionModify_Operation_Add)
		return;

    if (nullptr != Global::get().mw) {
	    QAction *a = new QAction(u8(msg.text()), Global::get().mw);
	    a->setData(u8(msg.action()));
	    connect(a, SIGNAL(triggered()), this, SLOT(context_triggered()));
	    unsigned int ctx = msg.context();
	    if (ctx & MumbleProto::ContextActionModify_Context_Server)
		    qlServerActions.append(a);
	    if (ctx & MumbleProto::ContextActionModify_Context_User)
		    qlUserActions.append(a);
	    if (ctx & MumbleProto::ContextActionModify_Context_Channel)
		    qlChannelActions.append(a);
    }
}

/// Helper method for removing a context action.
///
/// @param msg The message object instructing the deletion of the action with further information about it
///
/// @see MainWindow::msgContextActionModify
void MainWindow::removeContextAction(const MumbleProto::ContextActionModify &msg) {
	QString action = u8(msg.action());

	QSet< QAction * > qs;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
	qs += QSet< QAction * >(qlServerActions.begin(), qlServerActions.end());
	qs += QSet< QAction * >(qlChannelActions.begin(), qlChannelActions.end());
	qs += QSet< QAction * >(qlUserActions.begin(), qlUserActions.end());
#else
	// In Qt 5.14 QList::toSet() has been deprecated as there exists a dedicated constructor of QSet for this now
	qs += qlServerActions.toSet();
	qs += qlChannelActions.toSet();
	qs += qlUserActions.toSet();
#endif

	foreach (QAction *a, qs) {
		if (a->data() == action) {
			qlServerActions.removeOne(a);
			qlChannelActions.removeOne(a);
			qlUserActions.removeOne(a);
			delete a;
		}
	}
}

/// This message is being received in order to set the version information of this client.
///
/// @param msg The message object with the respective information
void MainWindow::msgVersion(const MumbleProto::Version &msg) {
	if (msg.has_version())
		Global::get().sh->uiVersion = msg.version();
	if (msg.has_release())
		Global::get().sh->qsRelease = u8(msg.release());
	if (msg.has_os()) {
		Global::get().sh->qsOS = u8(msg.os());
		if (msg.has_os_version())
			Global::get().sh->qsOSVersion = u8(msg.os_version());
	}
}

/// This message is being received if the client has queried for the list of all users.
///
/// @param msg The message object containing the user list
void MainWindow::msgUserList(const MumbleProto::UserList &msg) {
	if (userEdit) {
		userEdit->reject();
		delete userEdit;
		userEdit = nullptr;
	}
	userEdit = new UserEdit(msg, this);
	userEdit->show();
}

/// This message is only sent by the client in oder to register/clear whisper targets. Therefore
/// this implementation does nothing.
void MainWindow::msgVoiceTarget(const MumbleProto::VoiceTarget &) {
}

/// This message is being received as an answer to the request for certain permissions or if the
/// server wants the client to resync all channel permissions.
///
/// @param msg The message object containing the respective information
void MainWindow::msgPermissionQuery(const MumbleProto::PermissionQuery &msg) {
	Channel *current = pmModel->getChannel(qtvUsers->currentIndex());

	if (msg.flush()) {
		foreach (Channel *c, Channel::c_qhChannels)
			c->uiPermissions = 0;

		// We always need the permissions of the current focus channel
		if (current && current->iId != static_cast< int >(msg.channel_id())) {
			Global::get().sh->requestChannelPermissions(current->iId);

			current->uiPermissions = ChanACL::All;
		}
	}
	Channel *c = Channel::get(msg.channel_id());
	if (c) {
		c->uiPermissions = msg.permissions();
		if (c->iId == 0)
			Global::get().pPermissions = static_cast< ChanACL::Permissions >(c->uiPermissions);
		if (c == current) {
			updateMenuPermissions();
		}
	}
}

/// This message is being received in order for the server to instruct this client which version of the CELT
/// codec it should use.
///
/// @param msg The message object
void MainWindow::msgCodecVersion(const MumbleProto::CodecVersion &msg) {
	int alpha = msg.has_alpha() ? msg.alpha() : -1;
	int beta  = msg.has_beta() ? msg.beta() : -1;
	bool pref = msg.prefer_alpha();

#ifdef USE_OPUS
	static bool warnedOpus = false;
	Global::get().bOpus                = msg.opus();

	if (!g.oCodec && !warnedOpus) {
        qCritical() << "Failed to load Opus, it will not be available for audio encoding/decoding.";
        //g.l->log(Log::CriticalError, tr("Failed to load Opus, it will not be available for audio encoding/decoding."));
		warnedOpus = true;
	}
#endif

	// Workaround for broken 1.2.2 servers
	if (Global::get().sh && Global::get().sh->uiVersion == 0x010202 && alpha != -1 && alpha == beta) {
		if (pref)
			beta = Global::get().iCodecBeta;
		else
			alpha = Global::get().iCodecAlpha;
	}

	if ((alpha != -1) && (alpha != Global::get().iCodecAlpha)) {
		Global::get().iCodecAlpha = alpha;
		if (pref && !Global::get().qmCodecs.contains(alpha))
			pref = !pref;
	}
	if ((beta != -1) && (beta != Global::get().iCodecBeta)) {
		Global::get().iCodecBeta = beta;
		if (!pref && !Global::get().qmCodecs.contains(beta))
			pref = !pref;
	}
	Global::get().bPreferAlpha = pref;

	int willuse = pref ? Global::get().iCodecAlpha : Global::get().iCodecBeta;

	static bool warnedCELT = false;

	if (!Global::get().qmCodecs.contains(willuse)) {
		if (!warnedCELT) {
            qInfo() << "Unable to find matching CELT codecs with other clients. You will not be "
                                            "able to talk to all users.";
            //g.l->log(Log::CriticalError, tr("Unable to find matching CELT codecs with other clients. You will not be "
            //								"able to talk to all users."));
			warnedCELT = true;
		}
	} else {
		warnedCELT = false;
	}
}

/// This message is being received in order to communicate user stats from the server to the client.
///
/// @param msg The message object containing the stats
void MainWindow::msgUserStats(const MumbleProto::UserStats &msg) {
	UserInformation *ui = qmUserInformations.value(msg.session());
	if (ui) {
		ui->update(msg);
	} else {
#ifdef USE_OVERLAY
        if (nullptr != Global::get().mw) {
		    ui = new UserInformation(msg, Global::get().ocIntercept ? Global::get().mw : nullptr);
        }
#else
		ui = new UserInformation(msg, nullptr);
#endif
		ui->setAttribute(Qt::WA_DeleteOnClose, true);
		connect(ui, SIGNAL(destroyed()), this, SLOT(destroyUserInformation()));

		qmUserInformations.insert(msg.session(), ui);
		ui->show();
	}
}

/// This message is only ever sent by the client in order to request binary data that otherwise
/// wouldn't be included in the normal messages (e.Global::get(). big images). Thus this implementation does
/// nothing.
void MainWindow::msgRequestBlob(const MumbleProto::RequestBlob &) {
}

/// This message is being received when the server wants to inform the client about suggested client configurations
/// made by the server administrator. These suggestions will be logged to Mumble's console (if unmet).
///
/// @param msg The message object containing the suggestions
void MainWindow::msgSuggestConfig(const MumbleProto::SuggestConfig &msg) {
	if (msg.has_version() && (msg.version() > MumbleVersion::getRaw())) {
        qWarning() << "The server requests minimum client version" << MumbleVersion::toString(msg.version());
        //g.l->log(Log::Warning,
        //		 tr("The server requests minimum client version %1").arg(MumbleVersion::toString(msg.version())));
	}
	if (msg.has_positional() && (msg.positional() != Global::get().s.doPositionalAudio())) {
		if (msg.positional())
            qWarning() << "The server requests positional audio be enabled.";
            //g.l->log(Log::Warning, tr("The server requests positional audio be enabled."));
		else
            qWarning() << "The server requests positional audio be disabled.";
            //g.l->log(Log::Warning, tr("The server requests positional audio be disabled."));
	}
	if (msg.has_push_to_talk() && (msg.push_to_talk() != (Global::get().s.atTransmit == Settings::PushToTalk))) {
		if (msg.push_to_talk())
            qWarning() << "The server requests Push-to-Talk be enabled.";
            //g.l->log(Log::Warning, tr("The server requests Push-to-Talk be enabled."));
		else
            qWarning() << "The server requests Push-to-Talk be disabled.";
            //g.l->log(Log::Warning, tr("The server requests Push-to-Talk be disabled."));
	}
}

#undef ACTOR_INIT
#undef VICTIM_INIT
#undef SELF_INIT
