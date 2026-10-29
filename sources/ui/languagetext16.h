// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_LANGUAGETEXT16_H
#define QET_LANGUAGETEXT16_H
#include <QVariantMap>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QTextLayout>
#include <QSharedPointer>
#include <QFont>
#include <QColor>
#include "languagecatalog16.h"

namespace QetLanguageText16 {
inline QVariantMap entry(const QVariantMap &data,const QString &id) {
 return data.value("entries").toMap().value(id).toMap();
}
inline QVariantMap variant(const QVariantMap &data,const QString &id) {
 auto actual=QetLanguages16::effectiveId21(data,id);return entry(data,actual).value("variants").toMap().value(data.value("activeLanguage",data.value("sourceLanguage","en")).toString()).toMap();
}
inline bool hasTranslation(const QVariantMap &data,const QString &id) {
 const auto e=entry(data,id),v=variant(data,id);
 return e.value("enabled").toBool()&&!v.value("stale").toBool()&&!v.value("text").toString().isEmpty() && data.value("activeLanguage",data.value("sourceLanguage","en"))!=data.value("sourceLanguage","en");
}
inline bool hasDisplay(const QVariantMap &data,const QString &id) {
 const auto e=entry(data,id),v=variant(data,id);
 if(id.isEmpty()||!e.value("enabled").toBool())return false;
 return hasTranslation(data,id)||!v.value("font").toString().isEmpty()||v.contains("width")||v.contains("align")||v.contains("alignment")||v.contains("direction");
}
inline QSharedPointer<QTextDocument> document(const QVariantMap &data,const QString &id,const QTextDocument *source,const QFont &sourceFont,qreal sourceWidth) {
 if(!source||!hasDisplay(data,id))return {};
 if(QetLanguages16::hiddenMember21(data,id)){auto empty=QSharedPointer<QTextDocument>::create();empty->setDocumentMargin(0);return empty;}
 auto v=variant(data,id);QSharedPointer<QTextDocument> doc(source->clone());
 if(hasTranslation(data,id) && (QetLanguages16::effectiveId21(data,id)!=id||entry(data,id).value("source",source->toPlainText()).toString()==source->toPlainText())) {
  QTextCursor first(const_cast<QTextDocument*>(source));first.movePosition(QTextCursor::Start);first.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor);auto format=first.charFormat();auto paragraph=first.blockFormat();
  doc->setPlainText(v.value("text").toString());QTextCursor all(doc.data());all.select(QTextCursor::Document);all.mergeCharFormat(format);all.mergeBlockFormat(paragraph);
 }
 QFont f=sourceFont;bool overrideFont=!v.value("font").toString().isEmpty()&&f.fromString(v.value("font").toString());
 doc->setDefaultFont(f);if(overrideFont){QTextCursor all(doc.data());all.select(QTextCursor::Document);QTextCharFormat fmt;fmt.setFont(f);all.mergeCharFormat(fmt);}
 if(v.contains("color")){QColor colour(v.value("color").toString());if(colour.isValid()){QTextCursor all(doc.data());all.select(QTextCursor::Document);QTextCharFormat format;format.setForeground(colour);all.mergeCharFormat(format);}}
 auto option=doc->defaultTextOption();option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
 if(v.contains("align")||v.contains("alignment")) {
  auto setting=v.value("align",v.value("alignment"));int a=setting.toInt();QString name=setting.toString().toLower();if(name=="left")a=Qt::AlignLeft;else if(name=="right")a=Qt::AlignRight;else if(name=="center"||name=="centre")a=Qt::AlignHCenter;
  if(a&Qt::AlignHorizontal_Mask){option.setAlignment(Qt::Alignment(a&Qt::AlignHorizontal_Mask));QTextCursor all(doc.data());all.select(QTextCursor::Document);QTextBlockFormat bf;bf.setAlignment(option.alignment());all.mergeBlockFormat(bf);}
 }
 QString direction=v.value("direction","auto").toString();option.setTextDirection(direction=="rtl"?Qt::RightToLeft:direction=="ltr"?Qt::LeftToRight:Qt::LayoutDirectionAuto);doc->setDefaultTextOption(option);
 qreal width=v.value("width",sourceWidth).toDouble();if(width>0)doc->setTextWidth(qBound<qreal>(1,width,100000));else {doc->setTextWidth(-1);doc->size();}
 return doc;
}
inline int lineCount(const QTextDocument *document) {
 if(!document)return 0;document->size();int count=0;for(auto b=document->begin();b.isValid();b=b.next())if(b.layout())count+=qMax(1,b.layout()->lineCount());return count;
}
}
#endif
