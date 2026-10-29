// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGEPOSITIONS21_H
#define QET_LANGUAGEPOSITIONS21_H
#include "languagecatalog16.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../qetgraphicsitem/diagramtextitem.h"
namespace QetLanguagePositions21 {
inline QVariantMap capture(QETProject *project,QVariantMap data){
 if(!project||data.isEmpty())return data;auto entries=data.value("entries").toMap();const auto lang=QetLanguages16::activeLanguage(data);bool source=lang==QetLanguages16::sourceLanguage(data);
 for(auto *folio:project->builtFolios())for(auto *item:folio->items())if(auto *text=dynamic_cast<DiagramTextItem*>(item)){
  QString id=text->languageId16();if(!entries.contains(id)||QetLanguages16::hiddenMember21(data,id))continue;auto member=entries.value(id).toMap();
  if(source||!member.value("enabled").toBool()){member["baseX"]=text->pos().x();member["baseY"]=text->pos().y();entries[id]=member;continue;}
  auto effective=QetLanguages16::effectiveId21(data,id);auto e=entries.value(effective).toMap();auto variants=e.value("variants").toMap();auto v=variants.value(lang).toMap();
  QPointF expected(v.value("x",member.value("baseX")).toDouble(),v.value("y",member.value("baseY")).toDouble());if(QLineF(expected,text->pos()).length()>0.01){v["x"]=text->pos().x();v["y"]=text->pos().y();v["locked"]=true;variants[lang]=v;e["variants"]=variants;entries[effective]=e;}
 }
 data["entries"]=entries;return data;
}
inline void restore(QETProject *project,Diagram *only=nullptr){if(!project)return;const auto data=project->languageData();if(data.isEmpty())return;const bool source=QetLanguages16::activeLanguage(data)==QetLanguages16::sourceLanguage(data);
 for(auto *folio:(only?QList<Diagram*>{only}:project->builtFolios()))for(auto *item:folio->items())if(auto *text=dynamic_cast<DiagramTextItem*>(item)){auto e=QetLanguages16::entry(data,text->languageId16());if(!e.contains("baseX")||!e.value("enabled").toBool())continue;auto v=QetLanguages16::variant(data,QetLanguages16::effectiveId21(data,text->languageId16()));
  text->setPos(source?e.value("baseX").toDouble():v.value("x",e.value("baseX")).toDouble(),source?e.value("baseY").toDouble():v.value("y",e.value("baseY")).toDouble());}
}
}
#endif
