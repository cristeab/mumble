#pragma once

#include "qmlhelpers.h"
#include "Settings.h"
#include <QSslCertificate>

class CertificateModel : public QObject
{
    Q_OBJECT

    QML_WRITABLE_PROPERTY(int, pageCount, setPageCount, NEW_CERT_PAGE_COUNT)
    QML_WRITABLE_PROPERTY(int, currentPageIndex, setCurrentPageIndex, 0)

    QML_READABLE_PROPERTY(QString, subjectName, setSubjectName, "")
    QML_READABLE_PROPERTY(QString, subjectEmail, setSubjectEmail, "")
    QML_READABLE_PROPERTY(QString, issuerName, setIssuerName, "")
    QML_READABLE_PROPERTY(QString, expiry, setExpiry, "")

    QML_WRITABLE_PROPERTY(QString, newSubjectName, setNewSubjectName, "")
    QML_WRITABLE_PROPERTY(QString, newSubjectEmail, setNewSubjectEmail, "")
    QML_READABLE_PROPERTY(QString, newIssuerName, setNewIssuerName, "")
    QML_READABLE_PROPERTY(QString, newExpiry, setNewExpiry, "")

public:
    enum PageCount { NEW_CERT_PAGE_COUNT = 6, IMPORT_CERT_PAGE_COUNT = 4, EXPORT_CERT_PAGE_COUNT = 2 };
    Q_ENUM(PageCount)

    explicit CertificateModel(QObject *parent = nullptr);
    Q_INVOKABLE bool generateNewCert();

private:

    void initializePage(int index);
    void setCert(const QList<QSslCertificate> &cert);
    Settings::KeyPair generateNewCert(const QString &name, const QString &email);
    static bool validateCert(const Settings::KeyPair &kp);

    QList<QSslCertificate> _cert;
    Settings::KeyPair _current;
    Settings::KeyPair _new;
};
