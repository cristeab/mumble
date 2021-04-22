// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "UserModel.h"
#include "Channel.h"
#include "ClientUser.h"
#include "Database.h"
#include "Log.h"
#include "MainWindow.h"
#include "Message.h"
#include "Overlay.h"
#include "ServerHandler.h"
#include "User.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

QHash <Channel *, ModelItem *> ModelItem::c_qhChannels;
QHash <ClientUser *, ModelItem *> ModelItem::c_qhUsers;
bool ModelItem::bUsersTop = false;

ModelItem::ModelItem(Channel *c) {
	this->cChan = c;
	this->pUser = NULL;
	bCommentSeen = true;
	c_qhChannels.insert(c, this);
	parent = c_qhChannels.value(c->cParent);
	iUsers = 0;
}

ModelItem::ModelItem(ClientUser *p) {
	this->cChan = NULL;
	this->pUser = p;
	bCommentSeen = true;
	c_qhUsers.insert(p, this);
	parent = c_qhChannels.value(p->cChannel);
	iUsers = 0;
}

ModelItem::ModelItem(ModelItem *i) {
	// Create a shallow clone
	this->cChan = i->cChan;
	this->pUser = i->pUser;
	this->parent = i->parent;
	this->bCommentSeen = i->bCommentSeen;

	if (pUser)
		c_qhUsers.insert(pUser, this);
	else if (cChan)
		c_qhChannels.insert(cChan, this);

	iUsers = i->iUsers;
}

ModelItem::~ModelItem() {
	Q_ASSERT(qlChildren.count() == 0);

	if (cChan && c_qhChannels.value(cChan) == this)
		c_qhChannels.remove(cChan);
	if (pUser && c_qhUsers.value(pUser) == this)
		c_qhUsers.remove(pUser);
}

void ModelItem::wipe() {
	foreach(ModelItem *i, qlChildren) {
		i->wipe();
		delete i;
	}
	qlChildren.clear();
	iUsers = 0;
}

ModelItem *ModelItem::child(int idx) const {
	if (! validRow(idx))
		return NULL;

	return qlChildren.at(idx);
}

bool ModelItem::validRow(int idx) const {
	return ((idx >= 0) && (idx < qlChildren.count()));
}

ClientUser *ModelItem::userAt(int idx) const {
	if (! validRow(idx))
		return NULL;
	return qlChildren.at(idx)->pUser;
}

Channel *ModelItem::channelAt(int idx) const {
	if (! validRow(idx))
		return NULL;
	return qlChildren.at(idx)->cChan;
}

int ModelItem::rowOf(Channel *c) const {
	for (int i=0;i<qlChildren.count();i++)
		if (qlChildren.at(i)->cChan == c)
			return i;
	return -1;
}

int ModelItem::rowOf(ClientUser *p) const {
	for (int i=0;i<qlChildren.count();i++)
		if (qlChildren.at(i)->pUser == p)
			return i;
	return -1;
}

int ModelItem::rowOfSelf() const {
	// Root?
	if (! parent)
		return 0;

	if (pUser)
		return parent->rowOf(pUser);
	else
		return parent->rowOf(cChan);
}

int ModelItem::rows() const {
	return qlChildren.count();
}

int ModelItem::insertIndex(Channel *c) const {
	QList<Channel*> qlpc;
	int ocount = 0;
    for(auto *item: qlChildren) {
		if (item->cChan) {
			if (item->cChan != c) {
				qlpc << item->cChan;
			}
		} else
			ocount++;
	}
	qlpc << c;
    std::sort(qlpc.begin(), qlpc.end(), Channel::lessThan);
	return qlpc.indexOf(c) + (bUsersTop ? ocount : 0);
}

