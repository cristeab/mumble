#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>

class TokensModel : public QAbstractListModel
{
    Q_OBJECT
    QML_WRITABLE_PROPERTY(int, currentIndex, setCurrentIndex, 0)

public:
    enum CallHistoryRoles {
        Name= Qt::UserRole+1
    };

    explicit TokensModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE QString currentToken() const {
        return isValidIndex(_currentIndex) ? _tokens.at(_currentIndex) : "";
    }
    Q_INVOKABLE void setCurrentToken(const QString &token);

private:
    bool isValidIndex(int index) const {
        return (0 <= index) && (index < _tokens.size());
    }
    QStringList _tokens;
    bool _isSaved = true;
    QByteArray _digest;
};
