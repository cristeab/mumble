// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "UserEdit.h"

#include <QItemSelectionModel>

#include "Channel.h"
#include "ServerHandler.h"
#include "User.h"
#include "UserListModel.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

UserEdit::UserEdit(const MumbleProto::UserList &userList, QWidget *p)
	: QDialog(p)
	, m_model(new UserListModel(userList, this))
	, m_filter(new UserListFilterProxyModel(this)) {

	const int userCount = userList.users_size();
	setWindowTitle(tr("Registered users: %n account(s)", "", userCount));

	m_filter->setSourceModel(m_model);
}

void UserEdit::accept() {
	if (m_model->isUserListDirty()) {
		MumbleProto::UserList userList = m_model->getUserListUpdate();
		g.sh->sendMessage(userList);
	}

	QDialog::accept();
}

void UserEdit::on_qlSearch_textChanged(QString pattern) {
	m_filter->setFilterWildcard(pattern);
}


void UserEdit::on_qpbRename_clicked() {
}

void UserEdit::on_qpbRemove_clicked() {
}

void UserEdit::on_qtvUserList_customContextMenuRequested(const QPoint &point) {
}

void UserEdit::onSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/) {
}

void UserEdit::onCurrentRowChanged(const QModelIndex &current, const QModelIndex &) {
}

void UserEdit::on_qsbInactive_valueChanged(int) {
	updateInactiveDaysFilter();
}

void UserEdit::on_qcbInactive_currentIndexChanged(int) {
	updateInactiveDaysFilter();
}

void UserEdit::updateInactiveDaysFilter() {
	int minimumInactiveDays = 0;
	m_filter->setFilterMinimumInactiveDays(minimumInactiveDays);
}


UserListFilterProxyModel::UserListFilterProxyModel(QObject *parent_)
	: QSortFilterProxyModel(parent_)
	, m_minimumInactiveDays(0) {

	setFilterKeyColumn(UserListModel::COL_NICK);
	setFilterCaseSensitivity(Qt::CaseInsensitive);
	setSortLocaleAware(true);
	setDynamicSortFilter(true);
}

bool UserListFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
	if(!QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent)) {
		return false;
	}

	const QModelIndex inactiveDaysIdx = sourceModel()->index(source_row,
	                                                         UserListModel::COL_INACTIVEDAYS,
	                                                         source_parent);

	bool ok;
	const int inactiveDays = inactiveDaysIdx.data().toInt(&ok);

	// If inactiveDaysIdx doesn't store an int the account hasn't been seen yet and mustn't be filtered
	if (ok && inactiveDays < m_minimumInactiveDays)
		return false;

	return true;
}

void UserListFilterProxyModel::setFilterMinimumInactiveDays(int minimumInactiveDays) {
	m_minimumInactiveDays = minimumInactiveDays;
	invalidateFilter();
}

void UserListFilterProxyModel::removeRowsInSelection(const QItemSelection &selection) {
	QItemSelection sourceSelection = mapSelectionToSource(selection);
	qobject_cast<UserListModel*>(sourceModel())->removeRowsInSelection(sourceSelection);
}

