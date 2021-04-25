// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

/* Copyright (C) 2005-2011, Thorvald Natvig <thorvald@natvig.com>
   Copyright (C) 2008, Andreas Messer <andi@bupfen.de>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "mumble_pch.hpp"

#include "AudioConfigDialog.h"

#include "AudioInput.h"
#include "AudioOutput.h"
#include "AudioOutputSample.h"
#include "Global.h"
#include "NetworkConfig.h"

static ConfigWidget *AudioInputDialogNew(Settings &st) {
	return new AudioInputDialog(st);
}

static ConfigWidget *AudioOutputDialogNew(Settings &st) {
	return new AudioOutputDialog(st);
}

static ConfigRegistrar iregistrar(1000, AudioInputDialogNew);
static ConfigRegistrar oregistrar(1010, AudioOutputDialogNew);

void AudioInputDialog::hideEvent(QHideEvent *) {
	qtTick->stop();
}

void AudioInputDialog::showEvent(QShowEvent *) {
	qtTick->start(20);
}

AudioInputDialog::AudioInputDialog(Settings &st) : ConfigWidget(st) {
	qtTick = new QTimer(this);
	qtTick->setObjectName(QLatin1String("Tick"));

	on_qcbPushClick_clicked(g.s.bTxAudioCue);
	on_Tick_timeout();
	on_qcbIdleAction_currentIndexChanged(g.s.iaeIdleAction);
}

QString AudioInputDialog::title() const {
	return tr("Audio Input");
}

QIcon AudioInputDialog::icon() const {
	return QIcon(QLatin1String("skin:config_audio_input.png"));
}

void AudioInputDialog::load(const Settings &r) {
	int i;
	QList<QString> keys;

	if (AudioInputRegistrar::qmNew)
		keys=AudioInputRegistrar::qmNew->keys();
	else
		keys.clear();
	i=keys.indexOf(AudioInputRegistrar::current);

	int echo = 0;
	if (r.bEcho)
		echo = r.bEchoMulti ? 2 : 1;
}

void AudioInputDialog::save() const {
}

void AudioInputDialog::on_qsFrames_valueChanged(int v) {
	updateBitrate();
}

void AudioInputDialog::on_qsDoublePush_valueChanged(int v) {
}

void AudioInputDialog::on_qsPTTHold_valueChanged(int v) {
}

void AudioInputDialog::on_qsTransmitHold_valueChanged(int v) {
}

void AudioInputDialog::on_qsQuality_valueChanged(int v) {
	updateBitrate();
}

void AudioInputDialog::on_qsNoise_valueChanged(int v) {
}

void AudioInputDialog::on_qsAmp_valueChanged(int v) {
}

void AudioInputDialog::updateBitrate() {
}

void AudioInputDialog::on_qcbPushClick_clicked(bool b) {
}

void AudioInputDialog::on_qpbPushClickBrowseOn_clicked() {
}

void AudioInputDialog::on_qpbPushClickBrowseOff_clicked() {
}

void AudioInputDialog::on_qpbPushClickPreview_clicked() {
}

void AudioInputDialog::continuePlayback() {
}

void AudioInputDialog::on_qpbPushClickReset_clicked() {
}

void AudioInputDialog::on_qcbTransmit_currentIndexChanged(int v) {
}

void AudioInputDialog::on_qcbSystem_currentIndexChanged(int) {
}

void AudioInputDialog::on_Tick_timeout() {
}


void AudioInputDialog::on_qcbIdleAction_currentIndexChanged(int v) {
}

AudioOutputDialog::AudioOutputDialog(Settings &st) : ConfigWidget(st) {
}

QString AudioOutputDialog::title() const {
	return tr("Audio Output");
}

QIcon AudioOutputDialog::icon() const {
	return QIcon(QLatin1String("skin:config_audio_output.png"));
}

void AudioOutputDialog::load(const Settings &r) {
	int i;
	QList<QString> keys;

	if (AudioOutputRegistrar::qmNew)
		keys=AudioOutputRegistrar::qmNew->keys();
	else
		keys.clear();
	i=keys.indexOf(AudioOutputRegistrar::current);
}

void AudioOutputDialog::save() const {
}

void AudioOutputDialog::on_qcbSystem_currentIndexChanged(int) {
}

void AudioOutputDialog::on_qsJitter_valueChanged(int v) {
}

void AudioOutputDialog::on_qsVolume_valueChanged(int v) {
}

void AudioOutputDialog::on_qsOtherVolume_valueChanged(int v) {
}

void AudioOutputDialog::on_qsPacketDelay_valueChanged(int v) {
}

void AudioOutputDialog::on_qsPacketLoss_valueChanged(int v) {
}

void AudioOutputDialog::on_qsDelay_valueChanged(int v) {
}

void AudioOutputDialog::on_qcbLoopback_currentIndexChanged(int v) {
}

void AudioOutputDialog::on_qsMinDistance_valueChanged(int v) {
}

void AudioOutputDialog::on_qsMaxDistance_valueChanged(int v) {
}

void AudioOutputDialog::on_qsMaxDistVolume_valueChanged(int v) {
}

void AudioOutputDialog::on_qsBloom_valueChanged(int v) {
}

void AudioOutputDialog::on_qcbPositional_stateChanged(int v) {
}

void AudioOutputDialog::on_qcbAttenuateOthersOnTalk_clicked(bool checked) {
}

void AudioOutputDialog::on_qcbAttenuateOthers_clicked(bool checked) {
}

void AudioOutputDialog::on_qcbOnlyAttenuateSameOutput_clicked(bool checked) {
}
