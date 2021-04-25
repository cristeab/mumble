// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "ACLEditor.h"

#include "ACL.h"
#include "Channel.h"
#include "ClientUser.h"
#include "Database.h"
#include "Log.h"
#include "ServerHandler.h"
#include "User.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

ACLGroup::ACLGroup(const QString &name) : Group(NULL, name) {
	bInherited = false;
}

ACLEditor::ACLEditor(int channelparentid, QWidget *p) : QDialog(p) {
	// Simple constructor for add channel menu
	bAddChannelMode = true;
	iChannel = channelparentid;

	pcaPassword = NULL;
	adjustSize();
}

ACLEditor::ACLEditor(int channelid, const MumbleProto::ACL &mea, QWidget *p) : QDialog(p) {
	QLabel *l;

	bAddChannelMode = false;

	iChannel = channelid;
	Channel *pChannel = Channel::get(iChannel);
	if (pChannel == NULL) {
		g.l->log(Log::Warning, tr("Failed: Invalid channel"));
		QDialog::reject();
		return;
	}

	msg = mea;

	QSpacerItem *si = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

	foreach(User *u, ClientUser::c_qmUsers) {
		if (u->iId >= 0) {
			qhNameCache.insert(u->iId, u->qsName);
			qhIDCache.insert(u->qsName.toLower(), u->iId);
		}
	}

	ChanACL *def = new ChanACL(NULL);

	def->bApplyHere = true;
	def->bApplySubs = true;
	def->bInherited = true;
	def->iUserId = -1;
	def->qsGroup = QLatin1String("all");
	def->pAllow = ChanACL::Traverse | ChanACL::Enter | ChanACL::Speak | ChanACL::Whisper | ChanACL::TextMessage;
	def->pDeny = (~def->pAllow) & ChanACL::All;

	qlACLs << def;

	for (int i = 0; i < mea.acls_size(); ++i) {
		const MumbleProto::ACL_ChanACL &as = mea.acls(i);

		ChanACL *acl = new ChanACL(NULL);
		acl->bApplyHere = as.apply_here();
		acl->bApplySubs = as.apply_subs();
		acl->bInherited = as.inherited();
		acl->iUserId = -1;
		if (as.has_user_id())
			acl->iUserId = as.user_id();
		else
			acl->qsGroup = u8(as.group());
		acl->pAllow = static_cast<ChanACL::Permissions>(as.grant());
		acl->pDeny = static_cast<ChanACL::Permissions>(as.deny());

		qlACLs << acl;
	}

	for (int i = 0; i < mea.groups_size(); ++i) {
		const MumbleProto::ACL_ChanGroup &gs = mea.groups(i);

		ACLGroup *gp = new ACLGroup(u8(gs.name()));
		gp->bInherit = gs.inherit();
		gp->bInherited = gs.inherited();
		gp->bInheritable = gs.inheritable();
		for (int j = 0; j < gs.add_size(); ++j)
			gp->qsAdd.insert(gs.add(j));
		for (int j = 0; j < gs.remove_size(); ++j)
			gp->qsRemove.insert(gs.remove(j));
		for (int j = 0; j < gs.inherited_members_size(); ++j)
			gp->qsTemporary.insert(gs.inherited_members(j));

		qlGroups << gp;
	}

	iUnknown = -2;

	numInheritACL = -1;

	bInheritACL = mea.inherit_acls();

	foreach(ChanACL *acl, qlACLs) {
		if (acl->bInherited)
			numInheritACL++;
	}

	refill(GroupAdd);
	refill(GroupRemove);
	refill(GroupInherit);
	refill(ACLList);
	refillGroupNames();

	ACLEnableCheck();
	groupEnableCheck();

	updatePasswordField();

	adjustSize();
}

ACLEditor::~ACLEditor() {
	foreach(ChanACL *acl, qlACLs) {
		delete acl;
	}
	foreach(ACLGroup *gp, qlGroups) {
		delete gp;
	}
}

void ACLEditor::showEvent(QShowEvent *evt) {
	ACLEnableCheck();
	QDialog::showEvent(evt);
}

