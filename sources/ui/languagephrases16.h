// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGEPHRASES16_H
#define QET_LANGUAGEPHRASES16_H
#include "languagecatalog16.h"
#include <QCryptographicHash>
namespace QetLanguages16 {
constexpr int SourceTextRole16 = Qt::UserRole + 1600;
inline bool protectedPhraseColumn(const QString &heading) {
 QString key=heading.toLower();key.remove(QRegularExpression("[^a-z0-9]"));
 static const QSet<QString> protectedKeys={"label","reference","references","ref","manufacturer","maker","brand","manufacturerreference","articlenumber","article","partnumber","quantity","qty","no","number","itemnumber","unity","unit","date","folio","page","position","diagramposition","filename"};
 return protectedKeys.contains(key)||key.startsWith("manufacturerreference")||key.startsWith("manufacturerauxiliary")||key.startsWith("quantityauxiliary");
}
inline bool protectedTitleValue(const QString &value) {
 static const QRegularExpression field("^%\\{?(author|filename|date|folio|plant|locmach|indexrev|version|previous|next)\\}?$",QRegularExpression::CaseInsensitiveOption);
 return field.match(value.trimmed()).hasMatch();
}
inline QString phraseId(const QString &source) {
 return QStringLiteral("phrase:")+QString::fromLatin1(QCryptographicHash::hash(source.toUtf8(),QCryptographicHash::Sha256).toHex());
}
inline bool isHumanPhrase(const QString &source) {
 const QString s=source.trimmed();if(s.isEmpty()||s.size()>20000||!s.contains(QRegularExpression("\\p{L}")))return false;
 // Electrical identifiers, physical values and part references stay literal.
 static const QRegularExpression identifier("^-?[A-Z0-9][A-Z0-9+./:_-]*$");
 if(identifier.match(s).hasMatch())return false;
 QString compact=s;compact.remove(QRegularExpression("\\s+"));if(compact.contains(QRegularExpression("[0-9+./:_-]"))&&identifier.match(compact).hasMatch())return false;
 if(s.startsWith("/")||s.contains(QRegularExpression("^[0-9.,]+\\s*(V|VAC|VDC|A|Hz|kW|W|mm|m)$",QRegularExpression::CaseInsensitiveOption)))return false;
 return true;
}
inline QString registerPhrase(QVariantMap &data,const QString &source,const QString &context) {
 if(!isHumanPhrase(source))return QString();const QString id=phraseId(source);auto entries=data.value("entries").toMap();auto e=entries.value(id).toMap();
 if(e.isEmpty())e={{"source",source},{"context",context},{"kind","phrase"},{"enabled",true}};
 else if(!e.value("context").toString().contains(context))e["context"]=e.value("context").toString()+"; "+context;
 entries[id]=e;data["entries"]=entries;return id;
}
inline QString resolvedPhrase(const QVariantMap &data,const QString &source) {
 return resolvedText(data,phraseId(source),source);
}
}
#endif
