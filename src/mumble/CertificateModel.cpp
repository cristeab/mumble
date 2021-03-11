#include "CertificateModel.h"
#include "SelfSignedCertificate.h"
#include <QUrl>
#include <QQmlEngine>
#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/x509.h>

#include "Global.h"

#define SSL_STRING(x) QString::fromLatin1(x).toUtf8().data()

CertificateModel::CertificateModel(QObject *parent) : QObject(parent)
{
    qmlRegisterType<CertificateModel>("CertificateModel", 1, 0, "CertificateModel");

    setObjectName("certModel");
    initializePage(0);
}

void CertificateModel::toLocalFile(const QUrl &fileUrl)
{
    const auto filePath = fileUrl.isLocalFile() ? fileUrl.toLocalFile() : fileUrl.toString();
    setCertFilePath(QDir::toNativeSeparators(filePath));
}

bool CertificateModel::generateNewCert()
{
    qDebug() << "generateNewCert";
    _new = generateNewCert(_newSubjectName, _newSubjectEmail);
    const auto rc = validateCert(_new);

    if (rc) {
        setupNewCertInfo();
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
    case 1:
        setupNewCertInfo();
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

bool CertificateModel::exportCert()
{
    qDebug() << "exportCert";
    if (_certFilePath.isEmpty()) {
        emit showErrorDialog(tr("Empty file path. Please choose a file path"));
        return false;
    }
    const auto qba = exportCert(_new);
    if (qba.isEmpty()) {
        emit showErrorDialog(tr("Your certificate and key could not be exported to PKCS#12 format. There might be an error in your certificate."));
        return false;
    }
    QFile f(_certFilePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Unbuffered)) {
        emit showErrorDialog(tr("The file could not be opened for writing. Please use another file."));
        return false;
    }
    if (!f.setPermissions(QFile::ReadOwner | QFile::WriteOwner)) {
        emit showErrorDialog(tr("The file's permissions could not be set. No certificate and key has been written. Please use another file."));
        return false;
    }
    qint64 written = f.write(qba);
    f.close();
    if (written != qba.length()) {
        emit showErrorDialog(tr("The file could not be written successfully. Please use another file."));
        return false;
    }
    return true;
}

QByteArray CertificateModel::exportCert(const Settings::KeyPair &kp)
{
    X509 *x509            = nullptr;
    EVP_PKEY *pkey        = nullptr;
    PKCS12 *pkcs          = nullptr;
    BIO *mem              = nullptr;
    STACK_OF(X509) *certs = sk_X509_new_null();
    const unsigned char *p;
    char *data = nullptr;

    if (kp.first.isEmpty())
        return QByteArray();

    QByteArray crt = kp.first.at(0).toDer();
    QByteArray key = kp.second.toDer();
    QByteArray qba;

    p    = reinterpret_cast< const unsigned char * >(key.constData());
    pkey = d2i_AutoPrivateKey(nullptr, &p, key.length());

    if (pkey) {
        p    = reinterpret_cast< const unsigned char * >(crt.constData());
        x509 = d2i_X509(nullptr, &p, crt.length());

        if (x509 && X509_check_private_key(x509, pkey)) {
            X509_keyid_set1(x509, nullptr, 0);
            X509_alias_set1(x509, nullptr, 0);


            QList< QSslCertificate > qlCerts = kp.first;
            qlCerts.removeFirst();

            foreach (const QSslCertificate &cert, qlCerts) {
                X509 *c = nullptr;
                crt     = cert.toDer();
                p       = reinterpret_cast< const unsigned char * >(crt.constData());

                c = d2i_X509(nullptr, &p, crt.length());
                if (c)
                    sk_X509_push(certs, c);
            }

            pkcs = PKCS12_create(SSL_STRING(""), SSL_STRING("Bubbles Identity"), pkey, x509, certs, -1, -1, 0, 0, 0);
            if (pkcs) {
                long size;
                mem = BIO_new(BIO_s_mem());
                i2d_PKCS12_bio(mem, pkcs);
                Q_UNUSED(BIO_flush(mem));
                size = BIO_get_mem_data(mem, &data);
                qba  = QByteArray(data, static_cast< int >(size));
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

void CertificateModel::finish()
{
    qDebug() << "Finish";
    g.s.kpCertificate = _new;
    initializePage(0);
}

bool CertificateModel::importCert(bool test)
{
    qDebug() << "importCert";
    if (!test && _certFilePath.isEmpty()) {
        emit showErrorDialog(tr("Empty file path. Please choose a file path"));
        return false;
    }
    QFile f(_certFilePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
        if (!test) {
            emit showErrorDialog(tr("The file could not be opened for reading. Please use another file."));
            return false;
        } else {
            return true;
        }
    }
    QByteArray qba = f.readAll();
    f.close();
    if (qba.isEmpty()) {
        if (!test) {
            emit showErrorDialog(tr("The file is empty or could not be read. Please use another file."));
            return false;
        } else {
            return true;
        }
    }
    const auto imp = importCert(qba, _certPassword);
    if (!validateCert(imp)) {
        if (!test) {
            emit showErrorDialog(tr("The file did not contain a valid certificate and key. Please use another file."));
        }
        return false;
    }
    if (!test) {
        _new = imp;
        setupNewCertInfo();
    }
    return true;
}

Settings::KeyPair CertificateModel::importCert(const QByteArray &data, const QString &pw)
{
    X509 *x509            = nullptr;
    EVP_PKEY *pkey        = nullptr;
    PKCS12 *pkcs          = nullptr;
    BIO *mem              = nullptr;
    STACK_OF(X509) *certs = nullptr;
    Settings::KeyPair kp;
    int ret = 0;

    mem = BIO_new_mem_buf(data.data(), data.size());
    Q_UNUSED(BIO_set_close(mem, BIO_NOCLOSE));
    pkcs = d2i_PKCS12_bio(mem, nullptr);
    if (pkcs) {
        ret = PKCS12_parse(pkcs, nullptr, &pkey, &x509, &certs);
        if (pkcs && !pkey && !x509 && !pw.isEmpty()) {
            if (certs) {
                if (ret)
                    sk_X509_free(certs);
                certs = nullptr;
            }
            ret = PKCS12_parse(pkcs, pw.toUtf8().constData(), &pkey, &x509, &certs);
        }
        if (pkey && x509 && X509_check_private_key(x509, pkey)) {
            unsigned char *dptr;
            QByteArray key, crt;

            key.resize(i2d_PrivateKey(pkey, nullptr));
            dptr = reinterpret_cast< unsigned char * >(key.data());
            i2d_PrivateKey(pkey, &dptr);

            crt.resize(i2d_X509(x509, nullptr));
            dptr = reinterpret_cast< unsigned char * >(crt.data());
            i2d_X509(x509, &dptr);

            QSslCertificate qscCert = QSslCertificate(crt, QSsl::Der);
            QSslKey qskKey          = QSslKey(key, QSsl::Rsa, QSsl::Der);

            QList< QSslCertificate > qlCerts;
            qlCerts << qscCert;

            if (certs) {
                for (int i = 0; i < sk_X509_num(certs); ++i) {
                    X509 *c = sk_X509_value(certs, i);

                    crt.resize(i2d_X509(c, nullptr));
                    dptr = reinterpret_cast< unsigned char * >(crt.data());
                    i2d_X509(c, &dptr);

                    QSslCertificate cert = QSslCertificate(crt, QSsl::Der);
                    qlCerts << cert;
                }
            }
            bool valid = !qskKey.isNull();
            foreach (const QSslCertificate &cert, qlCerts)
                valid = valid && !cert.isNull();
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

void CertificateModel::setupNewCertInfo()
{
    if (_new.first.isEmpty()) {
        setNewSubjectName("");
        setNewSubjectEmail("");
        setNewExpiry("");
        setNewIssuerName("");
    } else {
        QSslCertificate qscCert = _new.first.at(0);

        const QStringList &names = qscCert.subjectInfo(QSslCertificate::CommonName);
        QString name;
        if (names.count() > 0) {
            name = names.at(0);
        }

        QStringList emails = qscCert.subjectAlternativeNames().values(QSsl::EmailEntry);

        QString tmpName = name;
        tmpName         = tmpName.replace(QLatin1String("\\x"), QLatin1String("%"));
        tmpName         = QUrl::fromPercentEncoding(tmpName.toLatin1());
        setNewSubjectName(tmpName);

        if (emails.count() > 0) {
            setNewSubjectEmail(emails.join(QLatin1String("\n")));
        } else {
            setNewSubjectEmail(tr("(none)"));
        }

        if (qscCert.expiryDate() <= QDateTime::currentDateTime()) {
            setNewExpiry(QString::fromLatin1("<font color=\"red\"><b>%1</b></font>")
                         .arg(qscCert.expiryDate().toString(Qt::SystemLocaleDate).toHtmlEscaped()));
        } else {
            setNewExpiry(qscCert.expiryDate().toString(Qt::SystemLocaleDate));
        }

        if (_new.first.count() > 1) {
            qscCert = _new.first.last();
        }

        const QStringList &issuerNames = qscCert.issuerInfo(QSslCertificate::CommonName);
        QString issuerName;
        if (issuerNames.count() > 0) {
            issuerName = issuerNames.at(0);
        }

        setNewIssuerName((issuerName == _newSubjectName) ? tr("Self-signed") : issuerName);
    }
}