int ModelItem::insertIndex(ClientUser *p) const {
	QList<ClientUser*> qlclientuser;
	ModelItem *item;

	int ocount = 0;

	foreach(item, qlChildren) {
		if (item->pUser) {
			if (item->pUser != p)
				qlclientuser << item->pUser;
		} else
			ocount++;
	}

	qlclientuser << p;
	qSort(qlclientuser.begin(), qlclientuser.end(), ClientUser::lessThan);

	return qlclientuser.indexOf(p) + (bUsersTop ? 0 : ocount);
}

QString ModelItem::hash() const {
	if (pUser) {
		if (! pUser->qsHash.isEmpty())
			return pUser->qsHash;
		else
			return QLatin1String(sha1(pUser->qsName).toHex());
	} else {
		QCryptographicHash chash(QCryptographicHash::Sha1);

		chash.addData(cChan->qsName.toUtf8());
		chash.addData(QString::number(cChan->iId).toUtf8());
		if (g.sh && g.sh->isRunning()) {
			QString host, user, pw;
			unsigned short port;
			g.sh->getConnectionInfo(host, port, user, pw);
			chash.addData(host.toUtf8());
			chash.addData(QString::number(port).toUtf8());
		}
		return QLatin1String(chash.result().toHex());
	}
}

UserModel::UserModel(QObject *p) {
	ModelItem::bUsersTop = g.s.bUserTop;

	uiSessionComment = 0;
	iChannelDescription = -1;
	bClicked = false;

	miRoot = new ModelItem(Channel::get(0));
}

UserModel::~UserModel() {
	removeAll();
	Q_ASSERT(ModelItem::c_qhUsers.count() == 0);
	Q_ASSERT(ModelItem::c_qhChannels.count() == 1);
	delete miRoot;
}

QString UserModel::stringIndex(const QModelIndex &idx) const {
	ModelItem *item = static_cast<ModelItem *>(idx.internalPointer());
	if (!idx.isValid())
		return QLatin1String("invIdx");
	if (!item)
		return QLatin1String("invPtr");
	if (item->pUser)
		return QString::fromLatin1("P:%1 [%2,%3]").arg(item->pUser->qsName).arg(idx.row()).arg(idx.column());
	else
		return QString::fromLatin1("C:%1 [%2,%3]").arg(item->cChan->qsName).arg(idx.row()).arg(idx.column());
}

