// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

#ifndef MUMBLE_MUMBLE_CERT_H_
#define MUMBLE_MUMBLE_CERT_H_

#include <QtCore/QtGlobal>
#include <QtCore/QString>
#if QT_VERSION >= 0x050000
# include <QtWidgets/QGroupBox>
#else
# include <QtGui/QGroupBox>
#endif

#include <QtNetwork/QHostInfo>
#include <QtNetwork/QSslCertificate>

#include "Settings.h"

class CertWizard {
private:
    Q_DISABLE_COPY(CertWizard)
protected:
    Settings::KeyPair kpCurrent, kpNew;
public:
    CertWizard() = default;
    static bool validateCert(const Settings::KeyPair &);
    static Settings::KeyPair generateNewCert(QString name = QString(), const QString &email = QString());
    static QByteArray exportCert(const Settings::KeyPair &cert);
    static Settings::KeyPair importCert(QByteArray, const QString & = QString());
};

#endif
