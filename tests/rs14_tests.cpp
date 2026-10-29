// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtWidgets>
#include <QtPrintSupport>
#include "qetproject.h"
#include "diagram.h"
#include "qetapp.h"
#include "qetmessagebox.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "print/printdrawing.h"
#define private public
#include "print/projectprintwindow.h"
#undef private
#include <iostream>
static int checks=0;
void check(bool v,const char *s){if(!v)throw std::runtime_error(s);++checks;}
int main(int argc,char **argv) {
 QApplication app(argc,argv);QCoreApplication::setOrganizationName("RS14Regression");QCoreApplication::setApplicationName("RS14Regression");
 QTemporaryDir temp;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,temp.path());QSettings().clear();QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 try {
 QString out=argv[1];QDir().mkpath(out);QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
 auto *note=new IndependentTextItem("DPI sample: 24 VDC / 13-14");page->addItem(note);note->setPos(100,100);
 for(int i=0;i<20;++i){QPen pen(Qt::black,0.5);page->addLine(100,160+i*3.1,400,160+i*3.1,pen);page->addLine(450+i*3.1,160,450+i*3.1,260,pen);}
 auto *model=new ProjectDBModel(&project,&project);model->setIdentifier("nomenclature");model->setQuery("SELECT 1 AS 'No.', 'Relay' AS 'Description'");auto *table=new QetGraphicsTableItem;page->addItem(table);table->setPos(100,400);table->setModel(model);table->setCustomFormat({{"enabled",true},{"title","PRINT TEST TABLE"},{"rows",2},{"rowHeight",25},{"headerHeight",25},{"titleHeight",25},{"widths",QVariantList{50,200}},{"headers",QVariantList{"No.","Description"}},{"headerStyle",QVariantMap{{"font",QFont("DejaVu Sans",10).toString()},{"color","#ff203040"}}}});
 auto *last=new IndependentTextItem("LAST TILE: PE / A1");page->addItem(last);last->setPos(850,600);
 QPen dash(Qt::black,0.5);dash.setDashPattern({12,8,2,8});page->addLine(100,290,500,290,dash);
 QFile fixture(out+"/DPI_Print_Test.qet");check(fixture.open(QIODevice::WriteOnly),"create manual print fixture");fixture.write(project.toXml().toByteArray(2));fixture.close();
 int tiles=0;
 for(bool fit:{true,false})for(int dpi:{96,300,600,1200}){
  auto *printer=new QPrinter(QPrinter::HighResolution);printer->setOutputFormat(QPrinter::PdfFormat);printer->setResolution(dpi);
  printer->setOutputFileName(out+QString("/%1-%2.pdf").arg(fit?"fit":"tiles").arg(dpi));
  printer->setPageSize(QPageSize(fit?QPageSize::A4:QPageSize::A5));printer->setPageOrientation(fit?QPageLayout::Landscape:QPageLayout::Portrait);
  printer->setPageMargins(QMarginsF(10,12,15,14),QPageLayout::Millimeter);
  ProjectPrintWindow window(&project,printer);window.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(fit);
  auto original=printer->pageLayout();window.m_preview->updatePreview();
  check(printer->resolution()==dpi,"preview preserves chosen DPI");
  check(printer->pageLayout()==original,"preview preserves margins and paper");
  if(!fit){int count=window.horizontalPagesCount(page,window.exportProperties(),false)*window.verticalPagesCount(page,window.exportProperties(),false);if(!tiles)tiles=count;check(count==tiles&&count>1,"tile count independent of DPI");check(window.m_preview->pageCount()==count,"preview has every tile");}
  window.requestPaint();check(printer->resolution()==dpi,"print preserves chosen DPI");check(printer->pageLayout()==original,"print preserves margins and paper");
 }
 // Full-paper rendering and page counts must also stay independent of DPI.
 for(int dpi:{96,600}){
  auto *printer=new QPrinter(QPrinter::HighResolution);printer->setOutputFormat(QPrinter::PdfFormat);printer->setResolution(dpi);printer->setPageSize(QPageSize(QPageSize::A4));printer->setPageOrientation(QPageLayout::Landscape);printer->setFullPage(true);printer->setOutputFileName(out+QString("/full-%1.pdf").arg(dpi));
  ProjectPrintWindow window(&project,printer);window.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(true);window.requestPaint();check(printer->fullPage()&&printer->resolution()==dpi,"full-page print preserves device settings");
 }
 // Retain the per-folio PDF orientation behaviour when a later page turns.
 auto *portrait=project.addNewDiagram();portrait->border_and_titleblock.setColumnsCount(8);portrait->border_and_titleblock.setRowsCount(12);
 auto *portraitNote=new IndependentTextItem("PORTRAIT PAGE");portrait->addItem(portraitNote);portraitNote->setPos(100,100);
 auto *landscape=project.addNewDiagram();auto *landscapeNote=new IndependentTextItem("LANDSCAPE AGAIN");landscape->addItem(landscapeNote);landscapeNote->setPos(100,100);
 {auto *printer=new QPrinter(QPrinter::HighResolution);printer->setOutputFormat(QPrinter::PdfFormat);printer->setResolution(600);printer->setPageSize(QPageSize(QPageSize::A4));printer->setPageMargins(QMarginsF(10,12,15,14),QPageLayout::Millimeter);printer->setOutputFileName(out+"/mixed-orientation.pdf");ProjectPrintWindow window(&project,printer);window.findChild<QCheckBox*>("m_fit_in_page_cb")->setChecked(true);window.m_printing=true;window.requestPaint();check(printer->resolution()==600,"mixed orientation preserves DPI");}
 // Exercise high-resolution raster output as well as the PDF vector engine.
 QGraphicsScene strokes;strokes.setSceneRect(0,0,200,120);for(int i=0;i<20;++i)strokes.addLine(10,10+i*4.13,180,10+i*4.13,QPen(Qt::black,0.5));
 QImage raster(1250,750,QImage::Format_RGB32);raster.setDotsPerMeterX(qRound(600/0.0254));raster.setDotsPerMeterY(qRound(600/0.0254));raster.fill(Qt::white);
 {QPainter p(&raster);QetPrint::render(&strokes,&p,QRectF(0,0,1250,750),QRectF(0,0,200,120));}
 QList<int> runs;int run=0;for(int y=0;y<raster.height();++y){if(qGray(raster.pixel(600,y))<128)++run;else if(run){runs<<run;run=0;}}
 check(runs.size()==20,"all thin lines survive raster printing");check(*std::max_element(runs.begin(),runs.end())-*std::min_element(runs.begin(),runs.end())<=1,"thin lines vary by at most one printer pixel");check(*std::min_element(runs.begin(),runs.end())>=3,"thin lines use high-resolution printer pixels");raster.save(out+"/600dpi-strokes.png");
 QETApp::unregisterProject(&project);std::cout<<"PASS "<<checks<<" printing checks; tiled page count "<<tiles<<"\n";return 0;
 }catch(const std::exception &e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<"\n";return 1;}
}
