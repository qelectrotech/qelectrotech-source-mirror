// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLECOMBINE18_H
#define QET_TABLECOMBINE18_H
#include <QtWidgets>
#include <QSqlError>
#include "tableformat13.h"
#include "tablequery15.h"
#include "../diagram.h"
#include "../qetproject.h"
#include "../dataBase/projectdatabase.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicsheaderitem.h"
#include "../utils/qetutils.h"
#include <algorithm>
#include <cmath>

namespace QetTableCombine18 {
// Deliberately combine compatible projections of ONE source, never join
// unrelated result sets by row number. Legacy RS tables use this structure.
struct Projection {
    QString prefix,tail;
    QStringList columns;
};
inline QString comparable(QString sql) {
    QString out;QChar quote;bool space=false;
    for(int i=0;i<sql.size();++i){const QChar c=sql[i];
        if(!quote.isNull()){
            out+=c;
            if(c==quote||(quote=='['&&c==']')){
                if(i+1<sql.size()&&sql[i+1]==c&&quote!='['){out+=sql[++i];continue;}
                quote=QChar();
            }continue;
        }
        if(c.isSpace()){space=!out.isEmpty();continue;}
        if(c=='-'&&i+1<sql.size()&&sql[i+1]=='-'){
            while(i<sql.size()&&sql[i]!='\n')++i;space=!out.isEmpty();continue;
        }
        if(c=='/'&&i+1<sql.size()&&sql[i+1]=='*'){
            const int end=sql.indexOf("*/",i+2);if(end<0)return sql.trimmed();
            i=end+1;space=!out.isEmpty();continue;
        }
        if(space){out+=' ';space=false;}out+=c;
        if(c=='\''||c=='"'||c=='`'||c=='[')quote=c;
    }
    return out.trimmed();
}
inline bool parse(const QString &query,Projection &p,QString &error) {
    QString sql=QetTableQuery15::withoutState(query).trimmed();
    if(sql.endsWith(';'))sql.chop(1);
    const auto tokens=QetTableQuery15::tokens(sql);int select=-1,from=-1;
    for(const auto &token:tokens){
        if(select<0&&token.word=="SELECT"){select=token.end;p.prefix=sql.left(token.start);continue;}
        if(select>=0&&from<0&&token.word=="FROM"){from=token.start;continue;}
        if(select>=0&&QStringList{"UNION","INTERSECT","EXCEPT"}.contains(token.word)){
            error=QObject::tr("The selected queries have an outer UNION/INTERSECT/EXCEPT. Use one predefined table instead of combining these results.");return false;
        }
    }
    if(select<0||from<0){error=QObject::tr("Only tables that select columns from the same source can be combined.");return false;}
    const QString projection=sql.mid(select,from-select).trimmed();
    const auto projectionTokens=QetTableQuery15::tokens(projection);
    if(!projectionTokens.isEmpty()&&QStringList{"DISTINCT","ALL"}.contains(projectionTokens.first().word)){
        error=QObject::tr("DISTINCT/ALL projections cannot be combined safely. Use one predefined table instead.");return false;
    }
    p.columns=QetTableQuery15::splitColumns(projection);p.tail=sql.mid(from).trimmed();
    if(p.columns.isEmpty()){error=QObject::tr("A selected table has no projected columns.");return false;}
    for(const auto &column:p.columns){
        const auto base=QetTableQuery15::baseExpression(column).trimmed();
        if(base=="*"||base.endsWith(".*")){
            error=QObject::tr("Expand wildcard columns before combining these tables.");return false;
        }
    }
    // An ordinal ORDER BY changes meaning when columns are added to SELECT.
    const auto tailTokens=QetTableQuery15::tokens(p.tail);
    for(int i=0;i+1<tailTokens.size();++i)if(tailTokens[i].word=="ORDER"&&tailTokens[i+1].word=="BY"){
        const int begin=tailTokens[i+1].end;int end=p.tail.size();
        for(int j=i+2;j<tailTokens.size();++j)if(QStringList{"LIMIT","OFFSET","WINDOW"}.contains(tailTokens[j].word)){end=tailTokens[j].start;break;}
        for(const auto &part:QetTableQuery15::splitColumns(p.tail.mid(begin,end-begin)))
            if(QRegularExpression("^\\s*[0-9]+(?:\\s+(?:ASC|DESC))?\\s*$",QRegularExpression::CaseInsensitiveOption).match(part).hasMatch()){
                error=QObject::tr("An ORDER BY column number would change meaning after combining. Use named ordering or one predefined table.");return false;
            }
    }
    return true;
}
inline QVariantMap sourceFormat(QetGraphicsTableItem *table) {
    if(table->customFormatting())return table->customFormat();
    auto *model=table->model();QVariantMap f{{"enabled",true},{"rows",qMax(0,table->displayNRow())},
        {"rowHeight",20},{"headerHeight",qMax(6,int(std::round(table->headerItem()->rect().height())))},
        {"autoRowHeight",false},{"autoHeaderHeight",false}};
    const auto index=model->index(0,0);QFont body=index.data(Qt::FontRole).value<QFont>();
    if(body.pointSizeF()<=0&&body.pixelSize()<=0)body=QApplication::font();
    QFont heading=model->headerData(0,Qt::Horizontal,Qt::FontRole).value<QFont>();
    if(heading.pointSizeF()<=0&&heading.pixelSize()<=0)heading=body;
    f["bodyStyle"]=QVariantMap{{"font",body.toString()},{"color","#ff000000"}};
    f["headerStyle"]=QVariantMap{{"font",heading.toString()},{"color","#ff000000"}};
    const auto margins=QETUtils::marginsFromString(index.data(Qt::UserRole+1).toString());
    f["paddingX"]=qMax(margins.left(),margins.right());f["paddingY"]=qMax(margins.top(),margins.bottom());
    QVariantList widths,heads,align,headerAlign;
    for(int c=0;c<model->columnCount();++c){
        widths<<table->headerItem()->sectionSize(c);
        auto headingText=model->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
        heads<<(headingText.isValid()?headingText:model->headerData(c,Qt::Horizontal));
        int alignment=index.data(Qt::TextAlignmentRole).toInt();if(!(alignment&Qt::AlignHorizontal_Mask))alignment=Qt::AlignLeft;
        align<<alignment;
        int ha=model->headerData(c,Qt::Horizontal,Qt::TextAlignmentRole).toInt();if(!(ha&Qt::AlignHorizontal_Mask))ha=Qt::AlignHCenter;
        headerAlign<<ha;
    }
    f["widths"]=widths;f["headers"]=heads;f["alignments"]=align;f["headerAlignments"]=headerAlign;
    const int rows=table->displayedRowCount();
    f["rowHeight"]=rows?qMax(6,int(std::round(qreal(table->size().height())/rows))):20;
    return f;
}

class Change : public QUndoCommand {
    QPointer<Diagram> page;
    QList<QPointer<QetGraphicsTableItem>> originals;
    QList<bool> selected;
    QPointer<QetGraphicsTableItem> merged;
public:
    Change(Diagram *diagram,const QList<QetGraphicsTableItem*> &tables,QetGraphicsTableItem *result)
        :QUndoCommand(QObject::tr("Combine adjacent table columns")),page(diagram),merged(result) {
        for(auto *table:tables){originals<<table;selected<<table->isSelected();page->qgiManager().manage(table);}
        page->qgiManager().manage(result);
    }
    ~Change() override {
        if(!page)return;
        for(const auto &table:originals)if(table)page->qgiManager().release(table);
        if(merged)page->qgiManager().release(merged);
    }
    void redo() override {
        if(!page||!merged)return;page->showMe();
        for(const auto &table:originals)if(table){table->setSelected(false);page->removeItem(table);}
        page->addItem(merged);merged->setSelected(true);
    }
    void undo() override {
        if(!page||!merged)return;page->showMe();merged->setSelected(false);page->removeItem(merged);
        for(int i=0;i<originals.size();++i)if(originals[i]){page->addItem(originals[i]);originals[i]->setSelected(selected[i]);}
    }
};

inline bool combine(Diagram *page,QList<QetGraphicsTableItem*> tables,QString &error,
                    QetGraphicsTableItem **result=nullptr) {
    error.clear();if(result)*result=nullptr;
    if(!page||page->isReadOnly()||!page->project()||page->project()->isReadOnly()){
        error=QObject::tr("Choose an editable sheet before combining tables.");return false;
    }
    if(tables.size()<2){error=QObject::tr("Select at least two adjacent tables on this sheet.");return false;}
    QSet<QetGraphicsTableItem*> unique;
    for(auto *table:tables){
        if(!table||table->diagram()!=page||unique.contains(table)||!table->isMovable()){
            error=QObject::tr("All selected tables must be separate, unlocked objects on the current sheet.");return false;
        }unique.insert(table);
        if(table->parentItem()||!table->transform().isIdentity()||table->rotation()!=0||table->scale()!=1){
            error=QObject::tr("Only unrotated, unscaled table objects can be combined.");return false;
        }
        if(table->previousTable()||table->nextTable()||table->hasPendingLink()){
            error=QObject::tr("Continuation tables cannot be combined. Select independent side-by-side tables.");return false;
        }
        if(!qobject_cast<ProjectDBModel*>(table->model())){error=QObject::tr("Only live project tables can be combined.");return false;}
    }
    std::sort(tables.begin(),tables.end(),[](QetGraphicsTableItem *a,QetGraphicsTableItem *b){return a->scenePos().x()<b->scenePos().x();});
    for(int i=1;i<tables.size();++i)if(std::abs(tables[i]->scenePos().x()-(tables[i-1]->scenePos().x()+tables[i-1]->size().width()))>2){
        error=QObject::tr("Place the tables directly next to each other before combining them.");return false;
    }
    auto *first=tables.first();auto *firstModel=qobject_cast<ProjectDBModel*>(first->model());
    const QString kind=firstModel->identifier();const int rows=firstModel->rowCount();
    Projection common;QStringList combinedColumns;QList<QVariantMap> formats;
    QList<QPair<int,int>> outputColumns;QHash<QString,QString> projected;
    for(int t=0;t<tables.size();++t){auto *table=tables[t];auto *model=qobject_cast<ProjectDBModel*>(table->model());Projection p;
        if(model->project()!=page->project()||model->identifier()!=kind||model->rowCount()!=rows||table->displayNRow()!=first->displayNRow()){
            error=QObject::tr("The selected tables must have the same type, row count and displayed-row limit.");return false;
        }
        if(std::abs(table->scenePos().y()-first->scenePos().y())>1){error=QObject::tr("Place the tables side by side with their body tops aligned before combining.");return false;}
        if(!parse(model->queryString(),p,error))return false;
        if(p.columns.size()!=model->columnCount()){error=QObject::tr("The selected query columns cannot be mapped safely to their displayed columns.");return false;}
        if(t==0)common=p;
        else if(comparable(p.prefix)!=comparable(common.prefix)||comparable(p.tail)!=comparable(common.tail)){
            error=QObject::tr("These tables do not use the same source query and row ordering. Combine only matching column pieces, or insert a new complete table.");return false;
        }
        formats<<sourceFormat(table);
        if(t){const auto &left=formats.first(),&right=formats.last();
            if(left.value("rowOverrides17").toMap()!=right.value("rowOverrides17").toMap()){
                error=QObject::tr("The tables have different individual row styles. Match or reset those row overrides before combining; column styles are preserved.");return false;
            }
            for(const QString &key:QStringList{"paddingX","paddingY","gridColor"}){
                const QVariant fallback=key=="gridColor"?QVariant("#ff000000"):QVariant(key=="paddingX"?3:1);
                if(left.value(key,fallback)!=right.value(key,fallback)){
                    error=QObject::tr("The tables have different cell padding or grid colours. Match those settings before combining.");return false;
                }
            }
        }
        for(int c=0;c<p.columns.size();++c){
            const auto rawName=model->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
            const QString name=(rawName.isValid()?rawName:model->headerData(c,Qt::Horizontal)).toString();
            if(projected.contains(name)){
                if(projected.value(name)!=comparable(p.columns[c])){
                    error=QObject::tr("Two different columns use the same heading '%1'. Give them unique headings before combining.").arg(name);return false;
                }
                // The same projected column appearing twice is redundant.
                continue;
            }
            projected[name]=comparable(p.columns[c]);combinedColumns<<p.columns[c];outputColumns<<qMakePair(t,c);
        }
    }
    const QString query=common.prefix+"SELECT "+combinedColumns.join(", ")+" "+common.tail;
    if(!projectDataBase::isReadOnlySelect(query,&error))return false;
    auto actual=page->project()->dataBase()->newQuery(query,&error);
    if(!actual.isActive()||actual.lastError().isValid()){
        if(error.isEmpty())error=actual.lastError().text();return false;
    }
    int row=0;
    while(actual.next()){
        if(row>=rows){error=QObject::tr("The combined query would change the number of rows.");return false;}
        for(int c=0;c<outputColumns.size();++c){auto pair=outputColumns[c];auto *model=tables[pair.first]->model();
            if(actual.value(c).toString()!=model->index(row,pair.second).data(QetLanguages16::SourceTextRole16).toString()){
                error=QObject::tr("The selected tables do not have matching row order. Their data was not changed.");return false;
            }
        }++row;
    }
    if(row!=rows){error=QObject::tr("The combined query would change the number of rows.");return false;}

    QVariantMap format=formats.first();format["enabled"]=true;
    QVariantList widths,heads,aligns,headerAligns,wraps,autoWidths,overflow,columnOverrides;
    QVector<QetFormatting::TableLayout15> layouts;
    for(int i=0;i<tables.size();++i){auto *model=tables[i]->model();const auto fallback=model->index(0,0).data(Qt::FontRole).value<QFont>();
        layouts<<QetFormatting::tableLayout(formats[i],model,0,tables[i]->displayedRowCount(),fallback);
    }
    QString title;QVariantMap titleStyle;int titleHeight=24,headerHeight=6;
    QVariantMap rowHeights;QSet<int> manualRows;int minimumHeight=6;bool automaticRows=false;
    for(int i=0;i<tables.size();++i){const auto &f=formats[i];const auto &layout=layouts[i];headerHeight=qMax(headerHeight,layout.headerHeight);
        const QString currentTitle=f.value("title").toString();
        if(!currentTitle.isEmpty()){
            if(!title.isEmpty()&&(title!=currentTitle||titleStyle!=f.value("titleStyle").toMap())){
                error=QObject::tr("The selected tables have different full-width titles. Match or remove those titles before combining.");return false;
            }
            title=currentTitle;titleStyle=f.value("titleStyle").toMap();titleHeight=qMax(titleHeight,layout.titleHeight);
        }
        minimumHeight=qMax(minimumHeight,QetFormatting::number(f,"rowHeight",20));automaticRows|=f.value("autoRowHeight").toBool();
        const auto explicitHeights=f.value("rowHeights18").toMap();
        for(auto it=explicitHeights.cbegin();it!=explicitHeights.cend();++it){bool ok=false;const int r=it.key().toInt(&ok);if(ok&&r>=0&&it.value().toInt()>=6)manualRows.insert(r);}
        for(int r=0;r<layout.rowHeights.size();++r)rowHeights[QString::number(r)]=qMax(rowHeights.value(QString::number(r),6).toInt(),layout.rowHeights[r]);
    }
    for(const auto &pair:outputColumns){const int t=pair.first,c=pair.second;const auto &f=formats[t];auto *model=tables[t]->model();
        widths<<layouts[t].widths[c];
        const auto sourceHeads=f.value("headers").toList();auto rawName=model->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
        heads<<(c<sourceHeads.size()?sourceHeads[c]:(rawName.isValid()?rawName:model->headerData(c,Qt::Horizontal)));
        auto a=f.value("alignments").toList(),ha=f.value("headerAlignments").toList();
        aligns<<(c<a.size()?a[c]:QVariant(int(Qt::AlignLeft)));headerAligns<<(c<ha.size()?ha[c]:QVariant(int(Qt::AlignHCenter)));
        wraps<<QetFormatting::columnFlag(f,"wraps",c);autoWidths<<QetFormatting::columnFlag(f,"autoWidths",c);
        const auto modes=f.value("overflowModes").toList();overflow<<(c<modes.size()?modes[c]:QVariant("clip"));
        const QFont fallback=model->index(0,0).data(Qt::FontRole).value<QFont>();
        auto body=QetFormatting::bodyCellStyle17(f,c,-1),heading=QetFormatting::headerCellStyle17(f,c);
        if(!body.contains("font"))body["font"]=fallback.toString();
        if(!heading.contains("font"))heading["font"]=fallback.toString();
        columnOverrides<<QVariantMap{{"bodyStyle",body},{"headerStyle",heading},
            {"bodyBackground",QetFormatting::bodyCellBackground17(f,c,-1).name(QColor::HexArgb)},
            {"headerBackground",QetFormatting::headerCellBackground17(f,c).name(QColor::HexArgb)}};
    }
    format["widths"]=widths;format["headers"]=heads;format["alignments"]=aligns;format["headerAlignments"]=headerAligns;
    format["wraps"]=wraps;format["autoWidths"]=autoWidths;format["overflowModes"]=overflow;format["columnOverrides17"]=columnOverrides;
    QVariantMap explicitCombinedHeights;for(int row:manualRows)if(rowHeights.contains(QString::number(row)))explicitCombinedHeights[QString::number(row)]=rowHeights.value(QString::number(row));
    format["rows"]=qMax(0,first->displayNRow());format["rowHeight"]=minimumHeight;format["rowHeights18"]=explicitCombinedHeights;
    format["autoRowHeight"]=automaticRows;format["autoHeaderHeight"]=true;format["headerHeight"]=headerHeight;
    format["title"]=title;format["titleStyle"]=titleStyle;format["titleHeight"]=titleHeight;

    auto *table=new QetGraphicsTableItem;auto *model=new ProjectDBModel(page->project(),table);
    model->setIdentifier(kind);model->setQuery(query);table->setModel(model);
    QVariantMap edits;
    for(int t=0;t<tables.size();++t){auto old=formats[t].value("manualCells25").toMap();for(auto it=old.cbegin();it!=old.cend();++it)edits[QString::number(t)+":"+it.key()]=it.value();}
    for(int out=0;out<outputColumns.size();++out){auto pair=outputColumns[out];auto old=formats[pair.first].value("manualCells25").toMap();for(int row=0;row<model->rowCount();++row){
      auto oldKey=QetManualCells25::key(tables[pair.first]->model(),row,pair.second),newKey=QetManualCells25::key(model,row,out);
      if(!newKey.isEmpty()&&old.contains(oldKey)){edits[newKey]=old.value(oldKey);edits.remove(QString::number(pair.first)+":"+oldKey);}
    }}
    QVariantMap headerLanguages;
    for(int out=0;out<outputColumns.size();++out){auto pair=outputColumns[out];auto oldKey=QetManualCells25::headerKey(tables[pair.first]->model(),pair.second),newKey=QetManualCells25::headerKey(model,out);if(newKey.isEmpty())continue;
        auto languages=formats[pair.first].value("headerTexts25").toMap();for(auto it=languages.cbegin();it!=languages.cend();++it){auto old=it.value().toMap();if(!old.contains(oldKey))continue;auto next=headerLanguages.value(it.key()).toMap();next[newKey]=old.value(oldKey);headerLanguages[it.key()]=next;}
    }
    format["headerTexts25"]=headerLanguages;
    format["manualCells25"]=edits;table->setCustomFormat(format);
    table->setPos(first->pos());table->setZValue(first->zValue());table->setTableName(QObject::tr("Combined %1 table").arg(kind));
    page->undoStack().push(new Change(page,tables,table));if(result)*result=table;
    return true;
}
inline bool combineSelected(Diagram *page,QWidget *parent=nullptr) {
    QList<QetGraphicsTableItem*> tables;if(page)for(auto *item:page->selectedItems())if(auto *table=dynamic_cast<QetGraphicsTableItem*>(item))tables<<table;
    QString error;if(combine(page,tables,error))return true;
    QMessageBox::warning(parent,QObject::tr("Combine selected tables"),error);return false;
}
}
#endif
