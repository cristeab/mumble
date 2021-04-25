// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "LookConfig.h"

#include "AudioInput.h"
#include "AudioOutput.h"
#include "MainWindow.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

static ConfigWidget *LookConfigNew(Settings &st) {
	return new LookConfig(st);
}

static ConfigRegistrar registrar(1100, LookConfigNew);

LookConfig::LookConfig(Settings &st) : ConfigWidget(st) {
}

QString LookConfig::title() const {
	return tr("User Interface");
}

QIcon LookConfig::icon() const {
	return QIcon(QLatin1String("skin:config_ui.png"));
}

void LookConfig::load(const Settings &r) {
}

void LookConfig::save() const {
}

void LookConfig::accept() const {
	g.mw->setShowDockTitleBars((g.s.wlWindowLayout == Settings::LayoutCustom) && !g.s.bLockLayout);
}
