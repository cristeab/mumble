// Copyright 2005-2019 The Mumble Developers. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file at the root of the
// Mumble source tree or at <https://www.mumble.info/LICENSE>.

// Ignore old-style casts for the whole file.
// We can't use push/pop. They were implemented in GCC 4.6,
// but we still build with GCC 4.2 for the legacy OS X Universal
// build.
#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#include "mumble_pch.hpp"

#include "Cert.h"

#include "Global.h"
#include "SelfSignedCertificate.h"

#define SSL_STRING(x) QString::fromLatin1(x).toUtf8().data()

CertView::CertView(QWidget *p) : QGroupBox(p) {
	QGridLayout *grid = new QGridLayout(this);
	QLabel *l;

	l = new QLabel(tr("Name"));
	grid->addWidget(l, 0, 0, 1, 1, Qt::AlignLeft);

	qlSubjectName = new QLabel();
	qlSubjectName->setTextFormat(Qt::PlainText);
	qlSubjectName->setWordWrap(true);
	grid->addWidget(qlSubjectName, 0, 1, 1, 1);

	l = new QLabel(tr("Email"));
	grid->addWidget(l, 1, 0, 1, 1, Qt::AlignLeft);

	qlSubjectEmail = new QLabel();
	qlSubjectEmail->setTextFormat(Qt::PlainText);
	qlSubjectEmail->setWordWrap(true);
	grid->addWidget(qlSubjectEmail, 1, 1, 1, 1);

	l = new QLabel(tr("Issuer"));
	grid->addWidget(l, 2, 0, 1, 1, Qt::AlignLeft);

	qlIssuerName = new QLabel();
	qlIssuerName->setTextFormat(Qt::PlainText);
	qlIssuerName->setWordWrap(true);
	grid->addWidget(qlIssuerName, 2, 1, 1, 1);

	l = new QLabel(tr("Expiry Date"));
	grid->addWidget(l, 3, 0, 1, 1, Qt::AlignLeft);

	qlExpiry = new QLabel();
	qlExpiry->setWordWrap(true);
	grid->addWidget(qlExpiry, 3, 1, 1, 1);

	grid->setColumnStretch(1, 1);
}

void CertView::setCert(const QList<QSslCertificate> &cert) {
	qlCert = cert;

	if (qlCert.isEmpty()) {
		qlSubjectName->setText(QString());
		qlSubjectEmail->setText(QString());
		qlIssuerName->setText(QString());
		qlExpiry->setText(QString());
	} else {
		QSslCertificate qscCert = qlCert.at(0);

#if QT_VERSION >= 0x050000
		const QStringList &names = qscCert.subjectInfo(QSslCertificate::CommonName);
		QString name;
		if (names.count() > 0) {
			name = names.at(0);
		}

		QStringList emails = qscCert.subjectAlternativeNames().values(QSsl::EmailEntry);
#else
		const QString &name = qscCert.subjectInfo(QSslCertificate::CommonName);
		QStringList emails(qscCert.alternateSubjectNames().values(QSsl::EmailEntry));
#endif

		QString tmpName = name;
		tmpName = tmpName.replace(QLatin1String("\\x"), QLatin1String("%"));
		tmpName = QUrl::fromPercentEncoding(tmpName.toLatin1());

		qlSubjectName->setText(tmpName);

		if (emails.count() > 0)
			qlSubjectEmail->setText(emails.join(QLatin1String("\n")));
		else
			qlSubjectEmail->setText(tr("(none)"));

		if (qscCert.expiryDate() <= QDateTime::currentDateTime())
			qlExpiry->setText(QString::fromLatin1("<font color=\"red\"><b>%1</b></font>").arg(Qt::escape(qscCert.expiryDate().toString(Qt::SystemLocaleDate))));
		else
			qlExpiry->setText(qscCert.expiryDate().toString(Qt::SystemLocaleDate));

		if (qlCert.count() > 1)
			qscCert = qlCert.last();

#if QT_VERSION >= 0x050000
		const QStringList &issuerNames = qscCert.issuerInfo(QSslCertificate::CommonName);
		QString issuerName;
		if (issuerNames.count() > 0) {
			issuerName = issuerNames.at(0);
		}
#else
		const QString &issuerName = qscCert.issuerInfo(QSslCertificate::CommonName);
#endif
		qlIssuerName->setText((issuerName == name) ? tr("Self-signed") : issuerName);
	}
}

CertWizard::CertWizard(QWidget *p) : QWizard(p) {

	setOption(QWizard::NoCancelButton, false);
}

int CertWizard::nextId() const {
	return -1;
}

void CertWizard::initializePage(int id) {
	QWizard::initializePage(id);
}

bool CertWizard::validateCurrentPage() {
	return QWizard::validateCurrentPage();
}

bool CertWizard::validateCert(const Settings::KeyPair &kp) {
	bool valid = ! kp.second.isNull() && ! kp.first.isEmpty();
	foreach(const QSslCertificate &cert, kp.first)
		valid = valid && ! cert.isNull();
	return valid;
}

