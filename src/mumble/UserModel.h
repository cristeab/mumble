// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_USERMODEL_H_
#define MUMBLE_MUMBLE_USERMODEL_H_

#include <QtCore/QAbstractItemModel>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSet>
#include <QtGui/QIcon>

class User;
class ClientUser;
class Channel;

struct ModelItem Q_DECL_FINAL {
	friend class UserModel;

private:
	Q_DISABLE_COPY(ModelItem)
public:
	Channel *cChan;
	ClientUser *pUser;

	bool bCommentSeen;

	ModelItem *parent;
	QList<ModelItem *> qlChildren;
	QList<ModelItem *> qlHiddenChildren;
	/// Number of users in this channel (recursive)
	int iUsers;

	static QHash <Channel *, ModelItem *> c_qhChannels;
	static QHash <ClientUser *, ModelItem *> c_qhUsers;
	static bool bUsersTop;

	ModelItem(Channel *c);
	ModelItem(ClientUser *p);
	ModelItem(ModelItem *);
	~ModelItem();

	ModelItem *child(int idx) const;

	bool validRow(int idx) const;
	ClientUser *userAt(int idx) const;
	Channel *channelAt(int idx) const;
	int rowOf(Channel *c) const;
	int rowOf(ClientUser *p) const;
	int rowOfSelf() const;
	int rows() const;
	int insertIndex(Channel *c) const;
	int insertIndex(ClientUser *p) const;
	QString hash() const;
	void wipe();
};

class UserModel : public QObject {
		friend struct ModelItem;
	private:
		Q_OBJECT
		Q_DISABLE_COPY(UserModel)
	protected:
		QIcon qiTalkingOn, qiTalkingWhisper, qiTalkingShout, qiTalkingOff;
		QIcon qiMutedPushToMute, qiMutedSelf, qiMutedServer, qiMutedLocal, qiIgnoredLocal, qiMutedSuppressed;
		QIcon qiPrioritySpeaker;
		QIcon qiRecording;
		QIcon qiDeafenedSelf, qiDeafenedServer;
		QIcon qiAuthenticated, qiChannel, qiLinkedChannel, qiActiveChannel;
		QIcon qiFriend;
		QIcon qiComment, qiCommentSeen, qiFilter;
		ModelItem *miRoot;
		QSet<Channel *> qsLinked;
		QMap<QString, ClientUser *> qmHashes;

		bool bClicked;

		QString stringIndex(const QModelIndex &index) const;
	public:
		UserModel(QObject *parent = 0);
        ~UserModel();

		ClientUser *addUser(unsigned int id, const QString &name);
		ClientUser *getUser(const QModelIndex &idx) const;
		ClientUser *getUser(const QString &hash) const;

		Channel *addChannel(int id, Channel *p, const QString &name);
		Channel *getChannel(const QModelIndex &idx) const;

		Channel *getSubChannel(Channel *p, int idx) const;

		void renameUser(ClientUser *p, const QString &name);
		void renameChannel(Channel *c, const QString &name);
		void repositionChannel(Channel *c, const int position);
		void setUserId(ClientUser *p, int id);
		void setHash(ClientUser *p, const QString &hash);
		void setFriendName(ClientUser *p, const QString &name);
		void setComment(ClientUser *p, const QString &comment);
		void setCommentHash(ClientUser *p, const QByteArray &hash);
		void seenComment(const QModelIndex &idx);

		void moveUser(ClientUser *p, Channel *c);
		void moveChannel(Channel *c, Channel *p);
		void setCommentHash(Channel *c, const QByteArray &hash);

		void removeUser(ClientUser *p);
		bool removeChannel(Channel *c, const bool onlyIfUnoccupied = false);

		void linkChannels(Channel *c, QList<Channel *> links);
		void unlinkChannels(Channel *c, QList<Channel *> links);
		void unlinkAll(Channel *c);

		void removeAll();

		QVariant otherRoles(const QModelIndex &idx, int role) const;

		unsigned int uiSessionComment;
		int iChannelDescription;
        ModelItem* rootItem() const { return miRoot; }
};

#endif