QVariant UserModel::otherRoles(const QModelIndex &idx, int role) const {
	ModelItem *item = static_cast<ModelItem *>(idx.internalPointer());
	ClientUser *p = item->pUser;
	Channel *c = item->cChan;
	int section = idx.column();
	bool isUser = p != NULL;

	switch (role) {
		case Qt::ToolTipRole:
			const_cast<UserModel *>(this)->uiSessionComment = 0;
			const_cast<UserModel *>(this)->iChannelDescription = -1;
			const_cast<UserModel *>(this)->bClicked = false;
			switch (section) {
				case 0: {
						if (isUser) {
							QString qsImage;
							if (! p->qbaTextureHash.isEmpty()) {
								if (p->qbaTexture.isEmpty()) {
									p->qbaTexture = g.db->blob(p->qbaTextureHash);
									if (p->qbaTexture.isEmpty()) {
										MumbleProto::RequestBlob mprb;
										mprb.add_session_texture(p->uiSession);
										g.sh->sendMessage(mprb);
									} else {
										g.o->verifyTexture(p);
									}
								}
								if (! p->qbaTexture.isEmpty()) {
									QBuffer qb(&p->qbaTexture);
									qb.open(QIODevice::ReadOnly);
									QImageReader qir(&qb, p->qbaTextureFormat);
									QSize sz = qir.size();
									if (sz.width() > 0) {
										qsImage = QString::fromLatin1("<img src=\"data:;base64,");
										qsImage.append(QString::fromLatin1(p->qbaTexture.toBase64().toPercentEncoding()));
										if (sz.width() > 128) {
											int targ = sz.width() / ((sz.width()+127)/ 128);
											qsImage.append(QString::fromLatin1("\" width=\"%1\" />").arg(targ));
										} else {
											qsImage.append(QString::fromLatin1("\" />"));
										}
									}
								}
							}

							if (p->qbaCommentHash.isEmpty()) {
								if (! qsImage.isEmpty())
									return qsImage;
								else
									return p->qsName;
							} else {
								if (p->qsComment.isEmpty()) {
									p->qsComment = QString::fromUtf8(g.db->blob(p->qbaCommentHash));
									if (p->qsComment.isEmpty()) {
										const_cast<UserModel *>(this)->uiSessionComment = p->uiSession;

										MumbleProto::RequestBlob mprb;
										mprb.add_session_comment(p->uiSession);
										g.sh->sendMessage(mprb);
										return QVariant();
									}
								}
								const_cast<UserModel *>(this)->seenComment(idx);
								QString base = Log::validHtml(p->qsComment);
								if (! qsImage.isEmpty())
									return QString::fromLatin1("<table><tr><td valign=\"top\">%1</td><td>%2</td></tr></table>").arg(qsImage,base);
								return base;
							}
						} else {
							if (c->qbaDescHash.isEmpty()) {
								return c->qsName;
							} else {
								if (c->qsDesc.isEmpty()) {
									c->qsDesc = QString::fromUtf8(g.db->blob(c->qbaDescHash));
									if (c->qsDesc.isEmpty()) {
										const_cast<UserModel *>(this)->iChannelDescription = c->iId;

										MumbleProto::RequestBlob mprb;
										mprb.add_channel_description(c->iId);
										g.sh->sendMessage(mprb);
										return QVariant();
									}
								}

								const_cast<UserModel *>(this)->seenComment(idx);
								return Log::validHtml(c->qsDesc);
							}
						}
					}
					break;
				case 1:
					return isUser ? p->getFlagsString() : QVariant();
			}
			break;
		case Qt::WhatsThisRole:
			switch (section) {
				case 0:
					if (isUser)
						return QString::fromLatin1("%1"
						                           "<table>"
						                           "<tr><td><img src=\"skin:talking_on.svg\" height=64 /></td><td valign=\"middle\">%2</td></tr>"
						                           "<tr><td><img src=\"skin:talking_alt.svg\" height=64 /></td><td valign=\"middle\">%3</td></tr>"
						                           "<tr><td><img src=\"skin:talking_whisper.svg\" height=64 /></td><td valign=\"middle\">%4</td></tr>"
						                           "<tr><td><img src=\"skin:talking_off.svg\" height=64 /></td><td valign=\"middle\">%5</td></tr>"
						                           "</table>").arg(tr("This is a user connected to the server. The icon to the left of the user indicates whether or not they are talking:"),
						                                           tr("Talking to your channel."),
						                                           tr("Shouting directly to your channel."),
						                                           tr("Whispering directly to you."),
						                                           tr("Not talking.")
						                                          );
					else
						return QString::fromLatin1("%1"
						                           "<table>"
						                           "<tr><td><img src=\"skin:channel_active.svg\" height=64 /></td><td valign=\"middle\">%2</td></tr>"
						                           "<tr><td><img src=\"skin:channel_linked.svg\" height=64 /></td><td valign=\"middle\">%3</td></tr>"
						                           "<tr><td><img src=\"skin:channel.svg\" height=64 /></td><td valign=\"middle\">%4</td></tr>"
						                           "</table>").arg(tr("This is a channel on the server. The icon indicates the state of the channel:"),
						                                           tr("Your current channel."),
						                                           tr("A channel that is linked with your channel. Linked channels can talk to each other."),
						                                           tr("A channel on the server that you are not linked to.")
						                                          );
				case 1:
					if (isUser)
						return QString::fromLatin1("%1"
						                           "<table>"
						                           "<tr><td><img src=\"skin:emblems/emblem-favorite.svg\" height=64 /></td><td valign=\"middle\">%2</td></tr>"
						                           "<tr><td><img src=\"skin:authenticated.svg\" height=64 /></td><td valign=\"middle\">%3</td></tr>"
						                           "<tr><td><img src=\"skin:muted_self.svg\" height=64 /></td><td valign=\"middle\">%4</td></tr>"
						                           "<tr><td><img src=\"skin:muted_server.svg\" height=64 /></td><td valign=\"middle\">%5</td></tr>"
						                           "<tr><td><img src=\"skin:muted_suppressed.svg\" height=64 /></td><td valign=\"middle\">%6</td></tr>"
						                           "<tr><td><img src=\"skin:muted_local.svg\" height=64 /></td><td valign=\"middle\">%7</td></tr>"
						                           "<tr><td><img src=\"skin:muted_pushtomute.svg\" height=64 /></td><td valign=\"middle\">%8</td></tr>"
						                           "<tr><td><img src=\"skin:deafened_self.svg\" height=64 /></td><td valign=\"middle\">%9</td></tr>"
						                           "<tr><td><img src=\"skin:deafened_server.svg\" height=64 /></td><td valign=\"middle\">%10</td></tr>"
						                           "<tr><td><img src=\"skin:comment.svg\" height=64 /></td><td valign=\"middle\">%11</td></tr>"
						                           "<tr><td><img src=\"skin:comment_seen.svg\" height=64 /></td><td valign=\"middle\">%12</td></tr>"
						                           "<tr><td><img src=\"skin:status/text-missing.svg\" height=64 /></td><td valign=\"middle\">%13</td></tr>"
						                           "</table>").arg(tr("This shows the flags the user has on the server, if any:"),
						                                           tr("On your friend list"),
						                                           tr("Authenticated user"),
						                                           tr("Muted (manually muted by self)"),
						                                           tr("Muted (manually muted by admin)"),
						                                           tr("Muted (not allowed to speak in current channel)"),
						                                           tr("Muted (muted by you, only on your machine)"),
						                                           tr("Muted (push-to-mute)")
						                                          ).arg(
						                                           tr("Deafened (by self)"),
						                                           tr("Deafened (by admin)"),
						                                           tr("User has a new comment set (click to show)"),
						                                           tr("User has a comment set, which you've already seen. (click to show)"),
						                                           tr("Ignoring Text Messages")
						);
					else
						return QString::fromLatin1("%1"
						                           "<table>"
						                           "<tr><td><img src=\"skin:comment.svg\" height=64 /></td><td valign=\"middle\">%10</td></tr>"
						                           "<tr><td><img src=\"skin:comment_seen.svg\" height=64 /></td><td valign=\"middle\">%11</td></tr>"
						                           "<tr><td><img src=\"skin:filter.svg\" height=64 /></td><td valign=\"middle\">%12</td></tr>"
						                           "</table>").arg(tr("This shows the flags the channel has, if any:"),
						                                           tr("Channel has a new comment set (click to show)"),
						                                           tr("Channel has a comment set, which you've already seen. (click to show)"),
						                                           tr("Channel will be hidden when filtering is enabled")
						                                          );

			}
			break;
	}
	return QVariant();
}

