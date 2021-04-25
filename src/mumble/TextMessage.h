// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_TEXTMESSAGE_H_
#define MUMBLE_MUMBLE_TEXTMESSAGE_H_

#include <QDialog>

class TextMessage : public QDialog {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(TextMessage)
	protected:
		QString qsRep;
	public slots:
		void on_qcbTreeMessage_stateChanged(int);
	public:
		TextMessage(QWidget *parent = NULL, QString title = tr("Enter text"), bool bChannel = false);
		static QString autoFormat(QString qsPlain);
		bool bTreeMessage;
};

#endif