void ACLEditor::accept() {
	Channel *pChannel = Channel::get(iChannel);
	if (pChannel == NULL) {
		// Channel gone while editing
		g.l->log(Log::Warning, tr("Failed: Invalid channel"));
		QDialog::reject();
		return;
	}

	// Update channel state
	if (bAddChannelMode) {
	} else {
		bool needs_update = false;

		updatePasswordACL();

		// Update ACL
		msg.set_inherit_acls(bInheritACL);
		msg.clear_acls();
		msg.clear_groups();

		foreach(ChanACL *acl, qlACLs) {
			if (acl->bInherited || (acl->iUserId < -1))
				continue;
			MumbleProto::ACL_ChanACL *mpa = msg.add_acls();
			mpa->set_apply_here(acl->bApplyHere);
			mpa->set_apply_subs(acl->bApplySubs);
			if (acl->iUserId != -1)
				mpa->set_user_id(acl->iUserId);
			else
				mpa->set_group(u8(acl->qsGroup));
			mpa->set_grant(acl->pAllow);
			mpa->set_deny(acl->pDeny);
		}

		foreach(ACLGroup *gp, qlGroups) {
			if (gp->bInherited && gp->bInherit && gp->bInheritable && (gp->qsAdd.count() == 0) && (gp->qsRemove.count() == 0))
				continue;
			MumbleProto::ACL_ChanGroup *mpg = msg.add_groups();
			mpg->set_name(u8(gp->qsName));
			mpg->set_inherit(gp->bInherit);
			mpg->set_inheritable(gp->bInheritable);
			foreach(int pid, gp->qsAdd)
				if (pid >= 0)
					mpg->add_add(pid);
			foreach(int pid, gp->qsRemove)
				if (pid >= 0)
					mpg->add_remove(pid);
		}
		g.sh->sendMessage(msg);
	}
	QDialog::accept();
}


const QString ACLEditor::userName(int pid) {
	if (qhNameCache.contains(pid))
		return qhNameCache.value(pid);
	else
		return QString::fromLatin1("#%1").arg(pid);
}

int ACLEditor::id(const QString &uname) {
	QString name = uname.toLower();
	if (qhIDCache.contains(name)) {
		return qhIDCache.value(name);
	} else {
		if (! qhNameWait.contains(name)) {
			MumbleProto::QueryUsers mpuq;
			mpuq.add_names(u8(name));
			g.sh->sendMessage(mpuq);

			iUnknown--;
			qhNameWait.insert(name, iUnknown);
			qhNameCache.insert(iUnknown, name);
		}
		return qhNameWait.value(name);
	}
}

void ACLEditor::returnQuery(const MumbleProto::QueryUsers &mqu) {
	if (mqu.names_size() != mqu.ids_size())
		return;

	for (int i = 0; i < mqu.names_size(); ++i) {
		int pid = mqu.ids(i);
		QString name = u8(mqu.names(i));
		QString lname = name.toLower();
		qhIDCache.insert(lname, pid);
		qhNameCache.insert(pid, name);

		if (qhNameWait.contains(lname)) {
			int tid = qhNameWait.take(lname);

			foreach(ChanACL *acl, qlACLs)
				if (acl->iUserId == tid)
					acl->iUserId = pid;
			foreach(ACLGroup *gp, qlGroups) {
				if (gp->qsAdd.remove(tid))
					gp->qsAdd.insert(pid);
				if (gp->qsRemove.remove(tid))
					gp->qsRemove.insert(pid);
			}
			qhNameCache.remove(tid);
		}
	}
	refillGroupInherit();
	refillGroupRemove();
	refillGroupAdd();
	refillComboBoxes();
	refillACL();
}

void ACLEditor::refill(WaitID wid) {
	switch (wid) {
		case ACLList:
			refillACL();
			break;
		case GroupInherit:
			refillGroupInherit();
			break;
		case GroupRemove:
			refillGroupRemove();
			break;
		case GroupAdd:
			refillGroupAdd();
			break;
	}
}

void ACLEditor::refillComboBoxes() {
	QStringList names = qhNameCache.values();
	names.sort();
}

void ACLEditor::refillACL() {
}

void ACLEditor::refillGroupNames() {
}

ACLGroup *ACLEditor::currentGroup() {
	return NULL;
}

ChanACL *ACLEditor::currentACL() {
    return NULL;
}

