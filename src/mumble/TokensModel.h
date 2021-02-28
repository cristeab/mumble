#pragma once

#include <QAbstractListModel>

class TokensModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit TokensModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int,QByteArray> roleNames() const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

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
