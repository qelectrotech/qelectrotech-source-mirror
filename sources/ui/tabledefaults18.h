// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEDEFAULTS18_H
#define QET_TABLEDEFAULTS18_H
#include <QSettings>
#include <QVariantMap>
#include <QSqlError>
#include <QSet>
#include <QVector>
#include "tablefields18.h"
#include "languagephrases16.h"
#include "../qetapp.h"
#include "../dataBase/projectdatabase.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicsheaderitem.h"
#include "../factory/ui/addtabledialog.h"

// Defaults are explicit local preferences for NEW tables. Loading a project,
// reopening a table or rendering a preview must never call applyNewTableFormat18.
namespace QetTableDefaults18 {
inline bool supportedKind18(const QString &kind) {return kind=="nomenclature"||kind=="summary";}
inline QString settingsKey18(const QString &kind) {return QStringLiteral("table18/defaults/")+kind;}
inline QStringList columnKeys18(const QAbstractItemModel *model) {
    QStringList result;if(!model)return result;
    for(int c=0;c<model->columnCount();++c) {
        QVariant raw=model->headerData(c,Qt::Horizontal,QetLanguages16::SourceTextRole16);
        result<<((raw.isValid()?raw:model->headerData(c,Qt::Horizontal)).toString());
    }
    return result;
}
inline bool validConfiguration18(const QVariantMap &value,const QString &kind,QString *error=nullptr) {
    QString message;
    if(!supportedKind18(kind))message=QObject::tr("Defaults are available for BOM and Contents tables.");
    else if(value.value("schema")!="qet-table-configuration"||value.value("version").toInt()!=1)
        message=QObject::tr("This is not a supported table configuration.");
    else if(value.value("kind").toString()!=kind)message=QObject::tr("This default belongs to a different table type.");
    else if(value.value("query").toString().trimmed().isEmpty())message=QObject::tr("Choose at least one table field before saving a default.");
    else if(projectDataBase::isReadOnlySelect(value.value("query").toString(),&message))return true;
    if(error)*error=message;return false;
}
inline QVariantMap savedConfiguration18(const QString &kind) {
    const auto value=QSettings().value(settingsKey18(kind)).toMap();
    return validConfiguration18(value,kind)?value:QVariantMap();
}
inline bool hasSavedDefault18(const QString &kind) {return !savedConfiguration18(kind).isEmpty();}
inline QVariantMap softwareFormat18(const QString &kind) {
    QFont base=QETApp::diagramTextsFont(),header=base;header.setBold(true);
    QVariantList widths,alignments,wraps;
    if(kind=="summary") {
        widths={55,280,430,110};alignments={int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter)};
        wraps={false,true,true,false};
    } else {
        widths={40,45,270,170,150,170,60};alignments={int(Qt::AlignHCenter),int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter)};
        wraps={false,false,true,true,true,true,false};
    }
    QVariantList headerAlignments;for(int c=0;c<widths.size();++c)headerAlignments<<int(Qt::AlignHCenter);
    return {{"enabled",true},{"rows",0},{"widths",widths},{"alignments",alignments},
            {"headerAlignments",headerAlignments},{"wraps",wraps},{"autoRowHeight",true},{"autoHeaderHeight",true},
            {"rowHeight",20},{"headerHeight",22},{"paddingX",3},{"paddingY",2},
            {"bodyStyle",QVariantMap{{"font",base.toString()},{"color","#ff000000"}}},
            {"headerStyle",QVariantMap{{"font",header.toString()},{"color","#ff000000"}}},
            {"headerBackground","#ffe4e4e4"},{"bodyBackground","#ffffffff"},{"gridColor","#ff000000"}};
}
inline QVariantMap softwareConfiguration18(const QString &kind) {
    if(!supportedKind18(kind))return {};
    const QStringList columns=kind=="summary"?QStringList{"Page","Page content","Comments","Date"}
            :QStringList{"No.","Qty","Name","Part number","References","Manufacturer","Unit"};
    return {{"schema","qet-table-configuration"},{"version",1},{"kind",kind},
            {"query",QetTableFields18::defaultQuery18(kind)},{"format",softwareFormat18(kind)},{"columns18",columns}};
}
inline QString preferenceKey20(const QString &kind,const QString &name){return "table20/"+kind+"/"+name;}
inline bool remember20(const QString &kind){return QSettings().value(preferenceKey20(kind,"remember"),false).toBool();}
inline void rememberFormat20(const QString &kind,const QVariantMap &format){if(!remember20(kind))return;QVariantMap last;for(const QString &key:QStringList{"titleStyle","headerStyle","bodyStyle","titleBackground","headerBackground","bodyBackground","gridColor","bodyAlignment20","headerAlignment20","titleAlignment20","paddingX","paddingY","columnResize20","rowResize20"})if(format.contains(key))last[key]=format.value(key);QSettings().setValue(preferenceKey20(kind,"last"),last);}
inline void saveFonts20(const QString &kind,const QVariantMap &format){QVariantMap fonts;for(const auto &key:{"titleStyle","headerStyle","bodyStyle"}){const auto style=format.value(key).toMap();if(style.contains("font"))fonts[key]=QVariantMap{{"font",style.value("font")}};}QSettings settings;settings.setValue(preferenceKey20(kind,"fonts"),fonts);settings.remove(preferenceKey20(kind,"last"));settings.sync();}
inline QVariantMap defaultConfiguration18(const QString &kind) {
    if(!supportedKind18(kind))return {};
    auto value=savedConfiguration18(kind);if(value.isEmpty())value=softwareConfiguration18(kind);auto format=value.value("format").toMap();
    auto fonts=QSettings().value(preferenceKey20(kind,"fonts")).toMap();for(auto it=fonts.cbegin();it!=fonts.cend();++it){auto style=format.value(it.key()).toMap();auto saved=it.value().toMap();style["font"]=saved.value("font");format[it.key()]=style;}
    if(remember20(kind)){const auto last=QSettings().value(preferenceKey20(kind,"last")).toMap();for(auto it=last.cbegin();it!=last.cend();++it)format[it.key()]=it.value();}
    value["format"]=format;return value;
}
// Capture legacy model fonts too. Otherwise saving a default from a table
// formatted through the old Display panel would silently lose its chosen font.
inline QVariantMap snapshotFormat18(QetGraphicsTableItem *table,const QAbstractItemModel *model,const QVariantMap &draftFormat) {
    if(!table||!model)return draftFormat;
    const bool custom=draftFormat.value("enabled").toBool();auto format=custom?draftFormat:QVariantMap();
    format["enabled"]=true;
    if(!custom) {
        QVariantList widths,alignments,headerAlignments;
        const int bodyAlignment=model->index(0,0).data(Qt::TextAlignmentRole).isValid()?model->index(0,0).data(Qt::TextAlignmentRole).toInt():int(Qt::AlignLeft);
        const int headerAlignment=model->headerData(0,Qt::Horizontal,Qt::TextAlignmentRole).isValid()?model->headerData(0,Qt::Horizontal,Qt::TextAlignmentRole).toInt():int(Qt::AlignHCenter);
        for(int c=0;c<model->columnCount();++c){widths<<table->headerItem()->sectionSize(c);alignments<<bodyAlignment;headerAlignments<<headerAlignment;}
        format["widths"]=widths;format["alignments"]=alignments;format["headerAlignments"]=headerAlignments;
        format["rows"]=qMax(0,table->displayNRow());format["rowHeight"]=qMax(6,table->minimumRowHeight());
        format["headerHeight"]=qMax(6,int(table->headerItem()->boundingRect().height()));
    }
    auto body=format.value("bodyStyle").toMap(),header=format.value("headerStyle").toMap();
    if(!body.contains("font"))body["font"]=(model->index(0,0).data(Qt::FontRole).isValid()?model->index(0,0).data(Qt::FontRole).value<QFont>():QETApp::diagramTextsFont()).toString();
    if(!header.contains("font"))header["font"]=(model->headerData(0,Qt::Horizontal,Qt::FontRole).isValid()?model->headerData(0,Qt::Horizontal,Qt::FontRole).value<QFont>():QETApp::diagramTextsFont()).toString();
    format["bodyStyle"]=body;format["headerStyle"]=header;return format;
}
inline bool saveDefault18(const QString &kind,const QVariantMap &value,QString *error=nullptr) {
    if(!validConfiguration18(value,kind,error))return false;
    QSettings settings;settings.setValue(settingsKey18(kind),value);settings.remove(preferenceKey20(kind,"fonts"));settings.remove(preferenceKey20(kind,"last"));settings.sync();
    if(settings.status()==QSettings::NoError)return true;
    if(error)*error=QObject::tr("Could not save the new-table default settings.");return false;
}
inline bool clearDefault18(const QString &kind,QString *error=nullptr) {
    if(!supportedKind18(kind)){if(error)*error=QObject::tr("This table type has no separate default.");return false;}
    QSettings settings;settings.remove(settingsKey18(kind));settings.remove(preferenceKey20(kind,"fonts"));settings.remove(preferenceKey20(kind,"last"));settings.sync();
    if(settings.status()==QSettings::NoError)return true;
    if(error)*error=QObject::tr("Could not restore the built-in new-table default.");return false;
}
inline QVariantMap formatForModel18(const QVariantMap &configuration,const QAbstractItemModel *model) {
    auto result=configuration.value("format").toMap();result.remove("manualCells25");result.remove("headerTexts25");const auto previous=configuration.value("columns18").toStringList(),next=columnKeys18(model);
    if(previous.isEmpty()||previous==next)return result;
    QVector<int> indices;QSet<int> used;
    for(const QString &heading:next) {
        int found=-1;for(int i=0;i<previous.size();++i)if(!used.contains(i)&&previous[i]==heading){found=i;used.insert(i);break;}indices<<found;
    }
    for(const QString &key:QStringList{"widths","headers","alignments","headerAlignments","autoWidths","wraps","overflowModes","columnOverrides17"}) {
        const auto old=result.value(key).toList();if(old.isEmpty())continue;QVariantList mapped;
        for(int c=0;c<next.size();++c) {
            const int index=indices[c];QVariant fallback;
            if(key=="widths")fallback=140;else if(key=="headers")fallback=next[c];
            else if(key=="alignments")fallback=int(Qt::AlignLeft);else if(key=="headerAlignments")fallback=int(Qt::AlignHCenter);
            else if(key=="columnOverrides17")fallback=QVariantMap();else if(key=="overflowModes")fallback="clip";else fallback=false;
            mapped<<((index>=0&&index<old.size())?old[index]:fallback);
        }result[key]=mapped;
    }
    return result;
}
inline QFont configuredFont18(const QVariantMap &format,const char *style,const QFont &fallback) {
    QFont result=fallback;const QString serialized=format.value(style).toMap().value("font").toString();
    if(!serialized.isEmpty())result.fromString(serialized);return result;
}
inline void initializeNewTableDialog18(AddTableDialog *dialog,const QString &kind) {
    if(!dialog||!supportedKind18(kind))return;
    const auto configuration=defaultConfiguration18(kind),format=configuration.value("format").toMap();
    dialog->setTableFont(configuredFont18(format,"bodyStyle",dialog->tableFont()));
    dialog->setHeaderFont(configuredFont18(format,"headerStyle",dialog->headerFont()));
    const auto alignments=format.value("alignments").toList(),headerAlignments=format.value("headerAlignments").toList();
    // Mixed per-column alignments cannot be represented by the old whole-table
    // control. Show Left and retain the individual columns unless it is changed.
    int body=int(Qt::AlignLeft),header=int(Qt::AlignHCenter);
    if(!alignments.isEmpty()) {const int first=alignments.first().toInt();bool same=true;for(const auto &v:alignments)same&=v.toInt()==first;if(same)body=first;}
    if(!headerAlignments.isEmpty())header=headerAlignments.first().toInt();
    dialog->setTableAlignment(Qt::Alignment(body));dialog->setHeaderAlignment(Qt::Alignment(header));
    const int x=format.value("paddingX",3).toInt(),y=format.value("paddingY",1).toInt();
    dialog->setTableMargins(QMargins(x,y,x,y));dialog->setHeaderMargins(QMargins(x,y,x,y));
    dialog->setProperty("newTableDefault18Configuration",configuration);
    dialog->setProperty("newTableDefault18BodyFont",dialog->tableFont().toString());
    dialog->setProperty("newTableDefault18HeaderFont",dialog->headerFont().toString());
    dialog->setProperty("newTableDefault18BodyAlignment",int(dialog->tableAlignment()));
    dialog->setProperty("newTableDefault18HeaderAlignment",int(dialog->headerAlignment()));
    dialog->setProperty("newTableDefault18PaddingX",x);dialog->setProperty("newTableDefault18PaddingY",y);
}
inline bool applyNewTableFormat18(QetGraphicsTableItem *table,const AddTableDialog *dialog=nullptr) {
    if(!table||table->property("newTableDefault18Applied").toBool())return false;
    auto *model=qobject_cast<ProjectDBModel*>(table->model());if(!model||!supportedKind18(model->identifier()))return false;
    auto configuration=dialog?dialog->property("newTableDefault18Configuration").toMap():QVariantMap();
    if(!validConfiguration18(configuration,model->identifier()))configuration=defaultConfiguration18(model->identifier());
    auto format=formatForModel18(configuration,model);
    if(dialog) {
        const bool initialized=dialog->property("newTableDefault18Configuration").isValid();
        auto body=format.value("bodyStyle").toMap(),header=format.value("headerStyle").toMap();
        if(!initialized||dialog->tableFont().toString()!=dialog->property("newTableDefault18BodyFont").toString())body["font"]=dialog->tableFont().toString();
        if(!initialized||dialog->headerFont().toString()!=dialog->property("newTableDefault18HeaderFont").toString())header["font"]=dialog->headerFont().toString();
        format["bodyStyle"]=body;format["headerStyle"]=header;
        auto align=[&](const char *key,int value,const char *initial){if(!initialized||value!=dialog->property(initial).toInt()){QVariantList a;for(int c=0;c<model->columnCount();++c)a<<value;format[key]=a;}};
        align("alignments",int(dialog->tableAlignment()),"newTableDefault18BodyAlignment");align("headerAlignments",int(dialog->headerAlignment()),"newTableDefault18HeaderAlignment");
        const auto margins=dialog->tableMargins();const int x=qMax(margins.left(),margins.right()),y=qMax(margins.top(),margins.bottom());
        if(!initialized||x!=dialog->property("newTableDefault18PaddingX").toInt())format["paddingX"]=qMax(0,x);
        if(!initialized||y!=dialog->property("newTableDefault18PaddingY").toInt())format["paddingY"]=qMax(0,y);
    }
    table->setCustomFormat(format);
    rememberFormat20(model->identifier(),format);table->setProperty("newTableDefault18Applied",true);return true;
}
}
#endif
