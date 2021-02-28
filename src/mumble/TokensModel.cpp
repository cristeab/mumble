#include "TokensModel.h"
#include "Database.h"
#include "ServerHandler.h"
#include "Global.h"

TokensModel::TokensModel(QObject *parent) : QAbstractListModel(parent)
{
    setObjectName("tokensModel");
}

int TokensModel::rowCount(const QModelIndex& /*parent*/) const
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
    case Name:
        out = _tokens.at(row);
        break;
    default:
        ;
    }
    return out;
}

QHash<int,QByteArray> TokensModel::roleNames() const
{
    return { { Name, "name" } };
}

void TokensModel::load()
{
    if ((nullptr == g.sh) || (nullptr == g.db)) {
        qWarning() << "Cannot load tokens";
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

void TokensModel::remove(int index)
{
    if (isValidIndex(index)) {
        emit layoutAboutToBeChanged();
        _tokens.removeAt(index);
        emit layoutChanged();
        _isSaved = false;
    }
}

void TokensModel::setCurrentToken(const QString &token)
{
    if (isValidIndex(_currentIndex)) {
        emit layoutAboutToBeChanged();
        _tokens[_currentIndex] = token;
        _tokens.sort();
        emit layoutChanged();
        setCurrentIndex(INVALID_INDEX);
        _isSaved = false;
    } else if (INVALID_INDEX == _currentIndex) {
        emit layoutAboutToBeChanged();
        _tokens << token;
        _tokens.sort();
        emit layoutChanged();
        setCurrentIndex(INVALID_INDEX);
        _isSaved = false;
    }
}
