// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEFIELDS18_H
#define QET_TABLEFIELDS18_H
#include "tablequery15.h"
#include "groupedbom13.h"

namespace QetTableFields18 {
struct Field {QString label,expression;};
inline QList<Field> fields18(const QString &kind) {
 if(kind=="summary")return {
  {QObject::tr("Page"),"folio AS \"Page\""},
  {QObject::tr("Page content"),"title AS \"Page content\""},
  {QObject::tr("Comments (blank column)"),"'' AS \"Comments\""},
  {QObject::tr("Date"),"strftime('%d.%m.%Y', date) AS \"Date\""},
  {QObject::tr("Author"),"author AS \"Author\""},
  {QObject::tr("Revision"),"indexrev AS \"Revision\""}};
 return {
  {QObject::tr("No. - sequential item number"),"ROW_NUMBER() OVER (ORDER BY diagram_position, position, label, element_uuid) AS \"No.\""},
  {QObject::tr("Qty - component quantity"),"CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity),''),'1'),',','.') AS REAL) AS \"Qty\""},
  {QObject::tr("Name / description"),"COALESCE(NULLIF(TRIM(description),''),designation,'') AS \"Name\""},
  {QObject::tr("Part number"),"COALESCE(manufacturer_reference,'') AS \"Part number\""},
  {QObject::tr("Manufacturer"),"manufacturer AS \"Manufacturer\""},
  {QObject::tr("References"),"label AS \"References\""},
  {QObject::tr("Unit"),"unity AS \"Unit\""},
  {QObject::tr("Comments / annotation"),"comment AS \"Comments\""},
  {QObject::tr("Part number from designation (legacy mapping)"),"designation AS \"Part number\""}};
}
inline void installFields18(const QString &kind,QListWidget *available,QList<QListWidgetItem*> &all) {
 const auto fields=fields18(kind);
 for(auto it=fields.crbegin();it!=fields.crend();++it){auto *item=QetTableQuery15::addItem(available,all,it->expression,it->label);const int row=available->row(item);if(row>=0){available->takeItem(row);available->insertItem(0,item);}}
}
inline QString defaultQuery18(const QString &kind) {
 if(kind=="nomenclature")return QetFormatting::groupedBomQuery(true,true,true,true);
 return QStringLiteral("SELECT folio AS \"Page\", title AS \"Page content\", '' AS \"Comments\", strftime('%d.%m.%Y', date) AS \"Date\" FROM project_summary_view ORDER BY pos");
}
inline QString componentListQuery18() {
 const auto fields=fields18("nomenclature");QStringList selected;
 for(int i:QList<int>{0,1,2,3,5,4,6})selected<<fields[i].expression;
 return "SELECT "+selected.join(", ")+" FROM element_nomenclature_view WHERE TRIM(COALESCE(description,''))<>'' OR TRIM(COALESCE(designation,''))<>'' OR TRIM(COALESCE(manufacturer_reference,''))<>'' ORDER BY diagram_position, position, label, element_uuid";
}
inline QString unquote18(QString name) {
 if(name.size()>1&&((name.front()=='"'&&name.back()=='"')||(name.front()=='\''&&name.back()=='\'')||(name.front()=='['&&name.back()==']')||(name.front()=='`'&&name.back()=='`'))){const QChar quote=name.front();name=name.mid(1,name.size()-2);if(quote!='[')name.replace(QString(2,quote),QString(quote));}return name;
}
inline QStringList outputNames18(const QString &query) {
 QStringList names;
 for(const auto &expression:QetTableQuery15::selectColumns(query)) {
  if(!QetTableQuery15::alias(expression).isEmpty()){names<<unquote18(QetTableQuery15::displayName(expression));continue;}
  // SQLite exposes a qualified field as its final column name, not t.field.
  const QRegularExpression qualified(QStringLiteral(R"rx(^(?:[A-Za-z_][A-Za-z0-9_]*|"(?:[^"]|"")+"|\[[^\]]+\]|`[^`]+`)\.(.+)$)rx"));
  QString name=expression.trimmed();auto match=qualified.match(name);
  while(match.hasMatch()){name=match.captured(1).trimmed();match=qualified.match(name);}
  names<<unquote18(name);
 }
 return names;
}
inline QStringList quotedOutputs18(const QString &query) {
 QStringList result;for(const auto &name:outputNames18(query))result<<QetTableQuery15::quotedName(name);return result;
}
inline bool canProjectOutputs18(const QString &query) {
 const auto names=outputNames18(query);QSet<QString> unique;if(names.isEmpty())return false;
 for(const auto &name:names){if(name.contains('*')||unique.contains(name.toCaseFolded()))return false;unique.insert(name.toCaseFolded());}
 return true;
}
inline QString projectOutputs18(QString source,const QStringList &expressions) {
 source=QetTableQuery15::withoutState(source).trimmed();if(source.endsWith(';'))source.chop(1);
 if(expressions==quotedOutputs18(source))return source;
 return "WITH \"qet_table_source18\" AS ("+source+") SELECT "+expressions.join(", ")+" FROM \"qet_table_source18\"";
}
}
#endif
