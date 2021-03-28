#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>

class TokensModel : public QAbstractListModel
{
    Q_OBJECT
    QML_WRITABLE_PROPERTY(int, currentIndex, setCurrentIndex, INVALID_INDEX)

public:
    enum CallHistoryRoles {
        Name= Qt::UserRole+1
    };

    explicit TokensModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    Q_INVOKABLE void load();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE QString currentToken() const {
        return isValidIndex(_currentIndex) ? _tokens.at(_currentIndex) : QString();
    }
    Q_INVOKABLE void setCurrentToken(const QString &token);

private:
    enum { INVALID_INDEX = -1 };
    bool isValidIndex(int index) const {
        return (0 <= index) && (index < _tokens.size());
    }
    void save();
    QStringList _tokens;
};
