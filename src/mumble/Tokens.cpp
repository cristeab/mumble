// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "Tokens.h"

#include "Database.h"
#include "ServerHandler.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

Tokens::Tokens(QWidget *p) : QDialog(p) {
}

void Tokens::accept() {
	QDialog::accept();
}

void Tokens::on_qpbAdd_clicked() {
}

void Tokens::on_qpbRemove_clicked() {
}