ClientUser *UserModel::addUser(unsigned int id, const QString &name) {
	ClientUser *p = ClientUser::add(id, this);
	p->qsName = name;

	ModelItem *item = new ModelItem(p);

	connect(p, SIGNAL(talkingStateChanged()), this, SLOT(userStateChanged()));
	connect(p, SIGNAL(muteDeafStateChanged()), this, SLOT(userStateChanged()));
	connect(p, SIGNAL(prioritySpeakerStateChanged()), this, SLOT(userStateChanged()));
	connect(p, SIGNAL(recordingStateChanged()), this, SLOT(userStateChanged()));

	Channel *c = Channel::get(0);
	ModelItem *citem = ModelItem::c_qhChannels.value(c);

	item->parent = citem;

	int row = citem->insertIndex(p);
	citem->qlChildren.insert(row, item);
	c->addClientUser(p);

	while (citem) {
		citem->iUsers++;
		citem = citem->parent;
	}

	return p;
}

void UserModel::removeUser(ClientUser *p) {
	if (g.uiSession && p->uiSession == g.uiSession)
		g.uiSession = 0;
	Channel *c = p->cChannel;
	ModelItem *item = ModelItem::c_qhUsers.value(p);
	ModelItem *citem = ModelItem::c_qhChannels.value(c);

	int row = citem->qlChildren.indexOf(item);

	c->removeUser(p);
	citem->qlChildren.removeAt(row);

	p->cChannel = NULL;

	ClientUser::remove(p);
	qmHashes.remove(p->qsHash);

	while (citem) {
		citem->iUsers--;
		citem = citem->parent;
	}

	delete p;
	delete item;
}