void ACLEditor::fillWidgetFromSet(QListWidget *qlw, const QSet<int> &qs) {
	qlw->clear();

	QList<idname> ql;
	foreach(int pid, qs) {
		ql << idname(userName(pid), pid);
	}
	qStableSort(ql);
	foreach(idname i, ql) {
		QListWidgetItem *qlwi = new QListWidgetItem(i.first, qlw);
		qlwi->setData(Qt::UserRole, i.second);
		if (i.second < 0) {
			QFont f = qlwi->font();
			f.setItalic(true);
			qlwi->setFont(f);
		}
	}
}

void ACLEditor::refillGroupAdd() {
	ACLGroup *gp = currentGroup();

	if (! gp)
		return;
}

void ACLEditor::refillGroupRemove() {
	ACLGroup *gp = currentGroup();
	if (! gp)
		return;
}

void ACLEditor::refillGroupInherit() {
	ACLGroup *gp = currentGroup();

	if (! gp)
		return;
}

void ACLEditor::groupEnableCheck() {
	ACLGroup *gp = currentGroup();

	bool enabled;
	if (! gp)
		enabled = false;
	else
		enabled = gp->bInherit;

	enabled = (gp != NULL);
}

void ACLEditor::ACLEnableCheck() {
	ChanACL *as = currentACL();

	bool enabled;
	if (! as)
		enabled = false;
	else
		enabled = ! as->bInherited;

	for (int idx = 0; idx < qlACLAllow.count(); idx++) {
		// Only enable other checkboxes if writeacl isn't set
		bool enablethis = enabled && (qlPerms[idx] == ChanACL::Write || !(as && (as->pAllow & ChanACL::Write)) || qlPerms[idx] == ChanACL::Speak);
		qlACLAllow[idx]->setEnabled(enablethis);
		qlACLDeny[idx]->setEnabled(enablethis);
	}

	if (as) {
		for (int idx = 0; idx < qlACLAllow.count(); idx++) {
			ChanACL::Perm p = qlPerms[idx];
			qlACLAllow[idx]->setChecked(as->pAllow & p);
			qlACLDeny[idx]->setChecked(as->pDeny & p);
		}
	}
}

void ACLEditor::on_qtwTab_currentChanged(int index) {
	if (index == 0) {
		// Switched to property tab, update password field
		updatePasswordField();
	} else if (index == 2) {
		// Switched to ACL tab, update ACL list
		updatePasswordACL();
		refillACL();
	}
}

void ACLEditor::updatePasswordField() {
	pcaPassword = NULL;
	foreach(ChanACL *acl, qlACLs) {
		// Check for sth that applies to '#<something>' AND grants 'Enter' AND may grant 'Speak', 'Whisper',
		// 'TextMessage', 'Link' but NOTHING else AND does not deny anything, then '<something>' is the password.
		if (acl->qsGroup.startsWith(QLatin1Char('#')) &&
		        acl->bApplyHere &&
		        !acl->bInherited &&
		        (acl->pAllow & ChanACL::Enter) &&
		        (acl->pAllow == (ChanACL::Enter | ChanACL::Speak | ChanACL::Whisper | ChanACL::TextMessage | ChanACL::LinkChannel) || // Backwards compat with old behaviour that didn't deny traverse
		         acl->pAllow == (ChanACL::Enter | ChanACL::Speak | ChanACL::Whisper | ChanACL::TextMessage | ChanACL::LinkChannel | ChanACL::Traverse)) &&
		        acl->pDeny == ChanACL::None) {
			pcaPassword = acl;
		}
	}
}

void ACLEditor::updatePasswordACL() {
}

void ACLEditor::on_qlwACLs_currentRowChanged() {
	ACLEnableCheck();
}

void ACLEditor::on_qpbACLAdd_clicked() {
	ChanACL *as = new ChanACL(NULL);
	as->bApplyHere = true;
	as->bApplySubs = true;
	as->bInherited = false;
	as->qsGroup = QLatin1String("all");
	as->iUserId = -1;
	as->pAllow = ChanACL::None;
	as->pDeny = ChanACL::None;
	qlACLs << as;
	refillACL();
}

void ACLEditor::on_qpbACLRemove_clicked() {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	qlACLs.removeAll(as);
	delete as;
	refillACL();
}

void ACLEditor::on_qpbACLUp_clicked() {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	int idx = qlACLs.indexOf(as);
	if (idx <= numInheritACL + 1)
		return;

	qlACLs.swap(idx - 1, idx);
	refillACL();
}

