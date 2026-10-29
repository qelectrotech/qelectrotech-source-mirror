/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QET_TEXTSTYLE13_H
#define QET_TEXTSTYLE13_H
#include <QtWidgets>
#include <QJsonDocument>
#include <QJsonObject>

namespace QetFormatting {
inline QString encode(const QVariantMap &v) { return QString::fromUtf8(QJsonDocument::fromVariant(v).toJson(QJsonDocument::Compact)); }
inline QVariantMap decode(const QString &s) { return QJsonDocument::fromJson(s.toUtf8()).object().toVariantMap(); }
inline QFont font(const QVariantMap &v, const QFont &fallback) {
    QFont f=fallback; if (!v.value("font").toString().isEmpty()) f.fromString(v.value("font").toString()); return f;
}
inline QColor color(const QVariantMap &v, const QColor &fallback=Qt::black) {
    QColor c(v.value("color").toString()); return c.isValid()?c:fallback;
}
class TextStyleWidget : public QGroupBox {
    QFont m_font; QColor m_color; QPushButton *m_fontButton,*m_colorButton; QSpinBox *m_opacity;
public:
    TextStyleWidget(const QString &title,const QVariantMap &value,const QFont &fallback,QWidget *parent=nullptr):QGroupBox(title,parent),m_font(QetFormatting::font(value,fallback)),m_color(QetFormatting::color(value)) {
        setCheckable(true); setChecked(!value.isEmpty());
        auto *form=new QFormLayout(this); m_fontButton=new QPushButton; m_colorButton=new QPushButton; m_opacity=new QSpinBox;
        m_opacity->setRange(0,100);m_opacity->setSuffix(" %");m_opacity->setValue(qRound(m_color.alphaF()*100));
        form->addRow(tr("Font"),m_fontButton);form->addRow(tr("Colour"),m_colorButton);form->addRow(tr("Opacity"),m_opacity);
        auto refresh=[this](){m_fontButton->setText(m_font.family()+" "+QString::number(m_font.pointSizeF()));m_colorButton->setText(m_color.name());};refresh();
        connect(m_fontButton,&QPushButton::clicked,this,[this,refresh](){bool ok=false;auto f=QFontDialog::getFont(&ok,m_font,this);if(ok){m_font=f;refresh();}});
        connect(m_colorButton,&QPushButton::clicked,this,[this,refresh](){auto c=QColorDialog::getColor(m_color,this,tr("Text colour"),QColorDialog::DontUseNativeDialog);if(c.isValid()){m_color=c;refresh();}});
    }
    QVariantMap value() const { if(!isChecked())return {};QColor c=m_color;c.setAlphaF(m_opacity->value()/100.0);return {{"font",m_font.toString()},{"color",c.name(QColor::HexArgb)}}; }
};
inline QString styledHtml(const QTextDocument *source,const QFont &font,const QColor &color) {
    QScopedPointer<QTextDocument> doc(source->clone());QTextCursor cursor(doc.data());cursor.select(QTextCursor::Document);
    QTextCharFormat f;f.setFont(font);f.setForeground(color);cursor.mergeCharFormat(f);return doc->toHtml();
}
inline void drawStyledText(QPainter &p,const QRectF &rect,const QString &label,const QString &value,const QVariantMap &labelStyle,const QVariantMap &valueStyle,const QFont &fallback,int alignment,bool fit) {
    QTextDocument doc;doc.setDocumentMargin(0);doc.setDefaultFont(fallback);
    QTextCursor cursor(&doc);QTextBlockFormat block;block.setAlignment(Qt::Alignment(alignment & Qt::AlignHorizontal_Mask));cursor.setBlockFormat(block);
    auto add=[&](const QString &text,const QVariantMap &style){QTextCharFormat f;f.setFont(font(style,fallback));f.setForeground(color(style,p.pen().color()));cursor.insertText(text,f);};
    add(label,labelStyle);add(value,valueStyle);
    doc.adjustSize();qreal scale=fit && doc.idealWidth()>rect.width()?rect.width()/doc.idealWidth():1.0;
    doc.setTextWidth(rect.width()/qMax(0.01,scale));
    qreal height=doc.size().height()*scale; qreal y=rect.top();
    if(alignment & Qt::AlignVCenter)y+=(rect.height()-height)/2;else if(alignment & Qt::AlignBottom)y+=rect.height()-height;
    p.save();p.setClipRect(rect,Qt::IntersectClip);p.translate(rect.left(),y);p.scale(scale,scale);doc.drawContents(&p);p.restore();
}
}
#endif
