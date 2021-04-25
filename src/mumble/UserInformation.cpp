// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "UserInformation.h"

#include "Audio.h"
#include "CELTCodec.h"
#include "HostAddress.h"
#include "ServerHandler.h"
#include "ViewCert.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

static QString decode_utf8_qssl_string(const QString &input) {
	QString i = input;
	return QUrl::fromPercentEncoding(i.replace(QLatin1String("\\x"), QLatin1String("%")).toLatin1());
}

#if QT_VERSION >= 0x050000
static QString decode_utf8_qssl_string(const QStringList &list) {
	if (list.count() > 0) {
		return decode_utf8_qssl_string(list.at(0));
	}
	return QString();
}
#endif

UserInformation::UserInformation(const MumbleProto::UserStats &msg, QWidget *p) : QDialog(p) {
	uiSession = msg.session();

	qtTimer = new QTimer(this);
	connect(qtTimer, SIGNAL(timeout()), this, SLOT(tick()));
	qtTimer->start(6000);

	update(msg);
	resize(sizeHint());
}

unsigned int UserInformation::session() const {
	return uiSession;
}

void UserInformation::tick() {
	if (bRequested)
		return;

	bRequested = true;

	g.sh->requestUserStats(uiSession, true);
}

void UserInformation::on_qpbCertificate_clicked() {
	ViewCert *vc = new ViewCert(qlCerts, this);
	vc->setWindowModality(Qt::WindowModal);
	vc->setAttribute(Qt::WA_DeleteOnClose, true);
	vc->show();
}

QString UserInformation::secsToString(unsigned int secs) {
	QStringList qsl;

	int weeks = secs / (60 * 60 * 24 * 7);
	secs = secs % (60 * 60 * 24 * 7);
	int days = secs / (60 * 60 * 24);
	secs = secs % (60 * 60 * 24);
	int hours = secs / (60 * 60);
	secs = secs % (60 * 60);
	int minutes = secs / 60;
	int seconds = secs % 60;

	if (weeks)
		qsl << tr("%1w").arg(weeks);
	if (days)
		qsl << tr("%1d").arg(days);
	if (hours)
		qsl << tr("%1h").arg(hours);
	if (minutes || hours)
		qsl << tr("%1m").arg(minutes);
	qsl << tr("%1s").arg(seconds);

	return qsl.join(QLatin1String(" "));
}

void UserInformation::update(const MumbleProto::UserStats &msg) {
	bRequested = false;

	bool showcon = false;

	ClientUser *cu = ClientUser::get(uiSession);
	if (cu)
		setWindowTitle(cu->qsName);

	if (msg.has_address()) {
		showcon = true;
		HostAddress ha(msg.address());
	}
	if (msg.celt_versions_size() > 0) {
		QStringList qsl;
		for (int i=0;i<msg.celt_versions_size(); ++i) {
			int v = msg.celt_versions(i);
			CELTCodec *cc = g.qmCodecs.value(v);
			if (cc)
				qsl << cc->version();
			else
				qsl << QString::number(v, 16);
		}
	}
}
