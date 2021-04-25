// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "OverlayEditor.h"

#include "OverlayClient.h"
#include "OverlayText.h"
#include "User.h"
#include "Channel.h"
#include "Message.h"
#include "Database.h"
#include "NetworkConfig.h"
#include "ServerHandler.h"
#include "MainWindow.h"
#include "GlobalShortcut.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

OverlayEditor::OverlayEditor(QWidget *p, QGraphicsItem *qgi, OverlaySettings *osptr) :
		QDialog(p),
		qgiPromote(qgi),
		oes(g.s.os) {

	os = osptr ? osptr : &g.s.os;

	QGraphicsProxyWidget *qgpw = graphicsProxyWidget();
	if (qgpw) {
		qgpw->setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
		if (g.ocIntercept) {
			qgpw->setPos(iroundf(static_cast<float>(g.ocIntercept->uiWidth) / 16.0f + 0.5f),
			             iroundf(static_cast<float>(g.ocIntercept->uiHeight) / 16.0f + 0.5f));
			qgpw->resize(iroundf(static_cast<float>(g.ocIntercept->uiWidth) * 14.0f / 16.0f + 0.5f),
			             iroundf(static_cast<float>(g.ocIntercept->uiHeight) * 14.0f / 16.0f + 0.5f));
		}
	}

	reset();
}

OverlayEditor::~OverlayEditor() {
	QGraphicsProxyWidget *qgpw = g.mw->graphicsProxyWidget();
	if (qgpw)
		qgpw->setOpacity(0.9f);
	if (qgiPromote)
		qgiPromote->setZValue(-1.0f);
}

void OverlayEditor::enterEvent(QEvent *e) {
	QGraphicsProxyWidget *qgpw = g.mw->graphicsProxyWidget();
	if (qgpw)
		qgpw->setOpacity(0.9f);

	qgpw = graphicsProxyWidget();
	if (qgpw)
		qgpw->setOpacity(1.0f);

	if (qgiPromote)
		qgiPromote->setZValue(-1.0f);

	QDialog::enterEvent(e);
}

void OverlayEditor::leaveEvent(QEvent *e) {
	QGraphicsProxyWidget *qgpw = g.mw->graphicsProxyWidget();
	if (qgpw)
		qgpw->setOpacity(0.3f);

	qgpw = graphicsProxyWidget();
	if (qgpw)
		qgpw->setOpacity(0.3f);

	if (qgiPromote)
		qgiPromote->setZValue(1.0f);

	QDialog::leaveEvent(e);
}

void OverlayEditor::reset() {
	oes.os = *os;
	oes.resync();
}

void OverlayEditor::apply() {
	*os = oes.os;
	emit applySettings();
}

void OverlayEditor::accept() {
	apply();
	QDialog::accept();
}

void OverlayEditor::on_qrbPassive_clicked() {
	oes.tsColor = Settings::Passive;
	oes.resync();
}

void OverlayEditor::on_qrbTalking_clicked() {
	oes.tsColor = Settings::Talking;
	oes.resync();
}

void OverlayEditor::on_qrbWhisper_clicked() {
	oes.tsColor = Settings::Whispering;
	oes.resync();
}

void OverlayEditor::on_qrbShout_clicked() {
	oes.tsColor = Settings::Shouting;
	oes.resync();
}
