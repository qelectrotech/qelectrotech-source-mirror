/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QET_TABLEFORMAT13_H
#define QET_TABLEFORMAT13_H
#include "textstyle13.h"
#include "tablemanual25.h"
#include "languagephrases16.h"
#include "../qetproject.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include <QAbstractItemModel>
#include <QTextLayout>
#include <cmath>

namespace QetFormatting {
inline int number(const QVariantMap &v,const char *key,int fallback,int minimum=1,int maximum=100000){return qBound(minimum,v.value(key,fallback).toInt(),maximum);}
inline QVector<int> tableWidths(const QVariantMap &v,int columns){
    QVector<int> result;auto widths=v.value("widths").toList();for(int i=0;i<columns;++i)result<<qBound(10,i<widths.size()?widths[i].toInt():100,10000);return result;
}
inline int tableWidth(const QVariantMap &v,int columns){int width=0;for(int w:tableWidths(v,columns))width+=w;return qMax(10,width);}
inline int tableTopHeight(const QVariantMap &v){return number(v,"headerHeight",20)+(v.value("title").toString().isEmpty()?0:number(v,"titleHeight",24));}
inline bool columnFlag(const QVariantMap &v,const char *key,int column,bool fallback=false){auto a=v.value(key).toList();return column>=0&&column<a.size()?a[column].toBool():fallback;}
inline bool columnOverflow(const QVariantMap &v,int column){auto a=v.value("overflowModes").toList();return column<a.size()&&a[column].toString()=="overflow";}
// 1.7 styles inherit table -> column -> absolute model row. Row indices are
// intentionally positional; refreshing/sorting the query may change the item
// displayed at a styled row. Empty overrides preserve previous-release output.
inline QVariantMap mergedStyle17(QVariantMap base,const QVariantMap &overrideStyle){for(auto it=overrideStyle.cbegin();it!=overrideStyle.cend();++it)base[it.key()]=it.value();return base;}
inline QVariantMap columnOverride17(const QVariantMap &v,int column){auto a=v.value("columnOverrides17").toList();return column>=0&&column<a.size()?a[column].toMap():QVariantMap();}
inline QVariantMap rowOverride17(const QVariantMap &v,int modelRow){return v.value("rowOverrides17").toMap().value(QString::number(modelRow)).toMap();}
inline QVariantMap cellOverride20(const QVariantMap &v,int c,int r){return v.value("cellOverrides20").toMap().value(QString::number(r)+":"+QString::number(c)).toMap();}
inline QVariantMap headerCellStyle17(const QVariantMap &v,int column){return mergedStyle17(mergedStyle17(v.value("headerStyle").toMap(),columnOverride17(v,column).value("headerStyle").toMap()),cellOverride20(v,column,-1).value("style").toMap());}
inline QVariantMap bodyCellStyle17(const QVariantMap &v,int column,int modelRow){return mergedStyle17(mergedStyle17(mergedStyle17(v.value("bodyStyle").toMap(),columnOverride17(v,column).value("bodyStyle").toMap()),rowOverride17(v,modelRow).value("bodyStyle").toMap()),cellOverride20(v,column,modelRow).value("style").toMap());}
inline QColor validBackground17(const QVariantMap &v,const char *key,const QColor &fallback){QColor c(v.value(key).toString());return c.isValid()?c:fallback;}
inline QColor headerCellBackground17(const QVariantMap &v,int column){return validBackground17(cellOverride20(v,column,-1),"background",validBackground17(columnOverride17(v,column),"headerBackground",validBackground17(v,"headerBackground",QColor("#ffd0d0d0"))));}
inline QColor bodyCellBackground17(const QVariantMap &v,int column,int modelRow){return validBackground17(cellOverride20(v,column,modelRow),"background",validBackground17(rowOverride17(v,modelRow),"bodyBackground",validBackground17(columnOverride17(v,column),"bodyBackground",validBackground17(v,"bodyBackground",Qt::white))));}
inline int cellAlignment20(const QVariantMap &v,int c,int r,int fallback){
    int a=fallback|Qt::AlignVCenter;auto merge=[&](const QVariantMap &o){if(o.contains("hAlign"))a=(a&~int(Qt::AlignHorizontal_Mask))|o.value("hAlign").toInt();if(o.contains("vAlign"))a=(a&~int(Qt::AlignVertical_Mask))|o.value("vAlign").toInt();};
    const QString prefix=r==-2?"title":r==-1?"header":"body";merge(v.value(prefix+"Alignment20").toMap());if(c>=0)merge(columnOverride17(v,c).value(prefix+"Alignment20").toMap());if(r>=0)merge(rowOverride17(v,r).value("bodyAlignment20").toMap());merge(cellOverride20(v,c,r));return a;
}
inline bool cellWrap20(const QVariantMap &v,int c,int r){if(r==-2)return v.value("titleAlignment20").toMap().value("wrap").toBool();bool result=columnFlag(v,"wraps",c);auto apply=[&](const QVariantMap &o){if(o.contains("wrap"))result=o.value("wrap").toBool();};apply(v.value(r==-1?"headerAlignment20":"bodyAlignment20").toMap());apply(columnOverride17(v,c).value(r==-1?"headerAlignment20":"bodyAlignment20").toMap());if(r>=0)apply(rowOverride17(v,r).value("bodyAlignment20").toMap());apply(cellOverride20(v,c,r));return result;}
inline QString tableLanguageText(const QAbstractItemModel *model,const QString &source){if(auto *db=qobject_cast<const ProjectDBModel*>(model))if(db->project())return QetLanguages16::resolvedPhrase(db->project()->languageData(),source);return source;}
inline QString tableTitle26(const QVariantMap &v,const QAbstractItemModel *m){
 auto overrides=v.value("titleTexts26").toMap();auto language=QetManualCells25::language(m);
 if(overrides.contains(language))return overrides.value(language).toString();
 return tableLanguageText(m,v.value("title").toString());
}
inline QString sourceColumnHeading25(const QVariantMap &v,const QAbstractItemModel *m,int col){
    if(!m||col<0||col>=m->columnCount())return {};
    auto a=v.value("headers").toList();if(col<a.size())return a[col].toString();
    auto raw=m->headerData(col,Qt::Horizontal,QetLanguages16::SourceTextRole16);
    return (raw.isValid()?raw:m->headerData(col,Qt::Horizontal)).toString();
}
inline QString columnHeading(const QVariantMap &v,const QAbstractItemModel *m,int col){
    auto key=QetManualCells25::headerKey(m,col);auto edits=v.value("headerTexts25").toMap().value(QetManualCells25::language(m)).toMap();
    if(!key.isEmpty()&&edits.contains(key))return edits[key].toString();
    return tableLanguageText(m,sourceColumnHeading25(v,m,col));
}
// Measurement and painting use the same QTextLayout path, including explicit
// newlines and long words. No font shrinking or text mutation is performed.
inline QSizeF layoutCellText(const QString &text,const QFont &f,qreal width,bool wrap,int alignment,QPainter *p=nullptr,const QPointF &origin=QPointF()){
    qreal height=0,maxWidth=0;const auto paragraphs=text.split('\n');
    for(const auto &paragraph:paragraphs){
        QTextLayout layout(paragraph,f);QTextOption option;option.setWrapMode(wrap?QTextOption::WrapAtWordBoundaryOrAnywhere:QTextOption::NoWrap);option.setAlignment(Qt::Alignment(alignment&Qt::AlignHorizontal_Mask));layout.setTextOption(option);
        layout.beginLayout();bool any=false;while(true){auto line=layout.createLine();if(!line.isValid())break;any=true;line.setLineWidth(qMax<qreal>(1,width));line.setPosition(QPointF(0,height));height+=line.height();maxWidth=qMax(maxWidth,line.naturalTextWidth());}layout.endLayout();
        if(!any)height+=QFontMetricsF(f).height();if(p)layout.draw(p,origin);
    }
    return QSizeF(maxWidth,height);
}
struct TableLayout15 {
    QVector<int> widths,rowHeights;
    int width=10,bodyHeight=0,headerHeight=20,titleHeight=0;
    QRectF bounds;
    QStringList warnings;
};
inline QVector<int> fittedColumnWidths(const QVariantMap &v,const QAbstractItemModel *model,int offset,int rows,const QFont &fallback){
    QVector<int> result;if(!model)return result;int px=number(v,"paddingX",3,0,100);
    for(int c=0;c<model->columnCount();++c){qreal w=0;auto measure=[&](const QString &s,const QFont &f){for(const auto &part:s.split('\n'))w=qMax(w,QFontMetricsF(f).horizontalAdvance(part));};measure(columnHeading(v,model,c),font(headerCellStyle17(v,c),fallback));for(int r=0;r<rows;++r)measure(QetManualCells25::text(v,model,offset+r,c),font(bodyCellStyle17(v,c,offset+r),fallback));result<<qBound(10,int(std::ceil(w))+2*px,10000);}return result;
}
// A manual body-row height overrides automatic sizing for that absolute model
// row only. Continuation tables therefore share the same row numbering.
inline int rowHeightOverride18(const QVariantMap &v,int modelRow) {
    bool ok=false;int h=v.value("rowHeights18").toMap().value(QString::number(modelRow)).toInt(&ok);
    return ok&&h>=6?qBound(6,h,10000):0;
}
inline int fittedRowHeight18(const QVariantMap &v,const QAbstractItemModel *model,int modelRow,const QVector<int> &widths,const QFont &fallback) {
    const int px=number(v,"paddingX",3,0,100),py=number(v,"paddingY",1,0,100);int h=6;
    if(model)for(int c=0;c<widths.size();++c){const QString text=QetManualCells25::text(v,model,modelRow,c);
        h=qMax(h,int(std::ceil(layoutCellText(text,font(bodyCellStyle17(v,c,modelRow),fallback),widths[c]-2*px,cellWrap20(v,c,modelRow),Qt::AlignLeft).height()))+2*py);}
    return qBound(6,h,10000);
}
inline TableLayout15 tableLayout(const QVariantMap &v,const QAbstractItemModel *model,int offset,int rows,const QFont &fallback){
    TableLayout15 g;if(!model){g.bounds=QRectF(0,0,10,0);return g;}g.widths=tableWidths(v,model->columnCount());const auto fit=fittedColumnWidths(v,model,offset,rows,fallback);for(int c=0;c<g.widths.size();++c)if(columnFlag(v,"autoWidths",c))g.widths[c]=fit[c];g.width=0;for(int w:g.widths)g.width+=w;g.width=qMax(10,g.width);
    const int px=number(v,"paddingX",3,0,100),py=number(v,"paddingY",1,0,100);const auto tf=font(v.value("titleStyle").toMap(),fallback);
    g.headerHeight=number(v,"headerHeight",20);g.titleHeight=v.value("title").toString().isEmpty()?0:number(v,"titleHeight",24);
    for(int c=0;c<g.widths.size();++c)if(v.value("autoHeaderHeight").toBool())g.headerHeight=qMax(g.headerHeight,int(std::ceil(layoutCellText(columnHeading(v,model,c),font(headerCellStyle17(v,c),fallback),g.widths[c]-2*px,cellWrap20(v,c,-1),Qt::AlignLeft).height()))+2*py);
    for(int r=0;r<rows;++r){int h=rowHeightOverride18(v,offset+r);if(!h){h=number(v,"rowHeight",20);if(v.value("autoRowHeight").toBool())h=qMax(h,fittedRowHeight18(v,model,offset+r,g.widths,fallback));}g.rowHeights<<h;g.bodyHeight+=h;}
    g.bounds=QRectF(0,-g.headerHeight-g.titleHeight,g.width,g.bodyHeight+g.headerHeight+g.titleHeight);
    auto inspect=[&](const QRectF &cell,const QString &s,const QFont &f,int c,int row){auto size=layoutCellText(s,f,cell.width()-2*px,cellWrap20(v,c,row<0?-1:offset+row),Qt::AlignLeft);bool over=size.width()>cell.width()-2*px+0.1||size.height()>cell.height()-2*py+0.1;if(over&&!s.isEmpty()){if(g.warnings.size()<12)g.warnings<<QObject::tr("%1, column %2: text exceeds the cell (%3).").arg(row<0?QObject::tr("Header"):QObject::tr("Row %1").arg(row+1)).arg(c+1).arg(columnOverflow(v,c)?QObject::tr("overflow allowed"):QObject::tr("clipped"));if(columnOverflow(v,c)){// Allow redraw/culling beyond the table in every alignment.
            qreal horizontal=qMax<qreal>(0,size.width()-(cell.width()-2*px));g.bounds=g.bounds.united(cell.adjusted(-horizontal,0,horizontal,qMax<qreal>(0,size.height()-(cell.height()-2*py))));}}};
    int x=0;for(int c=0;c<g.widths.size();++c){inspect(QRectF(x,-g.headerHeight,g.widths[c],g.headerHeight),columnHeading(v,model,c),font(headerCellStyle17(v,c),fallback),c,-1);int y=0;for(int r=0;r<rows;++r){QString s=QetManualCells25::text(v,model,offset+r,c);inspect(QRectF(x,y,g.widths[c],g.rowHeights[r]),s,font(bodyCellStyle17(v,c,offset+r),fallback),c,r);y+=g.rowHeights[r];}x+=g.widths[c];}
    if(g.titleHeight){auto titleSize=layoutCellText(tableTitle26(v,model),tf,g.width-2*px,v.value("titleAlignment20").toMap().value("wrap").toBool(),Qt::AlignHCenter);if(titleSize.height()>g.titleHeight-2*py||titleSize.width()>g.width-2*px)g.warnings<<QObject::tr("The title text exceeds its fixed cell size.");}return g;
}
inline void paintTable(QPainter &p,const QVariantMap &v,const QAbstractItemModel *model,int offset,int rows,const QFont &fallback){
    if(!model)return;const auto g=tableLayout(v,model,offset,rows,fallback);const int px=number(v,"paddingX",3,0,100),py=number(v,"paddingY",1,0,100);auto align=v.value("alignments").toList(),halign=v.value("headerAlignments").toList();QColor grid(v.value("gridColor","#ff000000").toString());if(!grid.isValid())grid=Qt::black;
    auto background=[&](const char *key,const char *def){QColor c(v.value(key,def).toString());return c.isValid()?c:QColor(def);};
    struct Cell {QRectF rect;QString text;QVariantMap style;QColor bg;int align;bool wrap,overflow;};QVector<Cell> cells;
    if(g.titleHeight)cells<<Cell{QRectF(0,-g.headerHeight-g.titleHeight,g.width,g.titleHeight),tableTitle26(v,model),v.value("titleStyle").toMap(),background("titleBackground","#ffe0e0e0"),cellAlignment20(v,-1,-2,Qt::AlignHCenter),v.value("titleAlignment20").toMap().value("wrap").toBool(),false};
    int x=0;for(int c=0;c<g.widths.size();++c){cells<<Cell{QRectF(x,-g.headerHeight,g.widths[c],g.headerHeight),columnHeading(v,model,c),headerCellStyle17(v,c),headerCellBackground17(v,c),cellAlignment20(v,c,-1,c<halign.size()?halign[c].toInt():Qt::AlignHCenter),cellWrap20(v,c,-1),columnOverflow(v,c)};int y=0;for(int r=0;r<rows;++r){cells<<Cell{QRectF(x,y,g.widths[c],g.rowHeights[r]),QetManualCells25::text(v,model,offset+r,c),bodyCellStyle17(v,c,offset+r),bodyCellBackground17(v,c,offset+r),cellAlignment20(v,c,offset+r,c<align.size()?align[c].toInt():Qt::AlignLeft),cellWrap20(v,c,offset+r),columnOverflow(v,c)};y+=g.rowHeights[r];}x+=g.widths[c];}
    // Paint all backgrounds before text so deliberate overflow is not erased
    // when the neighbouring cell is painted afterwards.
    for(const auto &c:cells){p.save();p.setPen(QPen(grid,0.7));p.setBrush(c.bg);p.drawRect(c.rect);p.restore();}
    for(const auto &c:cells){p.save();QRectF content=c.rect.adjusted(px,py,-px,-py);if(!c.overflow)p.setClipRect(c.rect.adjusted(0.5,0.5,-0.5,-0.5),Qt::IntersectClip);auto f=font(c.style,fallback);auto size=layoutCellText(c.text,f,content.width(),c.wrap,c.align);p.setFont(f);p.setPen(color(c.style));layoutCellText(c.text,f,content.width(),c.wrap,c.align,&p,content.topLeft()+QPointF(0,qMax<qreal>(0,(c.align&Qt::AlignBottom)?content.height()-size.height():(c.align&Qt::AlignTop)?0:(content.height()-size.height())/2)));p.restore();}
}
// Font and colour are independent overrides. In particular a row-font change
// must not flatten the colours inherited from different columns (and vice versa).
class TextOverrideEditor17 : public QGroupBox {
    QCheckBox *m_fontOverride=nullptr,*m_colourOverride=nullptr;
    QPushButton *m_fontButton=nullptr,*m_colourButton=nullptr;
    QSpinBox *m_opacity=nullptr;
public:
    TextOverrideEditor17(const QString &title,const QVariantMap &original,const QVariantMap &inherited,const QFont &fallback,const QString &prefix,QWidget *parent=nullptr):QGroupBox(title,parent){
        auto *form=new QFormLayout(this);
        m_fontOverride=new QCheckBox(tr("Override font"));m_fontOverride->setObjectName(prefix+"FontOverride17");m_fontOverride->setChecked(!original.value("font").toString().isEmpty());
        auto initialFont=QetFormatting::font(mergedStyle17(inherited,original),fallback);m_fontButton=new QPushButton(initialFont.family()+" "+QString::number(initialFont.pointSizeF()));m_fontButton->setObjectName(prefix+"Font17");m_fontButton->setProperty("fontValue17",initialFont.toString());m_fontButton->setEnabled(m_fontOverride->isChecked());form->addRow(m_fontOverride,m_fontButton);
        QObject::connect(m_fontOverride,&QCheckBox::toggled,m_fontButton,&QPushButton::setEnabled);
        QObject::connect(m_fontButton,&QPushButton::clicked,this,[this](){bool ok=false;QFont current;current.fromString(m_fontButton->property("fontValue17").toString());auto chosen=QFontDialog::getFont(&ok,current,this);if(ok){m_fontButton->setProperty("fontValue17",chosen.toString());m_fontButton->setText(chosen.family()+" "+QString::number(chosen.pointSizeF()));}});
        m_colourOverride=new QCheckBox(tr("Override colour / opacity"));m_colourOverride->setObjectName(prefix+"ColourOverride17");m_colourOverride->setChecked(QColor(original.value("color").toString()).isValid());auto initialColour=QetFormatting::color(mergedStyle17(inherited,original));
        auto *colourControls=new QWidget;auto *cl=new QHBoxLayout(colourControls);cl->setContentsMargins(0,0,0,0);m_colourButton=new QPushButton(initialColour.name());m_colourButton->setObjectName(prefix+"TextColour17");m_colourButton->setProperty("colour",initialColour.name(QColor::HexArgb));m_opacity=new QSpinBox;m_opacity->setObjectName(prefix+"TextOpacity17");m_opacity->setRange(0,100);m_opacity->setSuffix(" %");m_opacity->setValue(qRound(initialColour.alphaF()*100));cl->addWidget(m_colourButton);cl->addWidget(m_opacity);colourControls->setEnabled(m_colourOverride->isChecked());form->addRow(m_colourOverride,colourControls);
        QObject::connect(m_colourOverride,&QCheckBox::toggled,colourControls,&QWidget::setEnabled);
        QObject::connect(m_colourButton,&QPushButton::clicked,this,[this](){auto initial=QColor(m_colourButton->property("colour").toString());initial.setAlphaF(m_opacity->value()/100.0);auto chosen=QColorDialog::getColor(initial,this,tr("Text colour and opacity"),QColorDialog::DontUseNativeDialog|QColorDialog::ShowAlphaChannel);if(chosen.isValid()){m_colourButton->setProperty("colour",chosen.name(QColor::HexArgb));m_colourButton->setText(chosen.name());m_opacity->setValue(qRound(chosen.alphaF()*100));}});
    }
    QVariantMap value() const {
        QVariantMap result;
        if(m_fontOverride->isChecked())result["font"]=m_fontButton->property("fontValue17").toString();
        if(m_colourOverride->isChecked()){QColor c(m_colourButton->property("colour").toString());c.setAlphaF(m_opacity->value()/100.0);result["color"]=c.name(QColor::HexArgb);}
        return result;
    }
};
// A compact modal editor avoids making the main formatting page wider/taller
// for each possible column or row. Unchecked properties inherit their style.
inline bool editCellOverride17(QWidget *parent,QVariantMap &overrideValue,const QVariantMap &tableFormat,const QFont &fallback,bool column){
    QDialog d(parent);d.setObjectName(column?"columnStyle17Dialog":"rowStyle17Dialog");d.setWindowTitle(column?QObject::tr("Column fonts and colours"):QObject::tr("Body-row fonts and colours"));d.resize(column?720:430,390);
    auto *outer=new QVBoxLayout(&d);auto *note=new QLabel(column?QObject::tr("Font, colour/opacity and background can be overridden independently. Unchecked properties inherit the table settings. A row override takes priority over a column override."):QObject::tr("This override applies to this numbered body row, across all columns. Override only the properties you want to change; unchecked properties keep each column/table setting."));note->setWordWrap(true);outer->addWidget(note);
    auto *styleRow=new QHBoxLayout;
    auto makeStyle=[&](const QString &title,const char *key,const char *objectName,const QString &prefix){auto *w=new TextOverrideEditor17(title,overrideValue.value(key).toMap(),tableFormat.value(key).toMap(),fallback,prefix,&d);w->setObjectName(objectName);styleRow->addWidget(w);return w;};
    auto *body=makeStyle(QObject::tr("Body text overrides"),"bodyStyle","bodyTextOverride17","body");TextOverrideEditor17 *header=nullptr;if(column)header=makeStyle(QObject::tr("Header text overrides"),"headerStyle","headerTextOverride17","header");outer->addLayout(styleRow);
    struct BackgroundControl {QString key;QCheckBox *enabled;QPushButton *button;};QList<BackgroundControl> backgrounds;auto *form=new QFormLayout;outer->addLayout(form);
    auto addBackground=[&](const QString &title,const char *key,const char *enabledName,const char *buttonName,const QColor &defaultColor){auto *enabled=new QCheckBox(title);enabled->setObjectName(enabledName);auto original=QColor(overrideValue.value(key).toString());enabled->setChecked(original.isValid());QColor initial=original.isValid()?original:validBackground17(tableFormat,key,defaultColor);auto *button=new QPushButton(initial.name(QColor::HexArgb));button->setObjectName(buttonName);button->setProperty("colour",initial.name(QColor::HexArgb));button->setEnabled(enabled->isChecked());form->addRow(enabled,button);QObject::connect(enabled,&QCheckBox::toggled,button,&QPushButton::setEnabled);QObject::connect(button,&QPushButton::clicked,&d,[button,&d](){auto c=QColorDialog::getColor(QColor(button->property("colour").toString()),&d,QObject::tr("Background colour and opacity"),QColorDialog::DontUseNativeDialog|QColorDialog::ShowAlphaChannel);if(c.isValid()){button->setProperty("colour",c.name(QColor::HexArgb));button->setText(c.name(QColor::HexArgb));}});backgrounds<<BackgroundControl{QString::fromLatin1(key),enabled,button};};
    addBackground(QObject::tr("Override body background"),"bodyBackground","bodyBackgroundOverride17","bodyBackgroundColour17",Qt::white);if(column)addBackground(QObject::tr("Override header background"),"headerBackground","headerBackgroundOverride17","headerBackgroundColour17",QColor("#ffd0d0d0"));
    outer->addStretch();auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);outer->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return false;
    QVariantMap result=overrideValue;for(const auto &key:{"bodyStyle","headerStyle","bodyBackground","headerBackground"})result.remove(key);if(!body->value().isEmpty())result["bodyStyle"]=body->value();if(header&&!header->value().isEmpty())result["headerStyle"]=header->value();for(const auto &b:backgrounds)if(b.enabled->isChecked())result[b.key]=b.button->property("colour");overrideValue=result;return true;
}
inline QString overrideSummary17(const QVariantMap &v){QStringList parts;auto body=v.value("bodyStyle").toMap(),header=v.value("headerStyle").toMap();if(body.contains("font"))parts<<QObject::tr("body font");if(body.contains("color"))parts<<QObject::tr("body colour/opacity");if(header.contains("font"))parts<<QObject::tr("header font");if(header.contains("color"))parts<<QObject::tr("header colour/opacity");if(v.contains("bodyBackground"))parts<<QObject::tr("body background");if(v.contains("headerBackground"))parts<<QObject::tr("header background");return parts.isEmpty()?QObject::tr("Inherits table/column settings"):parts.join(", ");}
// An accepted background choice applies immediately to the dialog draft, even
// when the chosen colour equals the table-level colour. Later individual edits
// can override it again. Cancelling either dialog leaves the original untouched.
inline void clearBackgroundOverrides25(QVariantList &columns,QVariantMap &rows,QVariantMap &cells,const QString &key){
    if(key!="bodyBackground"&&key!="headerBackground"&&key!="titleBackground")return;
    for(auto &entry:columns){auto value=entry.toMap();value.remove(key);entry=value;}
    if(key=="bodyBackground")for(auto it=rows.begin();it!=rows.end();++it){auto value=it.value().toMap();value.remove(key);it.value()=value;}
    for(auto it=cells.begin();it!=cells.end();++it){
        const int row=it.key().section(':',0,0).toInt();
        if((row>=0&&key=="bodyBackground")||(row==-1&&key=="headerBackground")||(row==-2&&key=="titleBackground")){
            auto value=it.value().toMap();value.remove("background");it.value()=value;
        }
    }
}
class TablePreview15 : public QWidget {
public:
    QVariantMap format;QAbstractItemModel *model=nullptr;QFont fallback;int rows=0;TablePreview15(QWidget *parent=nullptr):QWidget(parent){setMinimumSize(450,280);setAutoFillBackground(true);}
    void paintEvent(QPaintEvent *) override {QPainter p(this);p.fillRect(rect(),Qt::white);if(!model)return;auto g=tableLayout(format,model,0,rows,fallback);QRectF b=g.bounds.adjusted(-8,-8,8,8);qreal scale=qMin((width()-16)/qMax<qreal>(1,b.width()),(height()-16)/qMax<qreal>(1,b.height()));p.translate(8,8);p.scale(scale,scale);p.translate(-b.topLeft());paintTable(p,format,model,0,rows,fallback);}
};
inline bool editTableFormat(QWidget *parent,QVariantMap &value,QAbstractItemModel *model,const QVector<int> &currentWidths,int currentRows,const QFont &fallback,qreal availableWidth=0){
    if(!model)return false;QDialog d(parent);d.setObjectName("tableFormat15Dialog");d.setWindowTitle(QObject::tr("Table layout and formatting"));d.resize(1080,740);auto *outer=new QVBoxLayout(&d);
    QVariantList columnOverrides=value.value("columnOverrides17").toList();QVariantMap rowOverrides=value.value("rowOverrides17").toMap();QVariantMap rowHeights=value.value("rowHeights18").toMap();QVariantMap cellOverrides=value.value("cellOverrides20").toMap();
    auto *enabled=new QCheckBox(QObject::tr("Use custom formatting (retained when the table is refreshed)"),&d);enabled->setChecked(value.value("enabled",true).toBool());outer->addWidget(enabled);
    auto *tabs=new QTabWidget(&d);outer->addWidget(tabs);auto *geometry=new QWidget;auto *form=new QFormLayout(geometry);tabs->addTab(geometry,QObject::tr("Layout and columns"));
    auto *title=new QLineEdit(value.value("title").toString());form->addRow(QObject::tr("Full-width title"),title);auto *languageNote=new QLabel(QObject::tr("Title and header text below are the original-language wording. Previews show the drawing language. Use F2 on a header to edit its wording for the current drawing language."));languageNote->setWordWrap(true);form->addRow(languageNote);
    auto spin=[&](const char *key,const QString &label,int initial,int minimum){auto *w=new QSpinBox;w->setObjectName(QString::fromLatin1(key));w->setRange(minimum,10000);w->setValue(number(value,key,initial,minimum,10000));form->addRow(label,w);return w;};
    auto *rows=spin("rows",QObject::tr("Rows (0 = all; extra rows stay blank)"),currentRows,0);
    auto *rowHeight=spin("rowHeight",QObject::tr("Manual / minimum row height (du)"),20,6);
    auto *autoRows=new QCheckBox(QObject::tr("Automatic row height: fit each row to its text"));autoRows->setObjectName("autoRowHeight");autoRows->setChecked(value.value("autoRowHeight").toBool());form->addRow(autoRows);
    auto *headerHeight=spin("headerHeight",QObject::tr("Manual / minimum column-header height"),20,6);
    auto *autoHeader=new QCheckBox(QObject::tr("Automatic column-header height"));autoHeader->setChecked(value.value("autoHeaderHeight").toBool());form->addRow(autoHeader);
    auto *titleHeight=spin("titleHeight",QObject::tr("Title-row height"),24,6);
    auto *paddingX=spin("paddingX",QObject::tr("Horizontal cell padding (du)"),3,0);auto *paddingY=spin("paddingY",QObject::tr("Vertical cell padding (du)"),1,0);paddingX->setMaximum(100);paddingY->setMaximum(100);
    auto *columns=new QTableWidget(model->columnCount(),7);columns->setObjectName("formatColumns15");columns->setSelectionBehavior(QAbstractItemView::SelectRows);columns->setSelectionMode(QAbstractItemView::ExtendedSelection);columns->setHorizontalHeaderLabels({QObject::tr("Header text"),QObject::tr("Width (du)"),QObject::tr("Text alignment"),QObject::tr("Header align"),QObject::tr("Width mode"),QObject::tr("Wrap text"),QObject::tr("Excess text")});columns->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);for(int c=1;c<7;++c)columns->setColumnWidth(c,c==2||c==3?120:c==1?85:110);columns->setMinimumHeight(150);form->addRow(columns);
    auto savedWidths=value.value("widths").toList(),savedAlign=value.value("alignments").toList(),savedHAlign=value.value("headerAlignments").toList();
    for(int i=0;i<model->columnCount();++i){columns->setItem(i,0,new QTableWidgetItem(sourceColumnHeading25(value,model,i)));auto *width=new QSpinBox;width->setRange(10,10000);width->setValue(i<savedWidths.size()?savedWidths[i].toInt():(i<currentWidths.size()?currentWidths[i]:100));columns->setCellWidget(i,1,width);
        for(int c=2;c<4;++c){auto *a=new QComboBox;a->addItem(QObject::tr("Left"),int(Qt::AlignLeft));a->addItem(QObject::tr("Centre"),int(Qt::AlignHCenter));a->addItem(QObject::tr("Right"),int(Qt::AlignRight));auto saved=c==2?savedAlign:savedHAlign;int n=a->findData(i<saved.size()?saved[i].toInt():(c==2?int(Qt::AlignLeft):int(Qt::AlignHCenter)));a->setCurrentIndex(qMax(0,n));columns->setCellWidget(i,c,a);}
        auto *mode=new QComboBox;mode->addItem(QObject::tr("Manual"),false);mode->addItem(QObject::tr("Automatic"),true);mode->setCurrentIndex(columnFlag(value,"autoWidths",i)?1:0);columns->setCellWidget(i,4,mode);width->setEnabled(mode->currentIndex()==0);QObject::connect(mode,QOverload<int>::of(&QComboBox::currentIndexChanged),width,[width](int index){width->setEnabled(index==0);});auto *wrap=new QCheckBox;wrap->setChecked(columnFlag(value,"wraps",i));columns->setCellWidget(i,5,wrap);auto *over=new QComboBox;over->addItem(QObject::tr("Clip"),"clip");over->addItem(QObject::tr("Allow overflow"),"overflow");over->setCurrentIndex(columnOverflow(value,i)?1:0);columns->setCellWidget(i,6,over);}
    auto *actions=new QHBoxLayout;auto *fitColumns=new QPushButton(QObject::tr("Auto-fit selected columns now"));auto *fitRows=new QPushButton(QObject::tr("Auto-fit row height now"));auto *fitAll=new QPushButton(QObject::tr("Auto-fit whole table now"));actions->addWidget(fitColumns);actions->addWidget(fitRows);actions->addWidget(fitAll);form->addRow(actions);
    auto *appearance=new QWidget;auto *al=new QVBoxLayout(appearance);tabs->addTab(appearance,QObject::tr("Fonts and colours"));auto *styleRow=new QHBoxLayout;auto *ts=new TextStyleWidget(QObject::tr("Title text"),value.value("titleStyle").toMap(),fallback);auto *hs=new TextStyleWidget(QObject::tr("Header text"),value.value("headerStyle").toMap(),fallback);auto *bs=new TextStyleWidget(QObject::tr("Body text"),value.value("bodyStyle").toMap(),fallback);styleRow->addWidget(ts);styleRow->addWidget(hs);styleRow->addWidget(bs);al->addLayout(styleRow);
    QMap<QString,QPushButton*> colours;auto *coloursForm=new QFormLayout;al->addLayout(coloursForm);for(auto entry:QList<QPair<QString,QString>>{{"titleBackground",QObject::tr("Title background")},{"headerBackground",QObject::tr("Header background")},{"bodyBackground",QObject::tr("Body background")},{"gridColor",QObject::tr("Grid colour")}}){auto *button=new QPushButton;QString def=entry.first=="gridColor"?"#ff000000":entry.first=="bodyBackground"?"#ffffffff":"#ffd0d0d0";button->setProperty("colour",value.value(entry.first,def));button->setObjectName(entry.first+"25");button->setText(button->property("colour").toString());colours[entry.first]=button;coloursForm->addRow(entry.second,button);QObject::connect(button,&QPushButton::clicked,&d,[button,&d,key=entry.first,&columnOverrides,&rowOverrides,&cellOverrides](){QColor c=QColorDialog::getColor(QColor(button->property("colour").toString()),&d,QObject::tr("Colour and opacity"),QColorDialog::DontUseNativeDialog|QColorDialog::ShowAlphaChannel);if(c.isValid()){button->setProperty("colour",c.name(QColor::HexArgb));button->setText(c.name(QColor::HexArgb));clearBackgroundOverrides25(columnOverrides,rowOverrides,cellOverrides,key);}});}auto *backgroundNote=new QLabel(QObject::tr("Choosing a background applies it to the whole title, header or body, including combined columns. Fonts and other formatting are kept. Check Title text above to enable its custom font and colour."));backgroundNote->setWordWrap(true);al->addWidget(backgroundNote);al->addStretch();
    auto *previewPage=new QWidget;auto *pl=new QVBoxLayout(previewPage);auto *preview=new TablePreview15;preview->model=model;preview->fallback=fallback;pl->addWidget(preview);auto *warnings=new QLabel;warnings->setWordWrap(true);warnings->setStyleSheet("color:#a04000");pl->addWidget(warnings);tabs->addTab(previewPage,QObject::tr("Preview and fit warnings"));
    auto read=[&](){QVariantMap v=value;v["enabled"]=enabled->isChecked();v["title"]=title->text();v["rows"]=rows->value();v["rowHeight"]=rowHeight->value();v["autoRowHeight"]=autoRows->isChecked();v["headerHeight"]=headerHeight->value();v["autoHeaderHeight"]=autoHeader->isChecked();v["titleHeight"]=titleHeight->value();v["paddingX"]=paddingX->value();v["paddingY"]=paddingY->value();QVariantList ws,heads,als,hals,autos,wraps,overs;for(int i=0;i<columns->rowCount();++i){heads<<columns->item(i,0)->text();ws<<static_cast<QSpinBox*>(columns->cellWidget(i,1))->value();als<<static_cast<QComboBox*>(columns->cellWidget(i,2))->currentData();hals<<static_cast<QComboBox*>(columns->cellWidget(i,3))->currentData();autos<<static_cast<QComboBox*>(columns->cellWidget(i,4))->currentData();wraps<<static_cast<QCheckBox*>(columns->cellWidget(i,5))->isChecked();overs<<static_cast<QComboBox*>(columns->cellWidget(i,6))->currentData();}v["widths"]=ws;v["headers"]=heads;v["alignments"]=als;v["headerAlignments"]=hals;v["autoWidths"]=autos;v["wraps"]=wraps;v["overflowModes"]=overs;v["titleStyle"]=ts->value();v["headerStyle"]=hs->value();v["bodyStyle"]=bs->value();v["columnOverrides17"]=columnOverrides;v["rowOverrides17"]=rowOverrides;v["rowHeights18"]=rowHeights;v["cellOverrides20"]=cellOverrides;
    for(auto it=colours.cbegin();it!=colours.cend();++it)v[it.key()]=it.value()->property("colour");
    return v;};
    auto *individual=new QWidget;individual->setObjectName("individualStyles17");auto *il=new QVBoxLayout(individual);tabs->addTab(individual,QObject::tr("Individual columns and rows"));
    auto *info=new QLabel(QObject::tr("Styles apply to this table object only. Inheritance: table, then column, then body row. Row overrides follow row numbers; sorting or refreshing can put a different item at that row. Use Preview to inspect the result."));info->setWordWrap(true);il->addWidget(info);
    auto *cg=new QGroupBox(QObject::tr("Individual column"));auto *cf=new QFormLayout(cg);auto *styleColumn=new QComboBox;styleColumn->setObjectName("styleColumn17");for(int c=0;c<model->columnCount();++c)styleColumn->addItem(QObject::tr("%1 — %2").arg(c+1).arg(columnHeading(value,model,c)),c);cf->addRow(QObject::tr("Column"),styleColumn);auto *columnSummary=new QLabel;columnSummary->setWordWrap(true);cf->addRow(columnSummary);auto *ca=new QHBoxLayout;auto *editColumn=new QPushButton(QObject::tr("Edit column fonts and colours..."));editColumn->setObjectName("editColumnStyle17");auto *resetColumn=new QPushButton(QObject::tr("Reset selected column"));resetColumn->setObjectName("resetColumnStyle17");auto *clearColumns=new QPushButton(QObject::tr("Reset all columns"));clearColumns->setObjectName("resetAllColumnStyles17");ca->addWidget(editColumn);ca->addWidget(resetColumn);ca->addWidget(clearColumns);cf->addRow(ca);il->addWidget(cg);
    auto *rg=new QGroupBox(QObject::tr("Individual body row (column headers excluded)"));auto *rf=new QFormLayout(rg);auto *styleRowNumber=new QSpinBox;styleRowNumber->setObjectName("styleRow17");styleRowNumber->setRange(1,qMax(10000,model->rowCount()));rf->addRow(QObject::tr("Body row number"),styleRowNumber);auto *heightOverride=new QCheckBox(QObject::tr("Override this row height"));heightOverride->setObjectName("rowHeightOverride18");auto *individualHeight=new QSpinBox;individualHeight->setObjectName("individualRowHeight18");individualHeight->setRange(6,10000);individualHeight->setSuffix(" du");rf->addRow(heightOverride,individualHeight);auto *fitIndividualHeight=new QPushButton(QObject::tr("Auto-fit this row now"));fitIndividualHeight->setObjectName("fitIndividualRow18");rf->addRow(fitIndividualHeight);auto *rowSummary=new QLabel;rowSummary->setWordWrap(true);rf->addRow(rowSummary);auto *ra=new QHBoxLayout;auto *editRow=new QPushButton(QObject::tr("Edit row font and colours..."));editRow->setObjectName("editRowStyle17");auto *resetRow=new QPushButton(QObject::tr("Reset selected row"));resetRow->setObjectName("resetRowStyle17");auto *clearRows=new QPushButton(QObject::tr("Reset all rows"));clearRows->setObjectName("resetAllRowStyles17");ra->addWidget(editRow);ra->addWidget(resetRow);ra->addWidget(clearRows);rf->addRow(ra);il->addWidget(rg);il->addStretch();
    auto updateSummaries=[&](){int c=styleColumn->currentIndex();auto cv=c>=0&&c<columnOverrides.size()?columnOverrides[c].toMap():QVariantMap();columnSummary->setText(overrideSummary17(cv));rowSummary->setText(overrideSummary17(rowOverrides.value(QString::number(styleRowNumber->value()-1)).toMap()));editColumn->setEnabled(c>=0);resetColumn->setEnabled(c>=0&&!cv.isEmpty());resetRow->setEnabled(rowOverrides.contains(QString::number(styleRowNumber->value()-1))||rowHeights.contains(QString::number(styleRowNumber->value()-1)));const QString key=QString::number(styleRowNumber->value()-1);QSignalBlocker blockCheck(heightOverride),blockHeight(individualHeight);const bool manual=rowHeights.contains(key);heightOverride->setChecked(manual);individualHeight->setEnabled(manual);individualHeight->setValue(manual?rowHeights.value(key).toInt():rowHeight->value());};
    QObject::connect(heightOverride,&QCheckBox::toggled,&d,[&](bool checked){QString key=QString::number(styleRowNumber->value()-1);if(checked)rowHeights[key]=individualHeight->value();else rowHeights.remove(key);individualHeight->setEnabled(checked);});
    QObject::connect(individualHeight,QOverload<int>::of(&QSpinBox::valueChanged),&d,[&](int h){if(heightOverride->isChecked())rowHeights[QString::number(styleRowNumber->value()-1)]=h;});
    QObject::connect(fitIndividualHeight,&QPushButton::clicked,&d,[&](){auto v=read();const int r=styleRowNumber->value()-1;auto g=tableLayout(v,model,0,rows->value()?rows->value():model->rowCount(),fallback);rowHeights[QString::number(r)]=fittedRowHeight18(v,model,r,g.widths,fallback);updateSummaries();});
    QObject::connect(styleColumn,QOverload<int>::of(&QComboBox::currentIndexChanged),&d,[&](int){updateSummaries();});QObject::connect(styleRowNumber,QOverload<int>::of(&QSpinBox::valueChanged),&d,[&](int){updateSummaries();});
    QObject::connect(editColumn,&QPushButton::clicked,&d,[&](){int c=styleColumn->currentIndex();if(c<0)return;while(columnOverrides.size()<=c)columnOverrides<<QVariantMap();auto next=columnOverrides[c].toMap();if(editCellOverride17(&d,next,read(),fallback,true)){columnOverrides[c]=next;updateSummaries();}});
    QObject::connect(resetColumn,&QPushButton::clicked,&d,[&](){int c=styleColumn->currentIndex();if(c>=0&&c<columnOverrides.size())columnOverrides[c]=QVariantMap();updateSummaries();});QObject::connect(clearColumns,&QPushButton::clicked,&d,[&](){columnOverrides.clear();updateSummaries();});
    QObject::connect(editRow,&QPushButton::clicked,&d,[&](){QString key=QString::number(styleRowNumber->value()-1);auto next=rowOverrides.value(key).toMap();if(editCellOverride17(&d,next,read(),fallback,false)){if(next.isEmpty())rowOverrides.remove(key);else rowOverrides[key]=next;updateSummaries();}});
    QObject::connect(resetRow,&QPushButton::clicked,&d,[&](){rowOverrides.remove(QString::number(styleRowNumber->value()-1));rowHeights.remove(QString::number(styleRowNumber->value()-1));updateSummaries();});QObject::connect(clearRows,&QPushButton::clicked,&d,[&](){rowOverrides.clear();rowHeights.clear();updateSummaries();});updateSummaries();
    auto count=[&](){return rows->value()?rows->value():model->rowCount();};
    auto refresh=[&](){preview->format=read();preview->rows=count();auto g=tableLayout(preview->format,model,0,preview->rows,fallback);auto list=g.warnings;if(availableWidth>0&&qMax<qreal>(g.width,g.bounds.right())>availableWidth)list.prepend(QObject::tr("Table width %1 exceeds the available sheet width %2 (drawing units).").arg(g.width).arg(qRound(availableWidth)));warnings->setText(list.isEmpty()?QObject::tr("All text fits. Table width: %1 du; body height: %2 du.").arg(g.width).arg(g.bodyHeight):list.join('\n'));preview->update();};
    auto fit=[&](bool selected,bool height){auto v=read();auto widths=fittedColumnWidths(v,model,0,count(),fallback);QSet<int> selectedRows;for(auto *item:columns->selectedItems())selectedRows.insert(item->row());for(int c=0;c<widths.size();++c)if(!selected||selectedRows.contains(c)){static_cast<QSpinBox*>(columns->cellWidget(c,1))->setValue(widths[c]);static_cast<QComboBox*>(columns->cellWidget(c,4))->setCurrentIndex(0);}if(height){rowHeights.clear();v=read();v["autoRowHeight"]=true;v["rowHeight"]=6;v["autoHeaderHeight"]=true;v["headerHeight"]=6;auto g=tableLayout(v,model,0,count(),fallback);int h=6;for(int n:g.rowHeights)h=qMax(h,n);rowHeight->setValue(h);autoRows->setChecked(false);headerHeight->setValue(g.headerHeight);autoHeader->setChecked(false);titleHeight->setValue(int(std::ceil(QFontMetricsF(font(v.value("titleStyle").toMap(),fallback)).height()))+2*paddingY->value());}refresh();};
    fitColumns->setEnabled(false);QObject::connect(columns,&QTableWidget::itemSelectionChanged,&d,[&](){fitColumns->setEnabled(!columns->selectedItems().isEmpty());});QObject::connect(fitColumns,&QPushButton::clicked,&d,[&](){fit(true,false);});QObject::connect(fitRows,&QPushButton::clicked,&d,[&](){rowHeights.clear();auto v=read();v["autoRowHeight"]=true;v["rowHeight"]=6;auto g=tableLayout(v,model,0,count(),fallback);int h=6;for(int n:g.rowHeights)h=qMax(h,n);rowHeight->setValue(h);autoRows->setChecked(false);refresh();});QObject::connect(fitAll,&QPushButton::clicked,&d,[&](){fit(false,true);});QObject::connect(tabs,&QTabWidget::currentChanged,&d,[&](int){refresh();});
    // A timer also catches font/colour changes made by the existing style editor.
    QTimer timer;QObject::connect(&timer,&QTimer::timeout,&d,refresh);timer.start(350);refresh();
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);outer->addWidget(buttons);QObject::connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return false;value=read();return true;
}
}
#endif
