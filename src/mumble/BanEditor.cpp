// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "BanEditor.h"

#include "Channel.h"
#include "Ban.h"
#include "ServerHandler.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

BanEditor::BanEditor(const MumbleProto::BanList &msg, QWidget *p) : QDialog(p)
	, maskDefaultValue(32) {

	qlBans.clear();
	for (int i=0;i < msg.bans_size(); ++i) {
		const MumbleProto::BanList_BanEntry &be = msg.bans(i);
		Ban b;
		b.haAddress = be.address();
		b.iMask = be.mask();
		b.qsUsername = u8(be.name());
		b.qsHash = u8(be.hash());
		b.qsReason = u8(be.reason());
		b.qdtStart = QDateTime::fromString(u8(be.start()), Qt::ISODate);
		b.qdtStart.setTimeSpec(Qt::UTC);
		if (! b.qdtStart.isValid())
			b.qdtStart = QDateTime::currentDateTime();
		b.iDuration = be.duration();
		if (b.isValid())
			qlBans << b;
	}
}

void BanEditor::accept() {
	MumbleProto::BanList msg;

	foreach(const Ban &b, qlBans) {
		MumbleProto::BanList_BanEntry *be = msg.add_bans();
		be->set_address(b.haAddress.toStdString());
		be->set_mask(b.iMask);
		be->set_name(u8(b.qsUsername));
		be->set_hash(u8(b.qsHash));
		be->set_reason(u8(b.qsReason));
		be->set_start(u8(b.qdtStart.toString(Qt::ISODate)));
		be->set_duration(b.iDuration);
	}

	g.sh->sendMessage(msg);
	QDialog::accept();
}
