// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGEBLOCKS21_H
#define QET_LANGUAGEBLOCKS21_H
#include "languagepositions21.h"
#include "languagephrases16.h"
#include "../qetgraphicsitem/element.h"
namespace QetLanguageBlocks21 {
inline Element *owner(DiagramTextItem *text){for(auto *p=text->parentItem();p;p=p->parentItem())if(auto *e=dynamic_cast<Element*>(p))return e;return nullptr;}
inline QList<DiagramTextItem*> items(QETProject *project,const QStringList &ids){QList<DiagramTextItem*> result;for(auto *page:project->diagrams())for(auto *item:page->items())if(auto *text=dynamic_cast<DiagramTextItem*>(item))if(ids.contains(text->languageId16()))result<<text;std::sort(result.begin(),result.end(),[](auto *a,auto *b){const auto pa=a->scenePos(),pb=b->scenePos();return qAbs(pa.y()-pb.y())>1?pa.y()<pb.y():pa.x()<pb.x();});return result;}
inline QString create(QETProject *project,QVariantMap &data,const QStringList &ids,const QString &translation,QString *error=nullptr){
 auto texts=items(project,ids);auto fail=[&](const QString &message){if(error)*error=message;return QString();};
 if(texts.size()<2)return fail(QObject::tr("Select at least two descriptive text items from one symbol or sheet."));
 const QString lang=QetLanguages16::activeLanguage(data);if(lang==QetLanguages16::sourceLanguage(data))return fail(QObject::tr("Choose a translation language before creating a description block."));
 auto entries=data.value("entries").toMap();auto *leader=texts.first();QStringList members,words;QRectF bounds;
 for(auto *text:texts){auto e=entries.value(text->languageId16()).toMap();if(text->diagram()!=leader->diagram()||owner(text)!=owner(leader))return fail(QObject::tr("A description block must belong to one symbol, or to free texts on one sheet."));if(!e.value("enabled").toBool()||!e.value("block21").toString().isEmpty()||!QetLanguages16::isHumanPhrase(text->toPlainText()))return fail(QObject::tr("Choose descriptive texts without an existing block; identifiers and ratings are excluded."));members<<text->languageId16();words<<text->toPlainText().trimmed();bounds=bounds.united(text->mapRectToScene(text->boundingRect()));}
 if(translation.trimmed().isEmpty())return fail(QObject::tr("Enter the complete translated description."));const QString id="block:"+QUuid::createUuid().toString(QUuid::WithoutBraces);
 // Preserve the existing description's visual edge. Long German compound
 // words may grow into free space away from the symbol, not through its wire.
 qreal longest=60;QFontMetricsF metrics(leader->font());for(const auto &word:translation.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts))longest=qMax(longest,metrics.horizontalAdvance(word)+2*leader->document()->documentMargin());
 const qreal width=qBound<qreal>(60,qMax(bounds.width(),longest),400);QPointF topLeft=bounds.topLeft();int alignment=Qt::AlignLeft;
 if(auto *element=owner(leader)){auto body=element->mapRectToScene(element->drawingRect());if(bounds.center().x()<body.left()){alignment=Qt::AlignRight;topLeft.setX(bounds.right()-width);}else if(bounds.center().x()<=body.right()){alignment=Qt::AlignHCenter;topLeft.setX(bounds.center().x()-width/2);}}
 const QPointF origin=leader->parentItem()?leader->parentItem()->mapFromScene(topLeft):topLeft;
 QVariantMap v{{"text",translation},{"stale",false},{"width",width},{"alignment",alignment},{"side","current"},{"auto",false},{"x",origin.x()},{"y",origin.y()}};

 entries[id]=QVariantMap{{"source",words.join(' ')},{"context",QObject::tr("Description block: %1").arg(words.join(' '))},{"kind","block"},{"enabled",true},{"leader21",leader->languageId16()},{"members21",members},{"variants",QVariantMap{{lang,v}}}};
 for(const auto &member:members){auto e=entries.value(member).toMap();e["block21"]=id;entries[member]=e;}data["entries"]=entries;return id;
}
inline void refreshSources(QVariantMap &data){auto entries=data.value("entries").toMap();for(auto it=entries.begin();it!=entries.end();++it){auto block=it.value().toMap();if(block.value("kind")!="block")continue;QStringList words;for(const auto &id:block.value("members21").toStringList())words<<entries.value(id).toMap().value("source").toString().trimmed();const auto source=words.join(' ');if(source!=block.value("source").toString()){block["source"]=source;auto vs=block.value("variants").toMap();for(auto v=vs.begin();v!=vs.end();++v){auto variant=v.value().toMap();variant["stale"]=true;v.value()=variant;}block["variants"]=vs;it.value()=block;}}data["entries"]=entries;}
inline void remove(QVariantMap &data,QString id){auto entries=data.value("entries").toMap();auto entry=entries.value(id).toMap();if(entry.value("kind")!="block"){id=entry.value("block21").toString();entry=entries.value(id).toMap();}if(id.isEmpty())return;for(const auto &member:entry.value("members21").toStringList()){auto e=entries.value(member).toMap();e.remove("block21");entries[member]=e;}entries.remove(id);data["entries"]=entries;}
}
#endif
