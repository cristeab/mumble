// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#include "mumble_pch.hpp"

#include "AudioWizard.h"

#include "AudioInput.h"
#include "AudioOutputSample.h"
#include "Log.h"
#include "MainWindow.h"

// We define a global macro called 'g'. This can lead to issues when included code uses 'g' as a type or parameter name (like protobuf 3.7 does). As such, for now, we have to make this our last include.
#include "Global.h"

CompletablePage::CompletablePage(QWizard *p) : QWizardPage(p) {
	bComplete = true;
}

void CompletablePage::setComplete(bool b) {
	bComplete = b;
	emit completeChanged();
}

bool CompletablePage::isComplete() const {
	return bComplete;
}

AudioWizard::AudioWizard(QWidget *p) : QWizard(p) {
	bInit = true;
	bLastActive = false;
	g.bInAudioWizard = true;

	ticker = new QTimer(this);
	ticker->setObjectName(QLatin1String("Ticker"));

	quint32 iMessage = Settings::LogNone;
	for (int i = Log::firstMsgType;i <= Log::lastMsgType; ++i) {
		iMessage |= (g.s.qmMessages[i] & (Settings::LogSoundfile | Settings::LogTTS));
	}

	fAngle = 0.0f;
	fX = fY = 0.0f;
	qgsScene = NULL;
	qgiSource = NULL;
	aosSource = NULL;

	setOption(QWizard::NoCancelButton, false);
	resize(700, 500);

	sOldSettings = g.s;
	g.s.lmLoopMode = Settings::Local;
	g.s.dPacketLoss = 0.0;
	g.s.dMaxPacketDelay = 0.0;
	g.s.bMute = true;
	g.s.bDeaf = false;

	bTransmitChanged = false;

	iMaxPeak = 0;
	iTicks = 0;

	qpTalkingOn = QPixmap::fromImage(QImage(QLatin1String("skin:talking_on.svg")).scaled(64,64));
	qpTalkingOff = QPixmap::fromImage(QImage(QLatin1String("skin:talking_off.svg")).scaled(64,64));

	bInit = false;

	connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(showPage(int)));

	ticker->setSingleShot(false);
	ticker->start(20);
}

bool AudioWizard::eventFilter(QObject *obj, QEvent *evt) {
	return QWizard::eventFilter(obj, evt);
}

void AudioWizard::on_qcbInput_activated(int) {
	on_qcbInputDevice_activated(0);
}

void AudioWizard::on_qcbInputDevice_activated(int) {
	if (bInit)
		return;

	if (! AudioInputRegistrar::qmNew)
		return;

	Audio::stopInput();
}

void AudioWizard::on_qcbOutput_activated(int) {
	on_qcbOutputDevice_activated(0);
}

void AudioWizard::on_qcbOutputDevice_activated(int) {
	if (bInit)
		return;

	if (! AudioOutputRegistrar::qmNew)
		return;

	Audio::stopOutput();
}

void AudioWizard::on_qsOutputDelay_valueChanged(int v) {
	g.s.iOutputDelay = v;
	restartAudio();
}

void AudioWizard::on_qsMaxAmp_valueChanged(int v) {
	g.s.iMinLoudness = qMin(v, 30000);
}

void AudioWizard::showPage(int pageid) {
	if (pageid == -1)
		return;

	CompletablePage *cp = qobject_cast<CompletablePage *>(currentPage());

	AudioOutputPtr ao = g.ao;
	if (ao)
		ao->wipe();
	aosSource = NULL;

	g.bPosTest = false;
}

int AudioWizard::nextId() const {
	AudioOutputPtr ao = g.ao;

	int nextid = QWizard::nextId();
	return nextid;
}

void AudioWizard::playChord() {
	AudioOutputPtr ao = g.ao;
	if (! ao || aosSource || bInit)
		return;
	aosSource = ao->playSample(QLatin1String(":/wb_male.oga"), true);
}

void AudioWizard::restartAudio() {
	aosSource = NULL;

	Audio::stop();
	Audio::start();

	if (qgsScene) {
		delete qgsScene;
		qgiSource = NULL;
		qgsScene = NULL;
	}
}

void AudioWizard::reject() {
	g.s = sOldSettings;

	g.s.lmLoopMode = Settings::None;
	restartAudio();

	AudioOutputPtr ao = g.ao;
	if (ao)
		ao->wipe();
	aosSource = NULL;
	g.bInAudioWizard = false;

	QWizard::reject();
}

