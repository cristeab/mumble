#pragma once

#include "qmlhelpers.h"
#include "Settings.h"
#include <QSslCertificate>
#include <QUrl>
#include <QDir>

class CertificateModel : public QObject
{
    Q_OBJECT

    QML_WRITABLE_PROPERTY(int, pageCount, setPageCount, NEW_CERT_PAGE_COUNT)

    QML_READABLE_PROPERTY(QString, subjectName, setSubjectName, QString())
    QML_READABLE_PROPERTY(QString, subjectEmail, setSubjectEmail, QString())
    QML_READABLE_PROPERTY(QString, issuerName, setIssuerName, QString())
    QML_READABLE_PROPERTY(QString, expiry, setExpiry, QString())

    QML_WRITABLE_PROPERTY(QString, newSubjectName, setNewSubjectName, QString())
    QML_WRITABLE_PROPERTY(QString, newSubjectEmail, setNewSubjectEmail, QString())
    QML_READABLE_PROPERTY(QString, newIssuerName, setNewIssuerName, QString())
    QML_READABLE_PROPERTY(QString, newExpiry, setNewExpiry, QString())

    QML_WRITABLE_PROPERTY(QString, certFilePath, setCertFilePath, QString())
    QML_WRITABLE_PROPERTY(bool, requestPassword, setRequestPassword, false)
    QML_WRITABLE_PROPERTY(QString, certPassword, setCertPassword, QString())

public:
    enum PageCount { NEW_CERT_PAGE_COUNT = 5, IMPORT_CERT_PAGE_COUNT = 4, EXPORT_CERT_PAGE_COUNT = 2 };
    Q_ENUM(PageCount)

    explicit CertificateModel(QObject *parent = nullptr);
    Q_INVOKABLE bool generateNewCert();
    Q_INVOKABLE void toLocalFile(const QUrl &fileUrl);
    Q_INVOKABLE bool exportCert();
    Q_INVOKABLE void finish();
    Q_INVOKABLE bool importCert(bool test = false);
    Q_INVOKABLE void initializePage(int index);

signals:
    void showDialog(const QString &title, const QString &msg);

private:
    void setCert(const QList<QSslCertificate> &cert);
    Settings::KeyPair generateNewCert(const QString &name, const QString &email);
    static bool validateCert(const Settings::KeyPair &kp);
    static QByteArray exportCert(const Settings::KeyPair &kp);
    static Settings::KeyPair importCert(const QByteArray &data, const QString &pw = QString());
    void setupNewCertInfo();
    void showErrorDialog(const QString &msg) {
        emit showDialog(tr("Error"), msg);
    }

    QList<QSslCertificate> _cert;
    Settings::KeyPair _current;
    Settings::KeyPair _new;
};
