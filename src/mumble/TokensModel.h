#pragma once

#include "qmlhelpers.h"
#include <QAbstractListModel>

class TokensModel : public QAbstractListModel
{
    Q_OBJECT
    QML_WRITABLE_PROPERTY(int, currentIndex, setCurrentIndex, 0)
    QML_WRITABLE_PROPERTY(int, currentEditIndex, setCurrentEditIndex, -1)

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
    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int index);

private:
    bool isValidIndex(int index) const {
        return (0 <= index) && (index < _tokens.size());
    }
    QStringList _tokens;
    bool _isSaved = true;
    QByteArray _digest;
};
