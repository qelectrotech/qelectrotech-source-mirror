// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TEXTGEOMETRY23_H
#define QET_TEXTGEOMETRY23_H
#include <QTextDocument>
#include <QPainterPath>
#include <QAbstractTextDocumentLayout>
#include <QTextBlock>
#include <QTextLayout>
namespace QetTextGeometry23 {
// Ignore trailing space advances for legacy manually aligned word stacks.
// The source document and its whitespace are never changed.
inline QRectF trimmedDocumentRect(const QTextDocument *doc) {
    QRectF rect(QPointF(),doc->size());
    const auto block=doc->lastBlock();const auto *layout=block.layout();
    if(!layout||layout->lineCount()!=1||doc->blockCount()!=1)return rect;
    const QString text=block.text();int end=text.size();while(end>0&&text[end-1].isSpace())--end;
    if(end<text.size()) {const auto line=layout->lineAt(0);const qreal tail=line.cursorToX(text.size())-line.cursorToX(end);if(tail>=0)rect.setRight(qMax(rect.left(),rect.right()-tail));else rect.setLeft(qMin(rect.right(),rect.left()-tail));}
    return rect;
}
// One rectangle per written line: excludes document margins and trailing blanks.
// Keep line height and a small clearance; this is conservative layout checking,
// not an electrical or pixel-perfect collision test.
inline QPainterPath writtenPath(const QTextDocument *doc) {
    QPainterPath path;doc->size();auto *documentLayout=doc->documentLayout();
    for(auto block=doc->begin();block.isValid();block=block.next()) {
        const auto *layout=block.layout();if(!layout)continue;
        const auto origin=documentLayout->blockBoundingRect(block).topLeft();
        const QString text=block.text();
        for(int i=0;i<layout->lineCount();++i) {
            const auto line=layout->lineAt(i);int first=line.textStart(),last=qMin(text.size(),first+line.textLength());
            while(first<last&&text[first].isSpace())++first;while(last>first&&text[last-1].isSpace())--last;
            if(first==last)continue;
            auto rect=line.naturalTextRect();const auto a=line.cursorToX(first),b=line.cursorToX(last);
            // For mixed-direction text retain Qt's conservative visual bounds.
            if(block.textDirection()!=Qt::RightToLeft){rect.setLeft(qMin(a,b));rect.setRight(qMax(a,b));}
            if(!rect.isEmpty())path.addRect(rect.translated(origin));
        }
    }
    return path;
}
}
#endif
