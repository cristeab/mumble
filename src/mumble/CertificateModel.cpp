#include "CertificateModel.h"
#include "SelfSignedCertificate.h"
#include "Global.h"
#include <QUrl>
#include <QQmlEngine>

CertificateModel::CertificateModel(QObject *parent) : QObject(parent)
{
    qmlRegisterType<CertificateModel>("CertificateModel", 1, 0, "CertificateModel");

    setObjectName("certModel");
    initializePage(0);

    connect(this, &CertificateModel::currentPageIndexChanged, this, [this]() {
        initializePage(_currentPageIndex);
    });
}

bool CertificateModel::generateNewCert()
{
    _new = generateNewCert(_newSubjectName, _newSubjectEmail);
    const auto rc = validateCert(_new);

    if (rc && !_new.first.isEmpty()) {
        QSslCertificate qscCert = _new.first.at(0);
        if (qscCert.expiryDate() <= QDateTime::currentDateTime()) {
            setNewExpiry(QString::fromLatin1("<font color=\"red\"><b>%1</b></font>")
                      .arg(qscCert.expiryDate().toString(Qt::SystemLocaleDate).toHtmlEscaped()));
        } else {
            setNewExpiry(qscCert.expiryDate().toString(Qt::SystemLocaleDate));
        }

        if (_cert.count() > 1) {
            qscCert = _cert.last();
        }

        const QStringList &issuerNames = qscCert.issuerInfo(QSslCertificate::CommonName);
        QString issuerName;
        if (issuerNames.count() > 0) {
            issuerName = issuerNames.at(0);
        }

        setNewIssuerName((issuerName == _newSubjectName) ? tr("Self-signed") : issuerName);
    }

    return rc;
}

void CertificateModel::setCert(const QList<QSslCertificate> &cert)
{
    _cert = cert;

    if (_cert.isEmpty()) {
        setSubjectName("");
        setSubjectEmail("");
        setIssuerName("");
        setExpiry("");
    } else {
        QSslCertificate qscCert = _cert.at(0);

        const QStringList &names = qscCert.subjectInfo(QSslCertificate::CommonName);
        QString name;
        if (names.count() > 0) {
            name = names.at(0);
        }

        QStringList emails = qscCert.subjectAlternativeNames().values(QSsl::EmailEntry);

        QString tmpName = name;
        tmpName         = tmpName.replace(QLatin1String("\\x"), QLatin1String("%"));
        tmpName         = QUrl::fromPercentEncoding(tmpName.toLatin1());
        setSubjectName(tmpName);

        if (emails.count() > 0) {
            setSubjectEmail(emails.join(QLatin1String("\n")));
        } else {
            setSubjectEmail(tr("(none)"));
        }

        if (qscCert.expiryDate() <= QDateTime::currentDateTime()) {
            setExpiry(QString::fromLatin1("<font color=\"red\"><b>%1</b></font>")
                                  .arg(qscCert.expiryDate().toString(Qt::SystemLocaleDate).toHtmlEscaped()));
        } else {
            setExpiry(qscCert.expiryDate().toString(Qt::SystemLocaleDate));
        }

        if (_cert.count() > 1) {
            qscCert = _cert.last();
        }

        const QStringList &issuerNames = qscCert.issuerInfo(QSslCertificate::CommonName);
        QString issuerName;
        if (issuerNames.count() > 0) {
            issuerName = issuerNames.at(0);
        }

        setIssuerName((issuerName == name) ? tr("Self-signed") : issuerName);
    }
}

Settings::KeyPair CertificateModel::generateNewCert(const QString &name, const QString &email)
{
    QSslCertificate qscCert;
    QSslKey qskKey;

    // Ignore return value.
    // The method sets qscCert and qskKey to null values if it fails.
    SelfSignedCertificate::generateMumbleCertificate(name, email, qscCert, qskKey);

    QList< QSslCertificate > qlCert;
    qlCert << qscCert;

    return Settings::KeyPair(qlCert, qskKey);
}

void CertificateModel::initializePage(int index)
{
    _current = _new = g.s.kpCertificate;
    switch (index) {
    case 0:
        setCert(_current.first);
        break;
    case 3:
        setCert(_new.first);
        break;
    }
}

bool CertificateModel::validateCert(const Settings::KeyPair &kp)
{
    bool valid = !kp.second.isNull() && !kp.first.isEmpty();
    for (const auto &cert: kp.first) {
        valid = valid && !cert.isNull();
    }
    return valid;
}
