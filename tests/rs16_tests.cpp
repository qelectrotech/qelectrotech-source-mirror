// SPDX-License-Identifier: GPL-2.0-or-later
// Production-code integration checks for multilingual projects and text layout.
#include <QtWidgets>
#include <QtPrintSupport>
#include <iostream>
#include "qetproject.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "diagram.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "ui/languagecatalog16.h"
#include "ui/languageplacement16.h"
#include "ui/languageui16.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "ui/languagephrases16.h"
#include "titleblocktemplate.h"
#include "titleblocktemplaterenderer.h"
#include "print/printdrawing.h"

namespace {
int checks=0;
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);++checks;}
QVariantMap catalog(const QString &id="note-1") {
 QFont smaller("DejaVu Sans",9);
 QVariantMap de{{"text",QString::fromUtf8("Magnetischer Sicherheitsschalter, \"Tür\"\nmit Zuhaltung")},{"font",smaller.toString()},{"width",115.0},{"maxLines",8},{"side","left"},{"gap",8.0},{"auto",true}};
 QVariantMap ja{{"text",QString::fromUtf8("磁気安全スイッチ")},{"width",100.0}};
 QVariantMap ar{{"text",QString::fromUtf8("مفتاح الأمان المغناطيسي")},{"width",110.0},{"direction","rtl"}};
 QVariantMap e{{"source","Magnetic safety switch"},{"context","Sheet 5 / -S4 / description"},{"kind","independent"},{"enabled",true},{"baseX",100.0},{"baseY",120.0},{"variants",QVariantMap{{"de",de},{"ja",ja},{"ar",ar}}}};
 return {{"sourceLanguage","en"},{"activeLanguage","de"},{"languages",QStringList{"en","de","ja","ar","es","nl"}},{"entries",QVariantMap{{id,e}}}};
}
void catalogChecks(const QString &out) {
 using namespace QetLanguages16;
 auto data=catalog();
 check(languageCodes(data).size()==6,"one project accepts six languages");
 check(resolvedText(data,"note-1","Magnetic safety switch").contains(QString::fromUtf8("Tür")),"active language selects translated text");
 check(resolvedText(data,"unknown","Original")=="Original","unregistered text remains unchanged");
 check(resolvedText(data,"note-1","Original")==variant(data,"note-1").value("text").toString(),"translation is linked by stable identity");
 data["activeLanguage"]="es";check(resolvedText(data,"note-1","Original")=="Original","missing translation falls back to source");
 check(!hasTranslation(data,"note-1"),"missing translation can be reported");
 data["activeLanguage"]="en";check(resolvedText(data,"note-1","Original")=="Original","source language displays the original");
 auto entries=data.value("entries").toMap();auto e=entries["note-1"].toMap();e["enabled"]=false;entries["note-1"]=e;data["entries"]=entries;data["activeLanguage"]="de";
 check(resolvedText(data,"note-1","Original")=="Original","disabled identity is protected from translation");
 data=catalog();auto staleEntries=data.value("entries").toMap();auto staleEntry=staleEntries["note-1"].toMap();auto staleVariants=staleEntry.value("variants").toMap();auto staleDe=staleVariants["de"].toMap();staleDe["stale"]=true;staleVariants["de"]=staleDe;staleEntry["variants"]=staleVariants;staleEntries["note-1"]=staleEntry;data["entries"]=staleEntries;
 check(resolvedText(data,"note-1","Original")=="Original"&&!hasTranslation(data,"note-1"),"stale translation falls back and requires review");
 auto confirmed=importCsv(data,exportCsv(catalog()));check(confirmed.ok&&hasTranslation(confirmed.data,"note-1"),"matching-source CSV import confirms a stale translation");
 data=catalog();QDomDocument doc;auto xml=toXml(doc,data);check(QJsonDocument::fromVariant(fromXml(xml))==QJsonDocument::fromVariant(data),"catalog and per-language layout XML roundtrip");
 auto unknownVersion=xml.cloneNode().toElement();unknownVersion.setAttribute("version",99);check(fromXml(unknownVersion).isEmpty(),"unsupported catalog version is not misread");
 auto malformed=doc.createElement("languages-v16");malformed.appendChild(doc.createTextNode("broken JSON"));check(fromXml(malformed).isEmpty(),"malformed catalog does not become partial state");
 const auto csv=exportCsv(data);check(csv.startsWith(QByteArray("\xEF\xBB\xBF")),"CSV is UTF-8 with Excel-compatible BOM");
 QList<QStringList> rows;QString error;check(parseCsv(QString::fromUtf8(csv),rows,&error),"exported CSV parses");
 check(rows.size()==2&&rows[1][3]=="Magnetic safety switch","CSV includes source-language column");
 check(rows[1][4]==variant(data,"note-1","de").value("text").toString(),"CSV preserves quoted commas, quotes and newlines");
 check(rows[1][5]==QString::fromUtf8("磁気安全スイッチ")&&rows[1][6].contains(QString::fromUtf8("مفتاح")),"CSV preserves Japanese and Arabic");
 auto empty=data;auto all=empty.value("entries").toMap();auto item=all["note-1"].toMap();item["variants"]=QVariantMap();all["note-1"]=item;empty["entries"]=all;
 auto imported=importCsv(empty,csv);check(imported.ok&&imported.imported==3,"CSV imports three translated variants");
 check(variant(imported.data,"note-1","de").value("text")==variant(data,"note-1","de").value("text"),"CSV restores translated newlines exactly");
 check(variant(imported.data,"note-1","ar").value("text")==variant(data,"note-1","ar").value("text"),"CSV restores right-to-left text exactly");
 auto blanks=importCsv(data,exportCsv(empty));check(blanks.ok&&variant(blanks.data,"note-1","de").value("text")==variant(data,"note-1","de").value("text"),"blank CSV cells preserve existing translations by default");
 auto cleared=importCsv(data,exportCsv(empty),true);check(cleared.ok&&variant(cleared.data,"note-1","de").value("text").toString().isEmpty(),"explicit blank overwrite can clear a translation");
 auto unknownCsv=csv;unknownCsv.replace("note-1","unknown-id");auto unknown=importCsv(data,unknownCsv);check(unknown.ok&&unknown.imported==0&&!unknown.warnings.isEmpty(),"unknown CSV identity is reviewed and skipped");
 auto changedCsv=csv;changedCsv.replace("Magnetic safety switch","Changed source");auto changed=importCsv(data,changedCsv);check(changed.ok&&changed.imported==0&&!changed.warnings.isEmpty(),"changed source is reviewed rather than silently relinked");
 auto bad=importCsv(data,QByteArray("ID,Source,de\n\"unterminated"));check(!bad.ok&&!bad.error.isEmpty(),"malformed CSV is rejected");
 auto invalidUtf=importCsv(data,QByteArray("ID,Source,de\n1,Source,\xff"));check(!invalidUtf.ok,"invalid UTF-8 is rejected");
 auto semicolon=importCsv(data,QByteArray("ID;Source;de\r\nnote-1;Magnetic safety switch;Sicherheitsschalter\r\n"));check(semicolon.ok&&semicolon.imported==1,"semicolon Excel CSV imports");
 check(variant(semicolon.data,"note-1","de").value("font")==variant(data,"note-1","de").value("font"),"translation import retains per-text font/layout overrides");
 const QByteArray extra("ID,Source,nb-NO\r\nnote-1,Magnetic safety switch,Magnetisk sikkerhetsbryter\r\n");auto additional=importCsv(data,extra);check(additional.ok&&languageCodes(additional.data).contains("nb-NO"),"import adds another language without a fixed language limit");
 auto placeholder=catalog();auto pe=placeholder.value("entries").toMap();auto ph=pe["note-1"].toMap();ph["source"]="Voltage %{voltage}";pe["note-1"]=ph;placeholder["entries"]=pe;
 auto missingPlaceholder=importCsv(placeholder,QByteArray("ID,Source,de\r\nnote-1,Voltage %{voltage},Spannung\r\n"));check(missingPlaceholder.ok&&missingPlaceholder.imported==0&&!missingPlaceholder.warnings.isEmpty(),"translation cannot silently drop a substitution placeholder");
 auto goodPlaceholder=importCsv(placeholder,QByteArray("ID,Source,de\r\nnote-1,Voltage %{voltage},Spannung %{voltage}\r\n"));check(goodPlaceholder.ok&&goodPlaceholder.imported==1,"translation can retain substitution placeholders");
 check(validLanguageCode("pt-BR")&&validLanguageCode("zh-Hant")&&!validLanguageCode("German language"),"language identifiers accept locale/script codes");
 QFile exportFile(out+"/translations-six-languages.csv");check(exportFile.open(QIODevice::WriteOnly),"create reusable translation-list example");exportFile.write(csv);exportFile.close();
 QETProject project;QETApp::registerProject(&project);project.addNewDiagram();int signalCount=0;QObject::connect(&project,&QETProject::languageDataChanged,[&](){++signalCount;});project.setLanguageData(data);check(signalCount==1&&project.languageData()==data,"project language change emits update signal");
 const QString path=out+"/Multilingual_Test.qet";QFile file(path);check(file.open(QIODevice::WriteOnly),"save multilingual project fixture");file.write(project.toXml().toByteArray(2));file.close();
 QETProject reopened(path);QETApp::registerProject(&reopened);check(reopened.state()==QETProject::Ok,"multilingual project reopens");check(QJsonDocument::fromVariant(reopened.languageData())==QJsonDocument::fromVariant(data),"full project save/reopen retains all languages and per-text rules");QETApp::unregisterProject(&reopened);QETApp::unregisterProject(&project);
}
void placementChecks() {
 using namespace QetPlacement16;
 QGraphicsScene scene;auto *anchor=scene.addRect(QRectF(200,150,70,70),QPen(Qt::black),QBrush(Qt::white));
 auto *text=scene.addText("Description",QFont("DejaVu Sans",10));text->setPos(205,95);
 auto *wire=scene.addLine(0,110,500,110,QPen(Qt::black,1));
 const QRectF sheet(0,0,520,360);Rule fixed;fixed.automatic=false;
 auto crossed=inspect(text,anchor,fixed,sheet);check(crossed.collisions>0&&!crossed.resolved,"collision review detects a conductor crossing text");
 Rule automatic;automatic.side="left";automatic.gap=10;auto before=text->pos();auto candidate=place(text,anchor,automatic,sheet,false);
 check(text->pos()==before,"placement preview does not move text");check(candidate.collisions==0&&candidate.resolved&&candidate.candidatesEvaluated>1,"bounded search finds a free position near the symbol");
 auto placed=place(text,anchor,automatic,sheet);check(placed.moved&&text->pos()==placed.position,"automatic placement applies its candidate");
 check(inspect(text,anchor,fixed,sheet).collisions==0,"placed text is clear of the conductor");
 text->setPos(205,95);Rule locked=automatic;locked.locked=true;auto lockedResult=place(text,anchor,locked,sheet);
 check(text->pos()==QPointF(205,95)&&!lockedResult.moved,"locked text keeps the manual position");check(!lockedResult.resolved&&!lockedResult.issues.isEmpty(),"locked overlap remains visible in review");
 Rule permitted=fixed;permitted.allowOverlap=true;auto allowed=inspect(text,anchor,permitted,sheet);check(allowed.collisions>0&&allowed.permittedOverlap&&allowed.resolved,"explicitly allowed overlap is reported as permitted");
 auto outside=inspect(text,anchor,permitted,QRectF(0,0,40,40));check(outside.outsideBounds&&!outside.resolved,"allowed overlap does not permit leaving the sheet");
 text->setTextWidth(25);text->setPlainText("one two three four five six seven eight nine ten");Rule limit=fixed;limit.maxLines=2;
 auto limited=inspect(text,anchor,limit,sheet);check(limited.lineCount>2&&!limited.resolved,"line limit is reviewed when text needs more lines");check(text->toPlainText().endsWith("ten"),"line limit never truncates the text");
 auto impossible=place(text,anchor,automatic,QRectF(0,0,10,10));check(!impossible.resolved&&impossible.outsideBounds,"no-space case is left unresolved rather than falsely accepted");
 scene.removeItem(wire);delete wire;
 // A rotated/transformed parent still receives parent-local coordinates.
 QGraphicsScene transformed;auto *parent=transformed.addRect(0,0,60,40);parent->setPos(180,120);parent->setRotation(25);
 auto *child=new QGraphicsTextItem("Rotated parent",parent);child->setPos(0,-30);
 auto transformResult=place(child,parent,automatic,QRectF(0,0,600,400));
 check(transformResult.resolved&&child->pos()==transformResult.position,"placement handles transformed parent coordinates");
}
void renderingChecks(const QString &out) {
 QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *note=new IndependentTextItem("Magnetic safety switch");page->addItem(note);note->setPos(100,120);note->setFont(QFont("DejaVu Sans",12));
 auto *untouched=new IndependentTextItem("-K1 / 13-14");page->addItem(untouched);untouched->setPos(400,120);untouched->setFont(QFont("DejaVu Sans",12));
 const QString id=note->languageId16();check(!id.isEmpty(),"placed text has a stable translation identity");
 const QString originalHtml=note->toHtml();const QFont originalFont=note->font();const qreal originalWidth=note->textWidth();QDomDocument doc;const auto before=note->toXml(doc);const auto sourceBounds=note->boundingRect();
 auto data=catalog(id);auto es=data.value("entries").toMap();auto e=es[id].toMap();auto vs=e.value("variants").toMap();auto de=vs["de"].toMap();de["x"]=300.0;de["y"]=230.0;de["auto"]=false;de["locked"]=true;vs["de"]=de;e["variants"]=vs;es[id]=e;data["entries"]=es;
 QetLanguagesUi16::apply(&project,data,false);note->refreshLanguage16();
 check(note->displayedText16()==de.value("text").toString(),"language switch displays the translated description");
 check(note->hasLanguageDisplay16()&&note->languageDocument16()!=nullptr,"translated display uses a separate document");
 check(note->languageLineCount16()>=3,"translated text wraps onto several lines");
 check(note->boundingRect().width()<=116&&note->boundingRect().height()>sourceBounds.height(),"translated bounds use the configured wrapping width and expanded height");
 check(note->languageDocument16()->defaultFont().pointSize()==9,"per-label font-size override affects translated display");
 check(untouched->displayedText16()=="-K1 / 13-14"&&!untouched->hasLanguageDisplay16(),"unregistered device and pin identifiers remain unchanged");
 check(note->toHtml()==originalHtml&&note->font()==originalFont&&note->textWidth()==originalWidth,"translation does not modify source HTML, font or text width");
 auto translatedXml=note->toXml(doc);check(translatedXml.attribute("text")==before.attribute("text")&&translatedXml.attribute("font")==before.attribute("font"),"save in another language retains source text and font attributes");
 check(note->pos()==QPointF(300,230),"language-specific manual position is applied");
 auto ar=data;ar["activeLanguage"]="ar";QetLanguagesUi16::apply(&project,ar,false);note->refreshLanguage16();
 check(note->displayedText16()==QString::fromUtf8("مفتاح الأمان المغناطيسي"),"Arabic language can be selected");
 check(note->languageDocument16()->defaultTextOption().textDirection()==Qt::RightToLeft,"Arabic layout uses the requested right-to-left direction");
 check(note->toHtml()==originalHtml,"right-to-left display preserves the source document");
 auto missing=data;missing["activeLanguage"]="es";QetLanguagesUi16::apply(&project,missing,false);note->refreshLanguage16();check(note->displayedText16()=="Magnetic safety switch","a missing language falls back visibly to the original");
 auto source=data;source["activeLanguage"]="en";QetLanguagesUi16::apply(&project,source,false);note->refreshLanguage16();
 check(note->pos()==QPointF(100,120),"returning to source language restores the source position");
 check(note->boundingRect()==sourceBounds&&!note->hasLanguageDisplay16(),"returning to source restores original text geometry");
 check(note->toHtml()==originalHtml,"repeated language switching leaves source rich text unchanged");
 // Simply opening and applying the source-language dialog must not create
 // width/font overrides from the controls' displayed default values.
 {QetLanguagesUi16::Dialog dialog(&project);auto *buttons=dialog.findChild<QDialogButtonBox*>();buttons->button(QDialogButtonBox::Apply)->click();dialog.reject();}
 check(note->boundingRect()==sourceBounds&&note->font()==originalFont&&note->textWidth()==originalWidth,"untouched source-language Apply preserves original geometry, font and width");
 auto untouchedSource=QetLanguages16::variant(project.languageData(),id,"en");check(!untouchedSource.contains("width")&&!untouchedSource.contains("font")&&!note->hasLanguageDisplay16(),"untouched source-language controls do not create display overrides");
 // CSV-imported text-only variants inherit width, manual placement and font.
 auto textOnly=data;auto textOnlyEntries=textOnly.value("entries").toMap();auto textOnlyEntry=textOnlyEntries[id].toMap();auto textOnlyVariants=textOnlyEntry.value("variants").toMap();textOnlyVariants["de"]=QVariantMap{{"text",de.value("text")}};textOnlyEntry["variants"]=textOnlyVariants;textOnlyEntries[id]=textOnlyEntry;textOnly["entries"]=textOnlyEntries;
 const auto normalizedFree=QetLanguagesUi16::apply(&project,textOnly,false);auto freeVariant=QetLanguages16::variant(normalizedFree,id,"de");
 check(!freeVariant.contains("width")&&!freeVariant.value("auto").toBool(),"text-only free-text translation inherits wrapping and retains manual placement by default");
 check(!freeVariant.contains("font")&&note->font()==originalFont,"import defaults inherit the individual source font without a global font change");
 // Display-only font changes also work for the source language without replacing text.
 auto sourceOverride=source;auto overrideEntries=sourceOverride.value("entries").toMap();auto overrideEntry=overrideEntries[id].toMap();auto overrideVariants=overrideEntry.value("variants").toMap();overrideVariants["en"]=QVariantMap{{"font",QFont("DejaVu Sans",8).toString()},{"width",90.0}};overrideEntry["variants"]=overrideVariants;overrideEntries[id]=overrideEntry;sourceOverride["entries"]=overrideEntries;
 QetLanguagesUi16::apply(&project,sourceOverride,false);note->refreshLanguage16();check(note->displayedText16()=="Magnetic safety switch"&&note->languageDocument16()->defaultFont().pointSize()==8,"source-language label can have an individual layout/font override");
 check(untouched->font()==originalFont,"individual font override does not change another label's font");
 // Render the actual scene through the vector print helper, then save an editable fixture.
 QetLanguagesUi16::apply(&project,data,false);note->refreshLanguage16();
 QImage image(1100,760,QImage::Format_ARGB32);image.fill(Qt::white);{QPainter p(&image);page->render(&p,QRectF(0,0,1100,760),QRectF(0,0,1100,760));}check(image.save(out+"/german-wrapping.png"),"render translated wrapping preview");
 {QPdfWriter pdf(out+"/german-translated.pdf");pdf.setResolution(600);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);QPainter p(&pdf);QetPrint::render(page,&p,QRectF(0,0,p.device()->width(),p.device()->height()),QRectF(0,0,1100,760));}
 const QString path=out+"/Multilingual_Layout_Test.qet";QFile file(path);check(file.open(QIODevice::WriteOnly),"save translated editable project fixture");file.write(project.toXml().toByteArray(2));file.close();
 QETProject reopened(path);QETApp::registerProject(&reopened);check(reopened.state()==QETProject::Ok,"translated-layout project reopens");
 IndependentTextItem *restored=nullptr;for(auto *d:reopened.diagrams())for(auto *item:d->items())if(auto *text=dynamic_cast<IndependentTextItem*>(item))if(text->languageId16()==id)restored=text;
 check(restored!=nullptr,"stable text identity survives project save/reopen");restored->refreshLanguage16();
 check(restored->displayedText16()==de.value("text").toString()&&restored->toPlainText()=="Magnetic safety switch","reopened project retains translated display and original source separately");
 QETApp::unregisterProject(&reopened);
 // A drag made outside the language dialog belongs to the active language.
 // Scanning captures it as a manual lock instead of discarding the correction.
 auto manual=project.languageData();auto manualEntries=manual.value("entries").toMap();auto manualEntry=manualEntries[id].toMap();auto manualVariants=manualEntry.value("variants").toMap();auto manualArabic=manualVariants["ar"].toMap();manualArabic["font"]=QFont("DejaVu Sans",11).toString();manualVariants["ar"]=manualArabic;manualEntry["variants"]=manualVariants;manualEntries[id]=manualEntry;manual["entries"]=manualEntries;
 QetLanguagesUi16::apply(&project,manual,false);note->setPos(420,310);QetLanguagesUi16::scan(&project,manual);
 const auto captured=QetLanguages16::variant(manual,id,"de");check(captured.value("x").toDouble()==420&&captured.value("y").toDouble()==310,"scan remembers an active-language manual drag");
 check(captured.value("locked").toBool(),"manual drag becomes a locked language-specific placement");
 check(captured.value("font")==de.value("font")&&QetLanguages16::variant(manual,id,"ar").value("font")==manualArabic.value("font"),"capturing a manual position preserves fonts in every language");
 auto manualSource=manual;manualSource["activeLanguage"]="en";QetLanguagesUi16::apply(&project,manualSource,false);check(note->pos()==QPointF(100,120),"manual translated position does not replace the source position");
 QetLanguagesUi16::apply(&project,manual,false);check(note->pos()==QPointF(420,310),"returning to the translated language restores the captured manual position");
 // The project dialog previews on the scene but Cancel restores the original
 // language and positions; Apply creates one project-wide undo operation.
 project.setModified(false);check(!project.projectOptionsWereModified(),"prepare a clean project-options state for Cancel");
 const auto beforeDialog=project.languageData();const auto beforeDialogPos=note->pos();
 {QetLanguagesUi16::Dialog dialog(&project);auto *grid=dialog.findChild<QTableWidget*>("translations16");check(grid!=nullptr,"language dialog exposes translation catalog");int row=-1;for(int r=0;r<grid->rowCount();++r)if(grid->item(r,2)->text()=="Magnetic safety switch")row=r;check(row>=0,"existing description appears in project translation dialog");grid->setCurrentCell(row,3);grid->item(row,3)->setText("Unapplied translation");QPushButton *previewButton=nullptr;for(auto *button:dialog.findChildren<QPushButton*>())if(button->text().startsWith("Preview language"))previewButton=button;check(previewButton!=nullptr,"language dialog offers a project preview");previewButton->click();check(note->displayedText16()=="Unapplied translation","preview temporarily changes the drawing before Cancel");dialog.reject();}
 check(project.languageData()==beforeDialog&&note->pos()==beforeDialogPos,"Cancel restores language catalog and original positions");
 check(!project.projectOptionsWereModified(),"Cancel leaves previously clean project options unmodified");
 {QetLanguagesUi16::Dialog dialog(&project);auto *grid=dialog.findChild<QTableWidget*>("translations16");int row=-1;for(int r=0;r<grid->rowCount();++r)if(grid->item(r,2)->text()=="Magnetic safety switch")row=r;grid->setCurrentCell(row,3);grid->item(row,3)->setText("Angewendeter Sicherheitsschalter");dialog.grab().save(out+"/project-languages-dialog.png");auto *buttons=dialog.findChild<QDialogButtonBox*>();check(buttons&&buttons->button(QDialogButtonBox::Apply),"language dialog provides explicit Apply");buttons->button(QDialogButtonBox::Apply)->click();check(note->displayedText16()=="Angewendeter Sicherheitsschalter","Apply updates the selected translation on the drawing");dialog.reject();}
 check(note->displayedText16()=="Angewendeter Sicherheitsschalter","Cancel after Apply retains the committed translation");page->undoStack().undo();check(project.languageData()==beforeDialog&&note->pos()==beforeDialogPos,"one Undo restores prior language data and positions");page->undoStack().redo();check(note->displayedText16()=="Angewendeter Sicherheitsschalter","Redo restores the committed language change");
 QETApp::unregisterProject(&project);
}
void phraseChecks() {
 using namespace QetLanguages16;QETProject project;QETApp::registerProject(&project);project.addNewDiagram();ProjectDBModel model(&project);model.setIdentifier("nomenclature");
 const QString sql="SELECT 1 AS 'No.', 'Safety relay' AS 'Description', 'Safety relay' AS 'Reference', 'Safety relay' AS 'Article'";model.setQuery(sql);
 QVariantMap data{{"sourceLanguage","en"},{"activeLanguage","de"},{"languages",QStringList{"en","de"}}};
 QString description=registerPhrase(data,"Safety relay","BOM description"),header=registerPhrase(data,"Description","BOM heading");auto entries=data.value("entries").toMap();
 for(auto pair:QList<QPair<QString,QString>>{{description,"Sicherheitsrelais"},{header,"Beschreibung"}}){auto e=entries[pair.first].toMap();e["variants"]=QVariantMap{{"de",QVariantMap{{"text",pair.second}}}};entries[pair.first]=e;}data["entries"]=entries;project.setLanguageData(data);
 check(model.index(0,1).data().toString()=="Sicherheitsrelais","BOM description displays the project language");check(model.headerData(1,Qt::Horizontal).toString()=="Beschreibung","BOM heading displays its translation");
 check(model.index(0,1).data(SourceTextRole16).toString()=="Safety relay","table exposes its source text for translation catalog collection");
 check(model.index(0,2).data().toString()=="Safety relay"&&model.index(0,3).data().toString()=="Safety relay","reference and article columns remain literal even when an identical phrase is translated");
 check(model.queryString()==sql,"table language change preserves its source SQL");
 check(registerPhrase(data,"-K1","reference").isEmpty()&&registerPhrase(data,"24 VDC","value").isEmpty(),"phrase collection excludes identifiers and physical values");
 check(!isHumanPhrase("-K1 /13-14")&&!isHumanPhrase("24 Vdc"),"spaced device references and mixed-case electrical values are protected");
 check(isHumanPhrase("SAFETY RELAY"),"uppercase descriptive words remain eligible for translation");
 data["activeLanguage"]="en";project.setLanguageData(data);check(model.index(0,1).data().toString()=="Safety relay","table source language restores its original description");
 QETApp::unregisterProject(&project);
}
void titleBlockChecks(const QString &out) {
 using namespace QetLanguages16;QDomDocument definition;
 check(definition.setContent(QString("<titleblocktemplate name='language-test'><information/><logos/><grid cols='t100%;' rows='35;'><field valign='center' row='0' col='0' name='author' displaylabel='true' align='left'><value><translation lang='en'>%author</translation></value><label><translation lang='en'>Author</translation></label></field></grid></titleblocktemplate>")),"create title-block translation fixture");
 TitleBlockTemplate title;check(title.loadFromXmlElement(definition.documentElement()),"load title-block fixture");QDomDocument beforeDoc;auto before=beforeDoc.createElement("titleblocktemplate");title.saveToXmlElement(before);const QString original=before.text();
 QVariantMap data{{"sourceLanguage","en"},{"activeLanguage","de"},{"languages",QStringList{"en","de"}}};QString author=registerPhrase(data,"Author","title-block caption"),body=registerPhrase(data,"Safety relay","description elsewhere");auto entries=data.value("entries").toMap();auto caption=entries[author].toMap();caption["variants"]=QVariantMap{{"de",QVariantMap{{"text","Autor"}}}};entries[author]=caption;auto value=entries[body].toMap();value["variants"]=QVariantMap{{"de",QVariantMap{{"text","Sicherheitsrelais"}}}};entries[body]=value;data["entries"]=entries;
 DiagramContext context;context.addValue("author","Safety relay");TitleBlockTemplateRenderer renderer;renderer.setTitleBlockTemplate(&title);renderer.setContext(context);
 QImage german(520,70,QImage::Format_RGB32),english(520,70,QImage::Format_RGB32);german.fill(Qt::white);english.fill(Qt::white);
 {QPainter p(&german);p.translate(5,10);renderer.render(&p,500,data);}auto en=data;en["activeLanguage"]="en";{QPainter p(&english);p.translate(5,10);renderer.render(&p,500,en);}
 check(german!=english,"title-block render changes with the selected project language");check(renderer.context()==context,"title-block language context does not leak into stored source context");
 QDomDocument afterDoc;auto after=afterDoc.createElement("titleblocktemplate");title.saveToXmlElement(after);check(after.text()==original,"translated title-block rendering preserves original template text");
 check(german.save(out+"/german-title-block.png"),"save translated title-block preview");
 {QPdfWriter pdf(out+"/german-title-block.pdf");pdf.setResolution(96);QPainter p(&pdf);p.translate(20,30);renderer.render(&p,500,data);}
}
void dynamicChecks(const QString &sourceDir) {
 const QString fixture=sourceDir+"/tests/qttest/fixtures/ten_sequential_numbers.qet";if(!QFileInfo::exists(fixture))return;
 QETProject project(fixture);QETApp::registerProject(&project);check(project.state()==QETProject::Ok,"embedded-symbol fixture loads for dynamic-text checks");DynamicElementTextItem *text=nullptr;
 for(auto *d:project.diagrams()){for(auto *i:d->items())if((text=dynamic_cast<DynamicElementTextItem*>(i)))break;if(text)break;}
 check(text!=nullptr,"embedded symbol fixture includes dynamic text");const QString original=text->toPlainText(),html=text->toHtml();auto data=catalog(text->languageId16());auto entries=data.value("entries").toMap();auto e=entries[text->languageId16()].toMap();e["source"]=original;entries[text->languageId16()]=e;data["entries"]=entries;project.setLanguageData(data);text->refreshLanguage16();
 check(text->displayedText16().contains(QString::fromUtf8("Sicherheitsschalter")),"dynamic symbol text displays its translation");
 check(text->toHtml()==html&&text->toPlainText()==original,"dynamic symbol translation preserves the original document");check(text->languageLineCount16()>=3,"dynamic symbol description wraps through common renderer");
 const QFont originalFont=text->font();auto imported=data;auto importedEntries=imported.value("entries").toMap();auto importedEntry=importedEntries[text->languageId16()].toMap();importedEntry["baseX"]=text->pos().x();importedEntry["baseY"]=text->pos().y();auto importedVariants=importedEntry.value("variants").toMap();importedVariants["de"]=QVariantMap{{"text",QString::fromUtf8("Magnetischer Sicherheitsschalter mit einer längeren Beschreibung")}};importedEntry["variants"]=importedVariants;importedEntries[text->languageId16()]=importedEntry;imported["entries"]=importedEntries;
 auto normalized=QetLanguagesUi16::apply(&project,imported,false);auto symbolVariant=QetLanguages16::variant(normalized,text->languageId16(),"de");
 check(!symbolVariant.contains("width"),"text-only import inherits existing wrapping instead of inventing a width");
 check(!symbolVariant.value("auto",false).toBool(),"text-only symbol import requires explicit automatic-placement opt-in");
 check(!symbolVariant.contains("font")&&text->font()==originalFont&&text->toHtml()==html,"automatic imported-symbol defaults retain the original individual font and source text");
 QETApp::unregisterProject(&project);
}
}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS16Regression");QCoreApplication::setApplicationName("RS16Regression");
 QTemporaryDir temp;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,temp.path());QSettings().clear();QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 const QString out=argc>1?QString::fromLocal8Bit(argv[1]):temp.path();QDir().mkpath(out);
 try{catalogChecks(out);placementChecks();renderingChecks(out);phraseChecks();titleBlockChecks(out);if(argc>2)dynamicChecks(QString::fromLocal8Bit(argv[2]));std::cout<<"PASS "<<checks<<" multilingual and placement checks\n";return 0;}catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<"\n";return 1;}
}
