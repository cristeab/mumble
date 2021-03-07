#include "CertificateModel.h"
#include "Global.h"
#include <QUrl>

CertificateModel::CertificateModel(QObject *parent) : QObject(parent)
{
    setObjectName("certModel");
    initializePage(0);

    connect(this, &CertificateModel::currentPageIndexChanged, this, [this]() {
        initializePage(_currentPageIndex);
    });
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
