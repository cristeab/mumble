// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "ConfigDialog.h"

#include "AudioInput.h"
#include "AudioOutput.h"
#include "Global.h"
#include "Overlay.h"

ConfigDialog::ConfigDialog(QWidget *p) : QDialog(p) {
	s = g.s;

	unsigned int idx = 0;
	ConfigWidgetNew cwn;
	foreach(cwn, *ConfigRegistrar::c_qmNew) {
		addPage(cwn(s), ++idx);
	}

	updateListView();

	if (! g.s.qbaConfigGeometry.isEmpty()) {
		if (! g.ocIntercept)
			restoreGeometry(g.s.qbaConfigGeometry);
	}
}

void ConfigDialog::addPage(ConfigWidget *cw, unsigned int idx) {
	QDesktopWidget dw;

	int w = INT_MAX, h = INT_MAX;

	for (int i=0;i<dw.numScreens();++i) {
		QRect ds=dw.availableGeometry(i);
		if (ds.isValid()) {
			w = qMin(w, ds.width());
			h = qMin(h, ds.height());
		}
	}

	QSize ms=cw->minimumSizeHint();
	cw->resize(ms);
	cw->setMinimumSize(ms);

	ms.rwidth() += 128;
	ms.rheight() += 192;
	qmWidgets.insert(idx, cw);
	cw->load(g.s);
}

ConfigDialog::~ConfigDialog() {
	foreach(QWidget *qw, qhPages)
		delete qw;
}

void ConfigDialog::on_pageButtonBox_clicked(QAbstractButton *b) {
}

void ConfigDialog::on_dialogButtonBox_clicked(QAbstractButton *b) {
}

void ConfigDialog::on_qlwIcons_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
}

void ConfigDialog::updateListView() {
}

void ConfigDialog::apply() {
	Audio::stop();

	foreach(ConfigWidget *cw, qmWidgets)
		cw->save();

	g.s = s;

	foreach(ConfigWidget *cw, qmWidgets)
		cw->accept();
	
	if (!g.s.bAttenuateOthersOnTalk)
		g.bAttenuateOthers = false;

	// They might have changed their keys.
	g.iPushToTalk = 0;

	Audio::start();
}

void ConfigDialog::accept() {
	apply();

	if (! g.ocIntercept)
		g.s.qbaConfigGeometry=saveGeometry();

	QDialog::accept();
}