void ACLEditor::on_qpbACLDown_clicked() {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	int idx = qlACLs.indexOf(as) + 1;
	if (idx >= qlACLs.count())
		return;

	qlACLs.swap(idx - 1, idx);
	refillACL();
}

void ACLEditor::on_qcbACLInherit_clicked(bool) {
	refillACL();
}

void ACLEditor::on_qcbACLApplyHere_clicked(bool checked) {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	as->bApplyHere = checked;
}

void ACLEditor::on_qcbACLApplySubs_clicked(bool checked) {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	as->bApplySubs = checked;
}

void ACLEditor::on_qcbACLGroup_activated(const QString &text) {
	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	as->iUserId = -1;

	if (text.isEmpty()) {
		as->qsGroup = QLatin1String("all");
	} else {
		as->qsGroup = text;
	}
	refillACL();
}

void ACLEditor::on_qcbACLUser_activated() {
}

void ACLEditor::ACLPermissions_clicked() {
	QCheckBox *source = qobject_cast<QCheckBox *>(sender());

	ChanACL *as = currentACL();
	if (! as || as->bInherited)
		return;

	int allowed = 0;
	int denied = 0;

	bool enabled = true;
	for (int idx = 0; idx < qlACLAllow.count(); idx++) {
		ChanACL::Perm p = qlPerms[idx];
		if (qlACLAllow[idx]->isChecked() && qlACLDeny[idx]->isChecked()) {
			if (source == qlACLAllow[idx])
				qlACLDeny[idx]->setChecked(false);
			else
				qlACLAllow[idx]->setChecked(false);
		}

		qlACLAllow[idx]->setEnabled(enabled || p == ChanACL::Speak);
		qlACLDeny[idx]->setEnabled(enabled || p == ChanACL::Speak);

		if (p == ChanACL::Write && qlACLAllow[idx]->isChecked())
			enabled = false;

		if (qlACLAllow[idx]->isChecked())
			allowed |= p;
		if (qlACLDeny[idx]->isChecked())
			denied |= p;
	}

	as->pAllow = static_cast<ChanACL::Permissions>(allowed);
	as->pDeny = static_cast<ChanACL::Permissions>(denied);
}

void ACLEditor::on_qcbGroupList_activated(const QString &text) {
	ACLGroup *gs = currentGroup();
	if (text.isEmpty())
		return;
	if (! gs) {
		QString name = text.toLower();
		gs = new ACLGroup(name);
		gs->bInherited = false;
		gs->bInherit = true;
		gs->bInheritable = true;
		gs->qsName = name;
		qlGroups << gs;
	}

	refillGroupNames();
	refillGroupAdd();
	refillGroupRemove();
	refillGroupInherit();
	groupEnableCheck();
}

void ACLEditor::on_qcbGroupList_editTextChanged(const QString & text) {
}

void ACLEditor::on_qpbGroupAdd_clicked() {
}

void ACLEditor::on_qpbGroupRemove_clicked() {
	ACLGroup *gs = currentGroup();
	if (! gs)
		return;

	if (gs->bInherited) {
		gs->bInheritable = true;
		gs->bInherit = true;
		gs->qsAdd.clear();
		gs->qsRemove.clear();
	} else {
		qlGroups.removeAll(gs);
		delete gs;
	}
	refillGroupNames();
	refillGroupAdd();
	refillGroupRemove();
	refillGroupInherit();
	groupEnableCheck();
}

void ACLEditor::on_qcbGroupInherit_clicked(bool checked) {
	ACLGroup *gs = currentGroup();
	if (! gs)
		return;

	gs->bInherit = checked;
	groupEnableCheck();
}

void ACLEditor::on_qcbGroupInheritable_clicked(bool checked) {
	ACLGroup *gs = currentGroup();
	if (! gs)
		return;

	gs->bInheritable = checked;
}

void ACLEditor::on_qpbGroupAddAdd_clicked() {
}

void ACLEditor::on_qpbGroupAddRemove_clicked() {
	ACLGroup *gs = currentGroup();
	if (! gs)
		return;
}

void ACLEditor::on_qpbGroupRemoveAdd_clicked() {
}

void ACLEditor::on_qpbGroupRemoveRemove_clicked() {
	refillGroupRemove();
}

void ACLEditor::on_qpbGroupInheritRemove_clicked() {
	refillGroupRemove();
}
