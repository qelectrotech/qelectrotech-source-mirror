// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtWidgets>
#include <iostream>
#include "qetproject.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "qetmessagebox.h"
#include "configdialog.h"
#include "ui/configpage/configpages.h"
#include "ui/configpage/generalconfigurationpage.h"
#include "ui/configpage/shortcutsconfigpage.h"
#include "ui/configpage/shortcutbarconfigpage.h"
#include "ui/configpage/gesturesconfigpage.h"
#include "ui/configpage/toolbarsconfigpage.h"
#include "ui/configpage/toolbarcommandsconfigpage.h"
#include "ui/languageui16.h"
int main(int argc,char **argv){
 int qtargc=1;char *qtargv[]={argv[0],nullptr};QApplication app(qtargc,qtargv);QCoreApplication::setOrganizationName("RS23Benchmark");QCoreApplication::setApplicationName("RS23Benchmark");QTemporaryDir settings;QSettings::setPath(QSettings::NativeFormat,QSettings::UserScope,settings.path());QETProject::setBackupEnabled(false);QET::QetMessageBox::setNonInteractive(true);
 QJsonObject result;auto measure=[&](QString name,auto f){QElapsedTimer t;t.start();f();double ms=t.nsecsElapsed()/1e6;result[name]=ms;std::cout<<name.toStdString()<<": "<<ms<<" ms"<<std::endl;};
 std::unique_ptr<QETApp> runtime;measure("application_construct",[&]{runtime.reset(new QETApp);});
 for(int run=0;run<3;++run){ConfigDialog cd;QString suffix=QString::number(run);
 measure("settings_ready_"+suffix,[&]{
#ifdef QET_LAZY_CONFIG23
 cd.addApplicationPages23();cd.setCurrentPage(0);
#else
 cd.addPage(new GeneralConfigurationPage);cd.addPage(new NewDiagramPage);cd.addPage(new ExportConfigPage);cd.addPage(new PrintConfigPage);cd.addPage(new ShortcutsConfigPage);cd.addPage(new ShortcutBarConfigPage);cd.addPage(new GesturesConfigPage);cd.addPage(new ToolbarsConfigPage);cd.addPage(new ToolbarCommandsConfigPage);cd.setCurrentPage(0);
#endif
 cd.show();app.processEvents();});cd.reject();}

#ifdef QET_LAZY_CONFIG23
 {ConfigDialog cd;cd.addApplicationPages23();auto *list=cd.findChild<QListWidget*>();for(int i=0;i<cd.pages.size();++i){cd.setCurrentPage(i);if(!cd.pages[i]||list->item(i)->text()!=cd.pages[i]->title()){std::cerr<<"Configuration page mismatch";return 2;}}result["configuration_pages_verified"]=cd.pages.size();}
#endif
 std::unique_ptr<QETProject> p;measure("project_open",[&]{p.reset(new QETProject(argv[2]));QETApp::registerProject(p.get());result["initial_unloaded_folios"]=p->unloadedFolioCount();QetLanguagesUi16::texts(p.get());});
 QVariantMap data;measure("language_scan",[&]{QetLanguagesUi16::scan(p.get(),data);});
 QFile csv(argv[3]);csv.open(QIODevice::ReadOnly);{QetLanguagesUi16::Dialog dialog(p.get());QString error;if(!dialog.importCsv21(csv.readAll(),&error)){std::cerr<<error.toStdString();return 1;}dialog.findChild<QComboBox*>("drawingLanguage16")->setCurrentText("de");measure("language_apply_review",[&]{dialog.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Apply)->click();});dialog.reject();}
 auto texts=QetLanguagesUi16::texts(p.get());qreal total=0;measure("bounds_20_passes",[&]{for(int i=0;i<20;++i)for(auto *t:texts)total+=t->boundingRect().width();});result["bounds_checksum"]=total;
 measure("review",[&]{result["review_messages"]=QetLanguagesUi16::review(p.get()).size();});
 auto *page=p->diagrams().at(6);QImage image(1600,1100,QImage::Format_ARGB32);measure("render_5_passes",[&]{for(int i=0;i<5;++i){image.fill(Qt::white);QPainter paint(&image);page->render(&paint);}});
 measure("language_switch_6",[&]{auto d=p->languageData();for(int i=0;i<6;++i){d["activeLanguage"]=i%2?"de":"en";d=QetLanguagesUi16::apply(p.get(),d,false);}});
 // This process measures startup and interactions only, not shutdown. The full
 // application owns top-level windows; exit after flushing the measurement file.
 QFile file(argv[1]);file.open(QIODevice::WriteOnly);file.write(QJsonDocument(result).toJson());file.close();std::cout.flush();std::_Exit(0);
}
