// SPDX-License-Identifier: GPL-2.0-or-later
// Real item/key-event regression, linked against the application's objects.
#include <QApplication>
#include <QClipboard>
#include <QDomDocument>
#include <QFile>
#include <QFontDatabase>
#include <QGraphicsView>
#include <QGlyphRun>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMimeData>
#include <QRawFont>
#include <QSettings>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextLayout>
#include <cstdio>
#include <stdexcept>
#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetresult.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetgraphicsitem/dynamicelementtextitem.h"
#include "../../sources/qetgraphicsitem/independenttextitem.h"

static void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
static void key(Diagram *scene, QGraphicsItem *item, int code, Qt::KeyboardModifiers modifiers) {
	QKeyEvent event(QEvent::KeyPress,code,modifiers); scene->sendEvent(item,&event);
}
static void redo(Diagram *scene, QGraphicsItem *item) {
	// Qt uses Ctrl+Y on Windows and Ctrl+Shift+Z on Linux.
	const QKeySequence shortcut(QKeySequence::Redo);
	check(shortcut.count()==1,"platform redo shortcut is a single key combination");
	const auto combination=shortcut[0];
	QKeyEvent event(QEvent::KeyPress,combination.key(),combination.keyboardModifiers());
	check(event.matches(QKeySequence::Redo),"generated key matches platform Redo");
	scene->sendEvent(item,&event);
}
static void clipboard(const QString &html, const QString &plain = QString(), bool withPlain = false) {
	auto *mime=new QMimeData; mime->setHtml(html); if(withPlain) mime->setText(plain); QApplication::clipboard()->setMimeData(mime);
}
static void select(QGraphicsTextItem *item,int start,int end) {
	auto cursor=item->textCursor(); cursor.setPosition(start); cursor.setPosition(end,QTextCursor::KeepAnchor); item->setTextCursor(cursor);
}
static void fieldStyle(QGraphicsTextItem *item,int start,int length,const QFont &field) {
	item->document()->documentLayout(); item->boundingRect();
	for(int i=start;i<start+length;++i) {
		if(item->toPlainText().at(i)=='\n') continue;
		QTextCursor cursor(item->document()); cursor.setPosition(i); cursor.setPosition(i+1,QTextCursor::KeepAnchor);
		const auto format=cursor.charFormat(); const QFont resolved=format.font().resolve(item->document()->defaultFont());
		check(resolved.family()==field.family()&&qAbs(resolved.pointSizeF()-field.pointSizeF())<0.001,"inserted character resolves to field font/size");
		check(!format.hasProperty(QTextFormat::ForegroundBrush)&&!format.isAnchor(),"clipboard colour/link discarded");
		const auto block=cursor.block(); const auto runs=block.layout()->glyphRuns(i-block.position(),1);
		check(!runs.isEmpty(),"inserted character laid out");
		for(const auto &run:runs) check(qAbs(run.rawFont().pixelSize()-QRawFont::fromFont(field).pixelSize())<0.01,"actual rendered font size follows field");
	}
}
int main(int argc,char **argv) {
	QApplication app(argc,argv); std::freopen(argv[2],"w",stdout);
	QTemporaryDir settings; QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path()); QCoreApplication::setOrganizationName("QETPlainPasteRegression");
	QETProject::setBackupEnabled(false); QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");
	try {
		const QString output=QString::fromLocal8Bit(argv[3]);
		QETProject project(QString::fromLocal8Bit(argv[1])); check(project.state()==QETProject::Ok,"fixture opens");
		auto *scene=project.diagrams().first(); auto *text=scene->elements().first()->dynamicTextItems().first();
		text->setTextFrom(DynamicElementTextItem::UserText);
		const QFont field("Liberation Sans",11); text->setFont(field);
		QGraphicsView view(scene); view.resize(400,300); view.show(); view.activateWindow(); QApplication::setActiveWindow(&view); view.setFocus(); QApplication::processEvents();
		auto edit=[&] { text->setTextInteractionFlags(Qt::TextEditorInteraction); text->setFocus(); scene->setFocus(); QApplication::processEvents(); check(text->hasFocus(),"real editing focus"); };
		auto paste=[&] { key(scene,text,Qt::Key_V,Qt::ControlModifier); };
		const QString rich="<p><a href='https://example.org'><span style='font-family:Courier New;font-size:40pt;color:#ff0000'><b><i>new</i></b></span></a></p><p>line</p>";
		text->setText("old"); edit(); select(text,0,3); clipboard(rich); paste();
		check(text->toPlainText()=="new\nline","HTML-only clipboard converts to plain with newlines"); fieldStyle(text,0,8,field);
		key(scene,text,Qt::Key_Z,Qt::ControlModifier); check(text->toPlainText()=="old","native undo is one paste step");
		redo(scene,text); check(text->toPlainText()=="new\nline","native redo"); fieldStyle(text,0,8,field);
		scene->clearFocus(); QApplication::processEvents(); check(text->text()=="new\nline","editing session updates stored user text");
		scene->undoStack().undo(); check(text->toPlainText()=="old","project undo"); scene->undoStack().redo(); check(text->toPlainText()=="new\nline","project redo");
		std::puts("PASS: HTML-only paste, newlines, field font/size, native and project undo/redo");
		text->setText("AB"); text->setHtml("<p style='font-family:Courier New;font-size:36pt'><b><i>AB</i></b></p>");
		edit(); select(text,2,2); clipboard("<span style='font-size:50pt'>ignored</span>","tail",true); paste();
		check(text->toPlainText()=="ABtail","clipboard plain representation takes precedence"); fieldStyle(text,2,4,field);
		QTextCursor prior(text->document()); prior.setPosition(0); prior.setPosition(1,QTextCursor::KeepAnchor); check(prior.charFormat().fontWeight()>=QFont::Bold,"existing preceding format untouched");
		select(text,1,4); clipboard("<span style='font-size:50pt'>Z</span>"); paste(); check(text->toPlainText()=="AZil","selection replaced in formatted document"); fieldStyle(text,1,1,field);
		key(scene,text,Qt::Key_Z,Qt::ControlModifier); check(text->toPlainText()=="ABtail","selection paste undo");
		redo(scene,text); check(text->toPlainText()=="AZil","selection paste redo");
		scene->clearFocus(); QApplication::processEvents();
		std::puts("PASS: paste after formatted text, plain/HTML precedence, selection replacement and undo/redo");
		text->setText("before"); edit(); select(text,0,6); clipboard("<b>ignored</b>","one\ntwo\nthree",true); paste(); fieldStyle(text,0,13,field); scene->clearFocus(); QApplication::processEvents();
		project.setFilePath(output+"/plain.qet"); check(project.write().isOk(),"save project");
		QDomDocument xml; QFile file(project.filePath()); check(file.open(QIODevice::ReadOnly),"read saved project"); check(bool(xml.setContent(file.readAll())),"parse saved project");
		bool found=false; const auto nodes=xml.elementsByTagName("dynamic_elmt_text");
		for(int i=0;i<nodes.size();++i) { auto node=nodes.at(i).toElement(); if(node.attribute("uuid")==text->uuid().toString()) { found=true; check(node.firstChildElement("html").isNull(),"no new HTML persistence"); check(node.firstChildElement("text").text()=="one\ntwo\nthree","existing plain XML text"); } }
		check(found,"attached text saved"); QETProject reopened(project.filePath()); check(reopened.state()==QETProject::Ok,"reopen project");
		DynamicElementTextItem *loaded=nullptr; for(auto *element:reopened.diagrams().first()->elements()) for(auto *item:element->dynamicTextItems()) if(item->uuid()==text->uuid()) loaded=item;
		check(loaded&&loaded->toPlainText()=="one\ntwo\nthree"&&loaded->font()==field,"plain text and field font survive reopen"); fieldStyle(loaded,0,13,field);
		std::puts("PASS: multiline clipboard, unchanged XML representation and save/reopen");
		QKeyEvent origin(QEvent::KeyPress,Qt::Key_V,Qt::ControlModifier|Qt::ShiftModifier); check(!origin.matches(QKeySequence::Paste),"Ctrl+Shift+V excluded from standard Paste");
		edit(); const QString before=text->toPlainText(); clipboard("<b>do not paste</b>"); scene->sendEvent(text,&origin); check(text->toPlainText()==before,"Ctrl+Shift+V not intercepted"); scene->clearFocus();
		auto *free=new IndependentTextItem; scene->addItem(free); free->setPlainText("free"); free->setTextInteractionFlags(Qt::TextEditorInteraction); free->setFocus(); scene->setFocus(); QApplication::processEvents(); select(free,0,4);
		clipboard("<p><b>bold</b> <i>italic</i> <span style='font-size:20pt'>large</span></p>"); key(scene,free,Qt::Key_V,Qt::ControlModifier);
		check(free->document()->find("bold").charFormat().fontWeight()>=QFont::Bold&&free->document()->find("italic").charFormat().fontItalic()&&free->document()->find("large").charFormat().fontPointSize()==20,"free-text rich paste unchanged");
		scene->clearFocus(); QApplication::processEvents(); project.setFilePath(output+"/free.qet"); check(project.write().isOk(),"save free text");
		QETProject freeReopened(project.filePath()); check(freeReopened.state()==QETProject::Ok,"reopen free text"); bool freeFound=false;
		for(auto *item:freeReopened.diagrams().first()->items()) if(auto *loadedFree=dynamic_cast<IndependentTextItem *>(item)) if(loadedFree->uuid()==free->uuid()) { freeFound=true; check(loadedFree->document()->find("bold").charFormat().fontWeight()>=QFont::Bold&&loadedFree->document()->find("italic").charFormat().fontItalic()&&loadedFree->document()->find("large").charFormat().fontPointSize()==20,"free-text formatting survives reopen"); }
		check(freeFound,"free text saved"); std::puts("PASS: Ctrl+Shift+V excluded; free-text rich paste and reopen unchanged");
	} catch(const std::exception &error) { std::printf("FAIL: %s\n",error.what()); return 1; }
	return 0;
}
