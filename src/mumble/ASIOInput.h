// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_ASIOINPUT_H_
#define MUMBLE_MUMBLE_ASIOINPUT_H_

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QWaitCondition>

#include <windows.h>
#include <asiodrvr.h>
#include "AudioInput.h"

typedef QPair<QString, QString> ASIODev;

class ASIOConfig {
	private:
		Q_DISABLE_COPY(ASIOConfig)
	protected:
		QList<ASIODev> qlDevs;
		bool bOk;
	public:
		ASIOConfig(Settings &st);
        virtual QString title() const;
        virtual QIcon icon() const;
        void save() const;
        void load(const Settings &r);
};

#define IEEE754_64FLOAT 1
#include "asio.h"

class ASIOInput : public AudioInput {
	private:
		Q_OBJECT
		Q_DISABLE_COPY(ASIOInput)
	protected:
		IASIO *iasio;

		int iNumMic, iNumSpeaker;
		long lBufSize;
		ASIOBufferInfo *abiInfo;
		ASIOChannelInfo *aciInfo;

		// ASIO Callbacks
		static ASIOInput *aiSelf;

		static void sampleRateChanged(ASIOSampleRate sRate);
		static long asioMessages(long selector, long value, void* message, double* opt);
		static void bufferSwitch(long index, ASIOBool processNow);
		static ASIOTime *bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);

		void addBuffer(long sampType, int interleave, void *src, float * RESTRICT dst);
		void bufferReady(long index);
		bool initializeDriver();

		QWaitCondition qwDone;
	public:
		ASIOInput();
		~ASIOInput() Q_DECL_OVERRIDE;
		void run() Q_DECL_OVERRIDE;
};

#endif