void AudioWizard::accept() {
	if (! bTransmitChanged)
		g.s.atTransmit = sOldSettings.atTransmit;
	else
		g.s.atTransmit = Settings::VAD;

	g.s.bMute = sOldSettings.bMute;
	g.s.bDeaf = sOldSettings.bDeaf;
	g.s.lmLoopMode = Settings::None;

	g.bPosTest = false;
	restartAudio();
	g.bInAudioWizard = false;
	QWizard::accept();
}

bool AudioWizard::validateCurrentPage() {
	return true;
}

void AudioWizard::on_Ticker_timeout() {
	AudioInputPtr ai = g.ai;
	AudioOutputPtr ao = g.ao;
	if (! ai || ! ao)
		return;

	int iPeak = static_cast<int>(ai->dMaxMic);

	if (iTicks++ >= 50) {
		iMaxPeak = 0;
		iTicks = 0;
	}
	if (iPeak > iMaxPeak)
		iMaxPeak = iPeak;

	bool active = ai->isTransmitting();
	if (active != bLastActive) {
		bLastActive = active;
	}

	if (! qgsScene) {
		unsigned int nspeaker = 0;
		const float *spos = ao->getSpeakerPos(nspeaker);
		if ((nspeaker > 0) && spos) {
			qgsScene = new QGraphicsScene(QRectF(-4.0f, -4.0f, 8.0f, 8.0f), this);
			qgsScene->addEllipse(QRectF(-0.12f, -0.12f, 0.24f, 0.24f), QPen(Qt::black), QBrush(Qt::darkRed));
			for (unsigned int i=0;i<nspeaker;++i) {
				if ((spos[3*i] != 0.0f) || (spos[3*i+1] != 0.0f) || (spos[3*i+2] != 0.0f))
					qgsScene->addEllipse(QRectF(spos[3*i] - 0.1f, spos[3*i+2] - 0.1f, 0.2f, 0.2f), QPen(Qt::black), QBrush(Qt::yellow));
			}
			qgiSource = qgsScene->addEllipse(QRectF(-.15f, -.15f, 0.3f, 0.3f), QPen(Qt::black), QBrush(Qt::green));
		}
	}
}

void AudioWizard::on_qsVAD_valueChanged(int v) {
	if (! bInit) {
		g.s.fVADmax = static_cast<float>(v) / 32767.0f;
		g.s.fVADmin = g.s.fVADmax * 0.9f;
	}
}

void AudioWizard::on_qrSNR_clicked(bool on) {
	if (on) {
		g.s.vsVAD = Settings::SignalToNoise;
		g.s.atTransmit = Settings::VAD;
		updateTriggerWidgets(false);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrAmplitude_clicked(bool on) {
	if (on) {
		g.s.vsVAD = Settings::Amplitude;
		g.s.atTransmit = Settings::VAD;
		updateTriggerWidgets(false);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_qrPTT_clicked(bool on) {
	if (on) {
		g.s.atTransmit = Settings::PushToTalk;
		updateTriggerWidgets(true);
		bTransmitChanged = true;
	}
}

void AudioWizard::on_skwPTT_keySet(bool valid, bool last) {
	updateTriggerWidgets(valid);
	bTransmitChanged = true;

	if (last) {
		GlobalShortcutEngine::engine->bNeedRemap = true;
		GlobalShortcutEngine::engine->needRemap();
	}
}

void AudioWizard::on_qcbEcho_clicked(bool on) {
	g.s.bEcho = on;
	restartAudio();
}

void AudioWizard::on_qcbHeadphone_clicked(bool on) {
	g.s.bPositionalHeadphone = on;
	restartAudio();
}

void AudioWizard::on_qcbPositional_clicked(bool on) {
	g.s.bPositionalAudio = on;
	g.s.bTransmitPosition = on;
	restartAudio();
}

void AudioWizard::updateTriggerWidgets(bool ptt) {
}

void AudioWizard::on_qcbAttenuateOthers_clicked(bool checked) {
	g.s.bAttenuateOthers = checked;
}

void AudioWizard::on_qcbHighContrast_clicked(bool on) {
	g.s.bHighContrast = on;
}

void AudioWizard::on_qrbQualityLow_clicked() {
	g.s.iQuality = 16000;
	g.s.iFramesPerPacket = 6;
	restartAudio();
}

void AudioWizard::on_qrbQualityBalanced_clicked() {
	g.s.iQuality = 40000;
	g.s.iFramesPerPacket = 2;
	restartAudio();
}

void AudioWizard::on_qrbQualityUltra_clicked() {
	g.s.iQuality = 72000;
	g.s.iFramesPerPacket = 1;
	restartAudio();
}

void AudioWizard::on_qrbQualityCustom_clicked() {
	g.s.iQuality = sOldSettings.iQuality;
	g.s.iFramesPerPacket = sOldSettings.iFramesPerPacket;
	restartAudio();
}
