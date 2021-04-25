// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_BANEDITOR_H_
#define MUMBLE_MUMBLE_BANEDITOR_H_

#include "Ban.h"

namespace MumbleProto {
class BanList;
}

class BanEditor : public QDialog {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(BanEditor)
	protected:
		QList<Ban> qlBans;

		int maskDefaultValue;
	public:
		BanEditor(const MumbleProto::BanList &msbl, QWidget *p = NULL);
        void accept();
};

#endif
