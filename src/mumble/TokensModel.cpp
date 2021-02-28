#include "TokensModel.h"
#include "Database.h"
#include "ServerHandler.h"
#include "Global.h"

TokensModel::TokensModel(QObject *parent) : QAbstractListModel(parent)
{
    setObjectName("tokensModel");
}

int TokensModel::rowCount(const QModelIndex &parent) const
{
    return _tokens.size();
}

QVariant TokensModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    if (!isValidIndex(row)) {
        qCritical() << "Invalid model index";
        return QVariant();
    }
    QVariant out;
    switch (role) {
    case Qt::DisplayRole:
        //[[fallthrough]];
    case Qt::EditRole:
        out = _tokens.at(row);
        break;
    default:
        ;
    }
    return out;
}

QHash<int,QByteArray> TokensModel::roleNames() const
{
    return { { Qt::DisplayRole, "display" },
             { Qt::EditRole, "edit" } };
}

bool TokensModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole) {
        if (!checkIndex(index)) {
            return false;
        }
        const auto row = index.row();
        if (isValidIndex(row)) {
            emit layoutAboutToBeChanged();
            _tokens[row] = value.toString();
            _tokens.sort();
            emit layoutChanged();
            _isSaved = false;
            return true;
        }
    }
    return false;
}

Qt::ItemFlags TokensModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags out = QAbstractListModel::flags(index);
    out |= Qt::ItemIsEditable;
    return out;
}

void TokensModel::load()
{
    if ((nullptr == g.sh) || (nullptr == g.db)) {
        qCritical() << "Cannot load tokens";
        return;
    }
    _digest = g.sh->qbaDigest;
    emit layoutAboutToBeChanged();
    _tokens = g.db->getTokens(_digest);
    _tokens.sort();
    emit layoutChanged();
}

void TokensModel::save()
{
    if (_isSaved) {
        return;//nothing to do
    }
    if ((nullptr == g.sh) || (nullptr == g.db)) {
        qCritical() << "Cannot save tokens";
        return;
    }
    QStringList tok;
    for (const auto &it: qAsConst(_tokens)) {
        if (!it.isEmpty()) {
            tok << it;
        }
    }
    g.db->setTokens(_digest, tok);
    g.sh->setTokens(tok);
    _isSaved = true;
}

void TokensModel::add()
{
    emit layoutAboutToBeChanged();
    _tokens << "";
    emit layoutChanged();
    _isSaved = false;
}

void TokensModel::remove(int index)
{
    if (isValidIndex(index)) {
        emit layoutAboutToBeChanged();
        _tokens.removeAt(index);
        emit layoutChanged();
        _isSaved = false;
    }
}
