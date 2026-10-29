// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEFIELDS19_H
#define QET_TABLEFIELDS19_H
#include "tablefields18.h"
#include <QCryptographicHash>
namespace QetTableFields19 {
inline QString semanticId(const QString &kind,const QString &expression) {
 QString base=QetTableQuery15::baseExpression(expression).trimmed(),plain=base;
 if(plain.startsWith('"')&&plain.endsWith('"'))plain=plain.mid(1,plain.size()-2);
 const auto lower=plain.toLower();
 if(lower.contains("row_number()")||lower=="no.")return kind+":number";
 if(kind=="summary") {
  if(lower=="author")return "summary:author";
  if(lower=="indexrev"||lower=="revision")return "summary:revision";
  if(lower=="folio"||lower=="page")return "summary:page";
  if(lower=="title"||lower=="page content")return "summary:content";
  if(lower=="date"||(lower.startsWith("strftime(")&&lower.contains("date")))return "summary:date";
  if(lower=="''"&&QetTableQuery15::displayName(expression)=="Comments")return "summary:comments";
 }else {
  if(lower=="label"||lower=="references")return "nomenclature:references";
  if(lower=="manufacturer"||lower=="coalesce(manufacturer,'')")return "nomenclature:manufacturer";
  if(lower=="manufacturer_reference"||lower=="coalesce(manufacturer_reference,'')"||lower=="part number")return "nomenclature:article";
  if(lower=="quantity"||lower=="qty"||lower.startsWith("cast(replace(coalesce(nullif(trim(quantity)"))return "nomenclature:quantity";
  if(lower=="unity"||lower=="unit")return "nomenclature:unit";
  if(lower=="name"||lower=="coalesce(nullif(trim(description),''),designation,'')")return "nomenclature:name";
 }
 return kind+":"+QString::fromLatin1(QCryptographicHash::hash(base.toUtf8(),QCryptographicHash::Sha256).toHex());
}
inline void deduplicate(const QString &kind,QListWidget *available,QListWidget *chosen,QList<QListWidgetItem*> &all) {
 QSet<QString> ids;
 auto identity=[&](QListWidgetItem *item){auto id=item->data(Qt::UserRole+19).toString();if(id.isEmpty()){id=semanticId(kind,item->data(Qt::UserRole).toString());item->setData(Qt::UserRole+19,id);}return id;};
 for(int i=0;i<chosen->count();++i)ids.insert(identity(chosen->item(i)));
 // Prefer the underlying field over an aliased spelling in the catalogue;
 // headings and formats are independent choices, and old saved raw keys remain valid.
 QHash<QString,QListWidgetItem*> preferred;QHash<QString,QString> friendly;
 for(int i=0;i<available->count();++i){auto *item=available->item(i);const auto id=identity(item);if(!preferred.contains(id)){preferred[id]=item;friendly[id]=item->text();}const auto expr=item->data(Qt::UserRole).toString();if(QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$").match(expr).hasMatch())preferred[id]=item;}
 for(int i=0;i<available->count();) {
  auto *item=available->item(i);auto id=identity(item);
  if(ids.contains(id)||preferred.value(id)!=item){all.removeAll(item);delete available->takeItem(i);}else{item->setText(friendly.value(id,item->text()));++i;}
 }
}

inline QStringList ids(const QString &kind,const QStringList &expressions){QStringList result;for(const auto &e:expressions)result<<semanticId(kind,e);return result;}
inline QString numberExpression(const QString &kind,int start) {
 const QString order=kind=="summary"?"pos":"diagram_position, position, label, element_uuid";
 return QString("(ROW_NUMBER() OVER (ORDER BY %1) + %2) AS \"No.\"").arg(order).arg(qMax(1,start)-1);
}
inline QStringList formatSelected(const QString &kind,QStringList expressions,int start,const QString &dateFormat=QString(),const QStringList &stableIds=QStringList()) {
 int index=0;for(auto &e:expressions) {
  auto id=index<stableIds.size()?stableIds[index]:semanticId(kind,e);++index;auto heading=QetTableQuery15::alias(e);
  if(id==kind+":number"){e=QetTableQuery15::baseExpression(numberExpression(kind,start))+" AS "+(heading.isEmpty()?QStringLiteral("\"No.\""):heading);}
  else if(kind=="summary"&&id=="summary:date"&&!dateFormat.isEmpty()) {QString format=dateFormat;format.replace('\'',"''");e="strftime('"+format+"', date) AS "+(heading.isEmpty()?QStringLiteral("\"Date\""):heading);}
 }
 return expressions;
}
inline QString condition(const QString &expression,int mode,QString value) {
 value.replace('\'',"''");
 switch(mode){case 1:return "COALESCE("+expression+",'')<>''";case 2:return "COALESCE("+expression+",'')=''";case 3:return expression+" LIKE '%"+value+"%'";case 4:return expression+" NOT LIKE '%"+value+"%'";case 5:return expression+"='"+value+"'";case 6:return expression+"<>'"+value+"'";default:return {};}
}
inline QString articleExpression(const QString &expression,const QString &suffix) {
 if(suffix.isEmpty())return expression;
 const QStringList keys{"manufacturer","manufacturer_reference","designation","description","unity","quantity"};QString result;
 for(int i=0;i<expression.size();) {
  const QChar c=expression[i];
  if(c=='\'') {int begin=i++;while(i<expression.size()){if(expression[i++]=='\''){if(i<expression.size()&&expression[i]=='\''){++i;continue;}break;}}result+=expression.mid(begin,i-begin);continue;}
  if(c=='"'||c=='`'||c=='[') {const QChar end=c=='['?']':c;int begin=i++;QString name;while(i<expression.size()&&expression[i]!=end)name+=expression[i++];if(i<expression.size())++i;if(keys.contains(name.toLower()))result+=QString(c)+name+suffix+end;else result+=expression.mid(begin,i-begin);continue;}
  if(c.isLetter()||c=='_'){int begin=i++;while(i<expression.size()&&(expression[i].isLetterOrNumber()||expression[i]=='_'))++i;QString name=expression.mid(begin,i-begin);result+=name+(keys.contains(name.toLower())?suffix:QString());continue;}
  result+=c;++i;
 }
 return result;
}
inline QString groupedQuery(const QVariantMap &options,const QString &filter,const QVariantList &extras,int start) {
 const bool sum=options.value("qty",true).toBool(),refs=options.value("refs",true).toBool(),seq=options.value("seq",true).toBool(),aux=options.value("aux",false).toBool(),data=options.value("data",true).toBool();
 if(filter.trimmed().isEmpty()&&extras.isEmpty()&&start==1)return QetFormatting::groupedBomQuery(sum,refs,seq,aux,data);
 auto article=[&](const QString &suffix){
  QString query=QString("SELECT COALESCE(manufacturer%1,'') AS manufacturer, COALESCE(manufacturer_reference%1,'') AS article, COALESCE(NULLIF(TRIM(description%1),''),designation%1,'') AS name, label, COALESCE(unity%1,'') AS unit, CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity%1),''),'1'),',','.') AS REAL) AS qty, COALESCE(NULLIF(element_uuid,''),'row:'||CAST(ROW_NUMBER() OVER () AS TEXT))||':%1' AS itemkey").arg(suffix);
  int i=0;for(const auto &v:extras){auto field=v.toMap();QString expression=field.value("key").toString();if(!suffix.isEmpty()&&QStringList{"manufacturer","manufacturer_reference","designation","description","unity","quantity"}.contains(expression))expression+=suffix;query+=", "+expression+" AS extra19_"+QString::number(i++);}
  query+=" FROM element_nomenclature_view";QStringList predicates;
  if(data||!suffix.isEmpty())predicates<<QString("(TRIM(COALESCE(manufacturer_reference%1,''))<>'' OR TRIM(COALESCE(designation%1,''))<>'' OR TRIM(COALESCE(description%1,''))<>'')").arg(suffix);
  if(!filter.trimmed().isEmpty())predicates<<"("+articleExpression(filter,suffix)+")";
  if(!predicates.isEmpty())query+=" WHERE "+predicates.join(" AND ");return query;
 };
 QString source=article("");if(aux)for(int i=1;i<=4;++i)source+=" UNION ALL "+article("_auxiliary"+QString::number(i));
 QString grouped="SELECT manufacturer, article, MIN(name) AS name, unit, "+QString(sum?"SUM(qty)":"COUNT(*)")+" AS qty, GROUP_CONCAT(DISTINCT label) AS refs";
 QStringList split;
 int i=0;for(const auto &v:extras){auto field=v.toMap();QString key="extra19_"+QString::number(i++),mode=field.value("mode","distinct").toString(),expression;
  if(mode=="split"){expression=key;split<<key;}
  else if(mode=="common")expression="CASE WHEN COUNT(DISTINCT COALESCE(CAST("+key+" AS TEXT),''))<=1 THEN MIN("+key+") ELSE '[multiple values]' END";
  else expression="GROUP_CONCAT(DISTINCT "+key+")";
  grouped+=", "+expression+" AS "+key;
 }
 grouped+=" FROM ("+source+") GROUP BY manufacturer,article,unit,CASE WHEN TRIM(article)='' THEN COALESCE(label,'')||itemkey ELSE '' END";if(!split.isEmpty())grouped+=","+split.join(',');
 QString ordering="manufacturer,article,name,refs";for(const auto &field:split)ordering+=","+field;
 QString fields=seq?QString("(ROW_NUMBER() OVER (ORDER BY %1) + %2) AS \"No.\", ").arg(ordering).arg(qMax(1,start)-1):QString();
 fields+="qty AS \"Qty\",name AS \"Name\",article AS \"Part number\"";if(refs)fields+=",refs AS \"References\"";fields+=",manufacturer AS \"Manufacturer\",unit AS \"Unit\"";
 i=0;for(const auto &v:extras)fields+=", extra19_"+QString::number(i++)+" AS "+QetTableQuery15::quotedName(v.toMap().value("heading").toString());
 return QString("SELECT /*qet-bom13 qty=%1 refs=%2 seq=%3 aux=%4 data=%5*/ ").arg(sum?1:0).arg(refs?1:0).arg(seq?1:0).arg(aux?1:0).arg(data?1:0)+fields+" FROM ("+grouped+") ORDER BY "+ordering;
}
inline QString category(const QString &kind,QListWidgetItem *item) {
 const QString id=item->data(Qt::UserRole+19).toString(),expression=QetTableQuery15::baseExpression(item->data(Qt::UserRole).toString());
 if(id.startsWith("custom:"))return "calculated";
 if(expression.contains("_auxiliary",Qt::CaseInsensitive))return "auxiliary";
 const QStringList common=kind=="summary"?QStringList{"summary:page","summary:content","summary:comments","summary:date","summary:author","summary:revision","summary:number"}:QStringList{"nomenclature:number","nomenclature:quantity","nomenclature:name","nomenclature:article","nomenclature:manufacturer","nomenclature:references","nomenclature:unit"};
 if(common.contains(id))return "common";
 const QString field=QetTableFields18::unquote18(expression).toLower();
 if(kind=="summary"||QStringList{"title","folio","diagram_position","position","plant","location","locmach"}.contains(field))return "sheet";
 return "component";
}
inline QVariantMap groupedOptions(const QString &sql) {
 QVariantMap result;for(const auto &key:QStringList{"qty","refs","seq","aux","data"}) {auto m=QRegularExpression("\\b"+key+"=([01])").match(sql);result[key]=m.hasMatch()?m.captured(1)=="1":key!="aux";}return result;
}
}
#endif
