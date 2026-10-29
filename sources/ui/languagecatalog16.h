// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGECATALOG16_H
#define QET_LANGUAGECATALOG16_H
#include <QVariantMap>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QDomDocument>
#include <QStringDecoder>
#include <algorithm>

namespace QetLanguages16 {
inline QString sourceLanguage(const QVariantMap &data) {return data.value("sourceLanguage","en").toString();}
inline QString activeLanguage(const QVariantMap &data) {return data.value("activeLanguage",sourceLanguage(data)).toString();}
inline bool validLanguageCode(const QString &code) {
 static const QRegularExpression re("^[A-Za-z]{2,8}(?:-[A-Za-z0-9]{1,8})*$");return re.match(code).hasMatch();
}
inline QStringList languageCodes(const QVariantMap &data) {
 QStringList codes=data.value("languages").toStringList();const QString source=sourceLanguage(data);
 codes.removeAll(source);codes.prepend(source);codes.removeDuplicates();return codes;
}
inline QVariantMap entry(const QVariantMap &data,const QString &id) {return data.value("entries").toMap().value(id).toMap();}
// A block is a reviewed set of existing text IDs. The original texts remain
// independently editable in the source language; only translated display joins.
inline QString blockId21(const QVariantMap &data,const QString &id,const QString &language=QString()){
 const QString code=language.isEmpty()?activeLanguage(data):language;
 if(code==sourceLanguage(data))return {};
 auto member=entry(data,id);const QString bid=member.value("block21").toString();const auto block=entry(data,bid);
 auto v=block.value("variants").toMap().value(code).toMap();
 return block.value("enabled").toBool()&&!v.value("stale").toBool()&&!v.value("text").toString().isEmpty()?bid:QString();
}
inline QString effectiveId21(const QVariantMap &data,const QString &id){auto block=blockId21(data,id);return block.isEmpty()?id:block;}
inline bool hiddenMember21(const QVariantMap &data,const QString &id){auto block=blockId21(data,id);return !block.isEmpty()&&entry(data,block).value("leader21").toString()!=id;}
inline QVariantMap variant(const QVariantMap &data,const QString &id,const QString &language=QString()) {
 return entry(data,id).value("variants").toMap().value(language.isEmpty()?activeLanguage(data):language).toMap();
}
inline QString resolvedText(const QVariantMap &data,const QString &id,const QString &source) {
 auto e=entry(data,id);if(e.isEmpty()||!e.value("enabled",false).toBool()||activeLanguage(data)==sourceLanguage(data))return source;
 const auto v=variant(data,id);const QString text=v.value("text").toString();return text.isEmpty()||v.value("stale").toBool()?source:text;
}
inline bool hasTranslation(const QVariantMap &data,const QString &id,const QString &language=QString()) {
 const QString lang=language.isEmpty()?activeLanguage(data):language;const auto v=variant(data,id,lang);return lang==sourceLanguage(data)||(!v.value("stale").toBool()&&!v.value("text").toString().isEmpty());
}
inline QStringList placeholders(const QString &text) {
 static const QRegularExpression re("%\\{[^}]+\\}|%[A-Za-z_][A-Za-z0-9_]*|\\{[A-Za-z_][A-Za-z0-9_]*\\}");
 QStringList result;auto matches=re.globalMatch(text);while(matches.hasNext())result<<matches.next().captured();std::sort(result.begin(),result.end());return result;
}
inline QString csvCell(QString value) {value.replace('"',"\"\"");return '"'+value+'"';}
inline QByteArray exportCsv(const QVariantMap &data,bool completeBlocks21=false) {
 const auto languages=languageCodes(data);QStringList header{"ID","Context","Source"};header<<languages;QString output;for(int i=0;i<header.size();++i){if(i)output+=',';output+=csvCell(header[i]);}output+="\r\n";
 const auto entries=data.value("entries").toMap();for(auto it=entries.cbegin();it!=entries.cend();++it){auto e=it.value().toMap();if(!e.value("enabled",false).toBool())continue;if(completeBlocks21&&!e.value("block21").toString().isEmpty()&&entry(data,e.value("block21").toString()).value("enabled").toBool())continue;QStringList row{it.key(),e.value("context").toString(),e.value("source").toString()};for(const auto &language:languages)row<<(language==sourceLanguage(data)?e.value("source").toString():e.value("variants").toMap().value(language).toMap().value("text").toString());for(int i=0;i<row.size();++i){if(i)output+=',';output+=csvCell(row[i]);}output+="\r\n";}
 return QByteArray("\xEF\xBB\xBF")+output.toUtf8();
}
// RFC 4180 parser, with semicolon-delimited Excel exports accepted too. Quoted
// newlines and escaped quotes are kept verbatim. Malformed input is rejected.
inline bool parseCsv(QString input,QList<QStringList> &rows,QString *error=nullptr) {
 rows.clear();if(input.startsWith(QChar(0xfeff)))input.remove(0,1);QChar delimiter=',';
 const int firstEnd=input.indexOf(QRegularExpression("[\\r\\n]"));const QString first=input.left(firstEnd<0?input.size():firstEnd);if(first.count(';')>first.count(','))delimiter=';';
 QStringList row;QString value;bool quoted=false,afterQuote=false;
 auto fail=[&](const QString &message){if(error)*error=message;rows.clear();return false;};
 for(int i=0;i<input.size();++i){const QChar c=input[i];
  if(quoted){if(c=='"'){if(i+1<input.size()&&input[i+1]=='"'){value+='"';++i;}else {quoted=false;afterQuote=true;}}else value+=c;continue;}
  if(c=='"'){if(afterQuote||!value.isEmpty())return fail(QStringLiteral("Unexpected quote in CSV."));quoted=true;continue;}
  if(c==delimiter){row<<value;value.clear();afterQuote=false;continue;}
  if(c=='\r'||c=='\n'){if(c=='\r'&&i+1<input.size()&&input[i+1]=='\n')++i;row<<value;if(row.size()>1||!row.first().isEmpty())rows<<row;row.clear();value.clear();afterQuote=false;continue;}
  if(afterQuote)return fail(QStringLiteral("Unexpected content after a quoted CSV cell."));value+=c;
 }
 if(quoted)return fail(QStringLiteral("Unterminated quoted CSV cell."));if(!value.isEmpty()||!row.isEmpty()||afterQuote){row<<value;rows<<row;}return true;
}
struct CsvImportResult {QVariantMap data;QStringList warnings;int imported=0;bool ok=false;QString error;};
inline CsvImportResult importCsv(const QVariantMap &data,const QByteArray &bytes,bool overwriteBlank=false) {
 CsvImportResult result;result.data=data;QStringDecoder decoder(QStringDecoder::Utf8);const QString decoded=decoder.decode(bytes);if(decoder.hasError()){result.error=QStringLiteral("The CSV file must use UTF-8 encoding.");return result;}QList<QStringList> rows;if(!parseCsv(decoded,rows,&result.error))return result;
 if(rows.isEmpty()){result.error=QStringLiteral("The CSV file is empty.");return result;}
 const auto header=rows.takeFirst();const int idColumn=header.indexOf("ID"),sourceColumn=header.indexOf("Source");if(idColumn<0||sourceColumn<0){result.error=QStringLiteral("CSV must contain ID and Source columns.");return result;}
 QSet<QString> seenHeaders;for(const auto &h:header){if(seenHeaders.contains(h)){result.error=QStringLiteral("Duplicate CSV heading: %1").arg(h);return result;}seenHeaders.insert(h);}
 QList<QPair<int,QString>> languages;QStringList codes=languageCodes(data);
 for(int i=0;i<header.size();++i){if(i==idColumn||i==sourceColumn||header[i]=="Context")continue;const QString code=header[i].trimmed();if(!validLanguageCode(code)){result.warnings<<QStringLiteral("Ignored unknown language column: %1").arg(header[i]);continue;}languages<<qMakePair(i,code);if(!codes.contains(code))codes<<code;}
 if(languages.isEmpty()){result.error=QStringLiteral("No language columns were found.");return result;}
 auto entries=data.value("entries").toMap();QSet<QString> seenIds;
 for(int n=0;n<rows.size();++n){const auto &row=rows[n];if(row.size()!=header.size()){result.warnings<<QStringLiteral("Row %1 has the wrong number of cells and was skipped.").arg(n+2);continue;}const QString id=row[idColumn];if(seenIds.contains(id)){result.warnings<<QStringLiteral("Duplicate ID %1 was skipped.").arg(id);continue;}seenIds.insert(id);if(!entries.contains(id)){result.warnings<<QStringLiteral("Unknown ID %1 was skipped.").arg(id);continue;}auto e=entries.value(id).toMap();if(row[sourceColumn]!=e.value("source").toString()){result.warnings<<QStringLiteral("Source text changed for %1; translation row was skipped.").arg(id);continue;}auto variants=e.value("variants").toMap();
  for(const auto &column:languages){if(column.second==sourceLanguage(data))continue;const QString translated=row[column.first];if(translated.isEmpty()&&!overwriteBlank)continue;if(!translated.isEmpty()&&placeholders(translated)!=placeholders(e.value("source").toString())){result.warnings<<QStringLiteral("Placeholders differ for %1 / %2; translation was skipped.").arg(id,column.second);continue;}auto v=variants.value(column.second).toMap();if(v.value("text").toString()!=translated||v.value("stale").toBool()){v["text"]=translated;v["stale"]=false;variants[column.second]=v;++result.imported;}}
  e["variants"]=variants;entries[id]=e;
 }
 result.data["entries"]=entries;result.data["languages"]=codes;result.ok=true;return result;
}
inline QDomElement toXml(QDomDocument &document,const QVariantMap &data) {
 auto element=document.createElement("languages-v16");element.setAttribute("version",1);element.appendChild(document.createTextNode(QString::fromUtf8(QJsonDocument::fromVariant(data).toJson(QJsonDocument::Compact))));return element;
}
inline QVariantMap fromXml(const QDomElement &element) {
 if(element.tagName()!="languages-v16"||element.attribute("version","1")!="1")return {};QJsonParseError error;auto doc=QJsonDocument::fromJson(element.text().toUtf8(),&error);return error.error==QJsonParseError::NoError&&doc.isObject()?doc.toVariant().toMap():QVariantMap();
}
}
#endif
