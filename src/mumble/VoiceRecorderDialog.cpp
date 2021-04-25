// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "VoiceRecorderDialog.h"

#include "AudioOutput.h"
#include "ServerHandler.h"
#include "VoiceRecorder.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

VoiceRecorderDialog::VoiceRecorderDialog(QWidget *p) : QDialog(p), qtTimer(new QTimer(this)) {
	qtTimer->setObjectName(QLatin1String("qtTimer"));
	qtTimer->setInterval(200);

	QString qsTooltip = QString::fromLatin1(
	                        "%1"
	                        "<table>"
	                        "  <tr>"
	                        "    <td width=\"25%\">%user</td>"
	                        "    <td>%2</td>"
	                        "  </tr>"
	                        "  <tr>"
	                        "    <td>%date</td>"
	                        "    <td>%3</td>"
	                        "  </tr>"
	                        "  <tr>"
	                        "    <td>%time</td>"
	                        "    <td>%4</td>"
	                        "  </tr>"
	                        "  <tr>"
	                        "    <td>%host</td>"
	                        "    <td>%5</td>"
	                        "  </tr>"
	                        "</table>").
	                    arg(tr("Valid variables are:")).
	                    arg(tr("Inserts the user's name")).
	                    arg(tr("Inserts the current date")).
	                    arg(tr("Inserts the current time")).
	                    arg(tr("Inserts the hostname"));

	if (g.s.iRecordingFormat < 0 || g.s.iRecordingFormat > VoiceRecorderFormat::kEnd)
		g.s.iRecordingFormat = 0;
}

VoiceRecorderDialog::~VoiceRecorderDialog() {
	reset();
}

void VoiceRecorderDialog::closeEvent(QCloseEvent *evt) {
	if (g.sh) {
		VoiceRecorderPtr recorder(g.sh->recorder);
		if (recorder && recorder->isRunning()) {
			int ret = QMessageBox::warning(this,
			                               tr("Recorder still running"),
			                               tr("Closing the recorder without stopping it will discard unwritten audio. Do you really want to close the recorder?"),
			                               QMessageBox::Yes | QMessageBox::No,
			                               QMessageBox::No);

			if (ret == QMessageBox::No) {
				evt->ignore();
				return;
			}

			recorder->stop(true);
		}
	}

	reset();
	evt->accept();

	QDialog::closeEvent(evt);
}

void VoiceRecorderDialog::on_qpbStart_clicked() {
	if (!g.uiSession || !g.sh) {
		QMessageBox::critical(this,
		                      tr("Recorder"),
		                      tr("Unable to start recording. Not connected to a server."));
		reset();
		return;
	}

	if (g.sh->uiVersion < 0x010203) {
		QMessageBox::critical(this,
		                      tr("Recorder"),
		                      tr("The server you are currently connected to is version 1.2.2 or older. "
		                         "For privacy reasons, recording on servers of versions older than 1.2.3 "
		                         "is not possible.\nPlease contact your server administrator for further "
		                         "information."));
		return;
	}

	if (g.sh->recorder) {
		QMessageBox::information(this,
		                         tr("Recorder"),
		                         tr("There is already a recorder active for this server."));
		return;
	}

	AudioOutputPtr ao(g.ao);
	if (!ao)
		return;

	g.sh->announceRecordingState(true);
	VoiceRecorderPtr recorder(g.sh->recorder);

	// Wire it up
	connect(&*recorder, SIGNAL(recording_stopped()), this, SLOT(onRecorderStopped()));
	connect(&*recorder, SIGNAL(error(int, QString)), this, SLOT(onRecorderError(int, QString)));

	recorder->start();
}

void VoiceRecorderDialog::on_qpbStop_clicked() {
	if (!g.sh) {
		reset();
		return;
	}

	VoiceRecorderPtr recorder(g.sh->recorder);
	if (!recorder) {
		reset();
		return;
	}

	// Stop clock and recording
	qtTimer->stop();
	recorder->stop();
}

void VoiceRecorderDialog::on_qtTimer_timeout() {
	if (!g.sh) {
		reset();
		return;
	}

	if (!g.uiSession) {
		reset(false);
		return;
	}

	VoiceRecorderPtr recorder(g.sh->recorder);
	if (!g.sh->recorder) {
		reset();
		return;
	}
}

void VoiceRecorderDialog::reset(bool resettimer) {
	qtTimer->stop();

	if (g.sh) {
		VoiceRecorderPtr recorder(g.sh->recorder);
		if (recorder) {
			g.sh->recorder.reset();
			g.sh->announceRecordingState(false);
		}
	}
}

void VoiceRecorderDialog::onRecorderStopped() {
	reset(false);
}

void VoiceRecorderDialog::onRecorderError(int err, QString strerr) {
	Q_UNUSED(err);
	QMessageBox::critical(this,
	                      tr("Recorder"),
	                      strerr);
	reset(false);
}