void UserModel::moveUser(ClientUser *p, Channel *np) {
	Channel *oc = p->cChannel;
	ModelItem *opi = ModelItem::c_qhChannels.value(oc);
	ModelItem *pi = ModelItem::c_qhChannels.value(np);
	ModelItem *item = ModelItem::c_qhUsers.value(p);

	while (opi) {
		opi->iUsers--;
		opi = opi->parent;
	}
	while (pi) {
		pi->iUsers++;
		pi = pi->parent;
	}
}

void UserModel::renameUser(ClientUser *p, const QString &name) {
	Channel *c = p->cChannel;
	p->qsName = name;

	ModelItem *pi = ModelItem::c_qhChannels.value(c);
	ModelItem *item = ModelItem::c_qhUsers.value(p);
}

void UserModel::setUserId(ClientUser *p, int id) {
	p->iId = id;
}

void UserModel::setHash(ClientUser *p, const QString &hash) {
	if (! p->qsHash.isEmpty())
		qmHashes.remove(p->qsHash);

	p->qsHash = hash;
	qmHashes.insert(p->qsHash, p);
}

void UserModel::setFriendName(ClientUser *p, const QString &name) {
	p->qsFriendName = name;
}

void UserModel::setCommentHash(ClientUser *cu, const QByteArray &hash) {
	if (hash != cu->qbaCommentHash) {
		ModelItem *item = ModelItem::c_qhUsers.value(cu);
		int oldstate = (cu->qsComment.isEmpty() && cu->qbaCommentHash.isEmpty()) ? 0 : (item->bCommentSeen ? 2 : 1);
		int newstate;

		cu->qsComment = QString();
		cu->qbaCommentHash = hash;

		item->bCommentSeen = g.db->seenComment(item->hash(), cu->qbaCommentHash);
		newstate = item->bCommentSeen ? 2 : 1;
	}
}

void UserModel::setCommentHash(Channel *c, const QByteArray &hash) {
	if (hash != c->qbaDescHash) {
		ModelItem *item = ModelItem::c_qhChannels.value(c);
		int oldstate = (c->qsDesc.isEmpty() && c->qbaDescHash.isEmpty()) ? 0 : (item->bCommentSeen ? 2 : 1);
		int newstate;

		c->qsDesc = QString();
		c->qbaDescHash = hash;

		item->bCommentSeen = g.db->seenComment(item->hash(), hash);
		newstate = item->bCommentSeen ? 2 : 1;
	}
}

void UserModel::seenComment(const QModelIndex &idx) {
	ModelItem *item;
	item = static_cast<ModelItem *>(idx.internalPointer());

	if (item->bCommentSeen)
		return;

	item->bCommentSeen = true;

	if (item->pUser)
		g.db->setSeenComment(item->hash(), item->pUser->qbaCommentHash);
	else
		g.db->setSeenComment(item->hash(), item->cChan->qbaDescHash);
}

void UserModel::renameChannel(Channel *c, const QString &name) {
	c->qsName = name;

	if (c->iId == 0) {
	} else {
		Channel *pc = c->cParent;
		ModelItem *pi = ModelItem::c_qhChannels.value(pc);
		ModelItem *item = ModelItem::c_qhChannels.value(c);
	}
}

