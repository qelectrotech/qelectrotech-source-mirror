// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGEPHRASECOLLECTOR16_H
#define QET_LANGUAGEPHRASECOLLECTOR16_H
#include "languagephrases16.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../titleblocktemplate.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include <QSqlRecord>
namespace QetLanguages16 {
inline void collectProjectPhrases(QETProject *project,QVariantMap &data) {
 if(!project)return;QSet<ProjectDBModel*> models;
 for(auto *diagram:project->diagrams()) {
  const QString context=QStringLiteral("Sheet %1: table/title block").arg(diagram->title());
  for(auto *item:diagram->items())if(auto *table=dynamic_cast<QetGraphicsTableItem*>(item)) {
   auto format=table->customFormat();registerPhrase(data,format.value("title").toString(),context);
   for(const auto &h:format.value("headers").toList())registerPhrase(data,h.toString(),context);
   auto *model=qobject_cast<ProjectDBModel*>(table->model());if(!model||models.contains(model))continue;models.insert(model);
   for(int c=0;c<model->columnCount();++c)registerPhrase(data,model->headerData(c,Qt::Horizontal,SourceTextRole16).toString(),context);
   auto query=project->dataBase()->newQuery(model->queryString());const int columns=query.record().count();int rows=0;
   while(query.next()&&rows++<projectDataBase::MaxResultRows)for(int c=0;c<columns;++c)if(!protectedPhraseColumn(query.record().fieldName(c)))registerPhrase(data,query.value(c).toString(),context);
  }
  auto *t=diagram->border_and_titleblock.titleBlockTemplate();if(!t)continue;const auto sourceContext=diagram->border_and_titleblock.titleblockInformation();QSet<TitleBlockCell*> seen;
  for(int row=0;row<t->rowsCount();++row)for(int col=0;col<t->columnsCount();++col){auto *cell=t->cell(row,col);if(!cell||seen.contains(cell)||cell->type()!=TitleBlockCell::TextCell)continue;seen.insert(cell);
   if(cell->display_label)registerPhrase(data,t->languageText16(cell->label.name(),sourceContext),context);
   if(!protectedTitleValue(cell->value.name()))registerPhrase(data,t->languageText16(cell->value.name(),sourceContext),context);
  }
 }
}
}
#endif
