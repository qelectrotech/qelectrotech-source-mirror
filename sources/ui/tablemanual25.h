// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEMANUAL25_H
#define QET_TABLEMANUAL25_H
#include <QtWidgets>
#include <QCryptographicHash>
#include "languagephrases16.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../qetproject.h"

namespace QetManualCells25 {
// Never use a row number to identify generated data. If source data no longer
// matches uniquely, keep the saved edit for review instead of guessing a row.
class Cache : public QObject {
public:
    bool valid=false;
    QStringList columns, rows;
    QHash<QString,int> columnCounts,rowCounts;
    explicit Cache(QAbstractItemModel *m):QObject(m) {
        setObjectName("manualCellCache25");
        auto invalidate=[this]{valid=false;};
        connect(m,&QAbstractItemModel::modelReset,this,invalidate);
        connect(m,&QAbstractItemModel::layoutChanged,this,invalidate);
        connect(m,&QAbstractItemModel::dataChanged,this,invalidate);
        connect(m,&QAbstractItemModel::rowsInserted,this,invalidate);
        connect(m,&QAbstractItemModel::rowsRemoved,this,invalidate);
        connect(m,&QAbstractItemModel::columnsInserted,this,invalidate);
        connect(m,&QAbstractItemModel::columnsRemoved,this,invalidate);
        connect(m,&QAbstractItemModel::headerDataChanged,this,invalidate);
    }
    void build(const QAbstractItemModel *m) {
        if(valid)return;
        columns.clear();rows.clear();columnCounts.clear();rowCounts.clear();
        auto names=m->property("qetLanguageColumns16").toStringList();
        int identity=-1;
        for(int c=0;c<m->columnCount();++c){
            auto raw=m->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
            QString name=names.value(c,(raw.isValid()?raw:m->headerData(c,Qt::Horizontal)).toString());
            columns<<name;++columnCounts[name];
            if(identity<0&&QStringList{"uuid","element_uuid","diagram_uuid"}.contains(name.toLower()))identity=c;
        }
        for(int r=0;r<m->rowCount();++r){
            QVariantMap values;
            int rowIdentity=identity;
            if(identity>=0){auto id=m->index(r,identity).data(QetLanguages16::SourceTextRole16);if(!id.isValid())id=m->index(r,identity).data();if(id.toString().trimmed().isEmpty())rowIdentity=-1;}
            for(int c=0;c<m->columnCount();++c){
                if(rowIdentity>=0&&c!=rowIdentity)continue;
                if(rowIdentity<0&&QStringList{"no.","no"}.contains(columns[c].toLower()))continue;
                auto raw=m->index(r,c).data(QetLanguages16::SourceTextRole16);
                values[columns[c]]=raw.isValid()?raw:m->index(r,c).data();
            }
            QString hash=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument::fromVariant(values).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());
            rows<<hash;++rowCounts[hash];
        }
        valid=true;
    }
};
inline Cache *cache(const QAbstractItemModel *m){
    if(!m)return nullptr;
    auto *c=dynamic_cast<Cache*>(m->findChild<QObject*>("manualCellCache25",Qt::FindDirectChildrenOnly));
    if(!c)c=new Cache(const_cast<QAbstractItemModel*>(m));
    c->build(m);return c;
}
inline QString language(const QAbstractItemModel *m){
    if(auto *db=qobject_cast<const ProjectDBModel*>(m))if(db->project()){
        auto data=db->project()->languageData();
        return data.value("activeLanguage",data.value("sourceLanguage","en")).toString();
    }
    return "en";
}
inline QString headerKey(const QAbstractItemModel *m,int col){
    if(!m||col<0||col>=m->columnCount())return {};
    auto *c=cache(m);return c->columnCounts.value(c->columns[col])==1?c->columns[col]:QString();
}
inline QString key(const QAbstractItemModel *m,int row,int col){
    if(!m||row<0||col<0||col>=m->columnCount())return {};
    auto *c=cache(m);
    if(c->columnCounts.value(c->columns[col])!=1)return {};
    QString rowKey;
    if(row>=m->rowCount())rowKey="blank:"+QString::number(row);
    else {rowKey=c->rows[row];if(c->rowCounts.value(rowKey)!=1)return {};}
    return rowKey+":"+QString::fromLatin1(c->columns[col].toUtf8().toHex())+":"+language(m);
}
inline QString text(const QVariantMap &format,const QAbstractItemModel *m,int row,int col){
    if(!m||row<0||col<0||col>=m->columnCount())return {};
    auto edits=format.value("manualCells25").toMap();
    if(!edits.isEmpty()){
        auto entry=edits.value(key(m,row,col)).toMap();
        if(entry.contains("text"))return entry.value("text").toString();
    }
    return row<m->rowCount()?m->index(row,col).data().toString():QString();
}
inline QSet<QString> currentKeys(const QAbstractItemModel *m,int displayedRows){
    QSet<QString> keys;
    if(m)for(int r=0;r<qMax(m->rowCount(),displayedRows);++r)for(int c=0;c<m->columnCount();++c){auto k=key(m,r,c);if(!k.isEmpty())keys.insert(k);}
    return keys;
}
}
#endif