void UserModel::repositionChannel(Channel *c, const int position) {
	c->iPosition = position;

	if (c->iId == 0) {
	} else {
		Channel *pc = c->cParent;
		ModelItem *pi = ModelItem::c_qhChannels.value(pc);
		ModelItem *item = ModelItem::c_qhChannels.value(c);
	}
}

Channel *UserModel::addChannel(int id, Channel *p, const QString &name) {
	Channel *c = Channel::add(id, name);

	if (! c)
		return NULL;

	ModelItem *item = new ModelItem(c);
	ModelItem *citem = ModelItem::c_qhChannels.value(p);

	item->parent = citem;

	int row = citem->insertIndex(c);
	p->addChannel(c);
	citem->qlChildren.insert(row, item);

	return c;
}

bool UserModel::removeChannel(Channel *c, const bool onlyIfUnoccupied) {
	const ModelItem *item = ModelItem::c_qhChannels.value(c);
	
	if (onlyIfUnoccupied && item->iUsers !=0) return false; // Checks full hierarchy

	foreach(const ModelItem *i, item->qlChildren) {
		if (i->pUser)
			removeUser(i->pUser);
		else
			removeChannel(i->cChan);
	}

	Channel *p = c->cParent;

	if (! p)
		return true;

	ModelItem *citem = ModelItem::c_qhChannels.value(p);

	int row = citem->rowOf(c);
	p->removeChannel(c);
	citem->qlChildren.removeAt(row);
	qsLinked.remove(c);

	Channel::remove(c);

	delete item;
	delete c;
	return true;
}

void UserModel::moveChannel(Channel *c, Channel *p) {
	Channel *oc = c->cParent;
	ModelItem *opi = ModelItem::c_qhChannels.value(c->cParent);
	ModelItem *pi = ModelItem::c_qhChannels.value(p);
	ModelItem *item = ModelItem::c_qhChannels.value(c);

	while (opi) {
		opi->iUsers -= item->iUsers;
		opi = opi->parent;
	}
	while (pi) {
		pi->iUsers += item->iUsers;
		pi = pi->parent;
	}
}

void UserModel::linkChannels(Channel *c, QList<Channel *> links) {
	foreach(Channel *l, links)
		c->link(l);
}

void UserModel::unlinkChannels(Channel *c, QList<Channel *> links) {
	foreach(Channel *l, links)
		c->unlink(l);
}

void UserModel::unlinkAll(Channel *c) {
	c->unlink(NULL);
}

void UserModel::removeAll() {
	ModelItem *item = miRoot;
	ModelItem *i;

	uiSessionComment = 0;
	iChannelDescription = -1;
	bClicked = false;

	foreach(i, item->qlChildren) {
		if (i->pUser)
			removeUser(i->pUser);
		else
			removeChannel(i->cChan);
	}

	qsLinked.clear();
}

ClientUser *UserModel::getUser(const QModelIndex &idx) const {
	if (! idx.isValid())
		return NULL;

	ModelItem *item;
	item = static_cast<ModelItem *>(idx.internalPointer());

	return item->pUser;
}

ClientUser *UserModel::getUser(const QString &hash) const {
	return qmHashes.value(hash);
}

Channel *UserModel::getChannel(const QModelIndex &idx) const {
	if (! idx.isValid())
		return NULL;

	ModelItem *item;
	item = static_cast<ModelItem *>(idx.internalPointer());

	if (item->pUser)
		return item->pUser->cChannel;
	else
		return item->cChan;
}

Channel *UserModel::getSubChannel(Channel *p, int idx) const {
	ModelItem *item=ModelItem::c_qhChannels.value(p);
	if (! item)
		return NULL;

	foreach(ModelItem *i, item->qlChildren) {
		if (i->cChan) {
			if (idx == 0)
				return i->cChan;
			idx--;
		}
	}
	return NULL;
}