Settings::KeyPair CertWizard::generateNewCert(QString qsname, const QString &qsemail) {
	QSslCertificate qscCert;
	QSslKey qskKey;

	// Ignore return value.
	// The method sets qscCert and qskKey to null values if it fails.
	SelfSignedCertificate::generateMumbleCertificate(qsname, qsemail, qscCert, qskKey);

	QList<QSslCertificate> qlCert;
	qlCert << qscCert;

	return Settings::KeyPair(qlCert, qskKey);
}

Settings::KeyPair CertWizard::importCert(QByteArray data, const QString &pw) {
	X509 *x509 = NULL;
	EVP_PKEY *pkey = NULL;
	PKCS12 *pkcs = NULL;
	BIO *mem = NULL;
	STACK_OF(X509) *certs = NULL;
	Settings::KeyPair kp;
	int ret = 0;

	mem = BIO_new_mem_buf(data.data(), data.size());
	Q_UNUSED(BIO_set_close(mem, BIO_NOCLOSE));
	pkcs = d2i_PKCS12_bio(mem, NULL);
	if (pkcs) {
		ret = PKCS12_parse(pkcs, NULL, &pkey, &x509, &certs);
		if (pkcs && !pkey && !x509 && ! pw.isEmpty()) {
			if (certs) {
				if (ret)
					sk_X509_free(certs);
				certs = NULL;
			}
			ret = PKCS12_parse(pkcs, pw.toUtf8().constData(), &pkey, &x509, &certs);
		}
		if (pkey && x509 && X509_check_private_key(x509, pkey)) {
			unsigned char *dptr;
			QByteArray key, crt;

			key.resize(i2d_PrivateKey(pkey, NULL));
			dptr=reinterpret_cast<unsigned char *>(key.data());
			i2d_PrivateKey(pkey, &dptr);

			crt.resize(i2d_X509(x509, NULL));
			dptr=reinterpret_cast<unsigned char *>(crt.data());
			i2d_X509(x509, &dptr);

			QSslCertificate qscCert = QSslCertificate(crt, QSsl::Der);
			QSslKey qskKey = QSslKey(key, QSsl::Rsa, QSsl::Der);

			QList<QSslCertificate> qlCerts;
			qlCerts << qscCert;

			if (certs) {
				for (int i=0;i<sk_X509_num(certs);++i) {
					X509 *c = sk_X509_value(certs, i);

					crt.resize(i2d_X509(c, NULL));
					dptr=reinterpret_cast<unsigned char *>(crt.data());
					i2d_X509(c, &dptr);

					QSslCertificate cert = QSslCertificate(crt, QSsl::Der);
					qlCerts << cert;
				}
			}
			bool valid = ! qskKey.isNull();
			foreach(const QSslCertificate &cert, qlCerts)
				valid = valid && ! cert.isNull();
			if (valid)
				kp = Settings::KeyPair(qlCerts, qskKey);
		}
	}

	if (ret) {
		if (pkey)
			EVP_PKEY_free(pkey);
		if (x509)
			X509_free(x509);
		if (certs)
			sk_X509_free(certs);
	}
	if (pkcs)
		PKCS12_free(pkcs);
	if (mem)
		BIO_free(mem);

	return kp;
}

QByteArray CertWizard::exportCert(const Settings::KeyPair &kp) {
	X509 *x509 = NULL;
	EVP_PKEY *pkey = NULL;
	PKCS12 *pkcs = NULL;
	BIO *mem = NULL;
	STACK_OF(X509) *certs = sk_X509_new_null();
	const unsigned char *p;
	char *data = NULL;

	if (kp.first.isEmpty())
		return QByteArray();

	QByteArray crt = kp.first.at(0).toDer();
	QByteArray key = kp.second.toDer();
	QByteArray qba;

	p = reinterpret_cast<const unsigned char *>(key.constData());
	pkey = d2i_AutoPrivateKey(NULL, &p, key.length());

	if (pkey) {
		p = reinterpret_cast<const unsigned char *>(crt.constData());
		x509 = d2i_X509(NULL, &p, crt.length());

		if (x509 && X509_check_private_key(x509, pkey)) {
			X509_keyid_set1(x509, NULL, 0);
			X509_alias_set1(x509, NULL, 0);


			QList<QSslCertificate> qlCerts = kp.first;
			qlCerts.removeFirst();

			foreach(const QSslCertificate &cert, qlCerts) {
				X509 *c = NULL;
				crt = cert.toDer();
				p = reinterpret_cast<const unsigned char *>(crt.constData());

				c = d2i_X509(NULL, &p, crt.length());
				if (c)
					sk_X509_push(certs, c);
			}

			pkcs = PKCS12_create(SSL_STRING(""), SSL_STRING("Mumble Identity"), pkey, x509, certs, -1, -1, 0, 0, 0);
			if (pkcs) {
				long size;
				mem = BIO_new(BIO_s_mem());
				i2d_PKCS12_bio(mem, pkcs);
				Q_UNUSED(BIO_flush(mem));
				size = BIO_get_mem_data(mem, &data);
				qba = QByteArray(data, static_cast<int>(size));
			}
		}
	}

	if (pkey)
		EVP_PKEY_free(pkey);
	if (x509)
		X509_free(x509);
	if (pkcs)
		PKCS12_free(pkcs);
	if (mem)
		BIO_free(mem);
	if (certs)
		sk_X509_free(certs);

	return qba;
}
