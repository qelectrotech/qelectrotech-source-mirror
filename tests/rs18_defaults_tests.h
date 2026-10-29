// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS18_DEFAULTS_TESTS_H
#define RS18_DEFAULTS_TESTS_H
#include <QtWidgets>
#include "ui/tabledefaults18.h"
#include "ui/tableeditor15.h"
#include "ui/tablequery15.h"
#include "factory/qetgraphicstablefactory.h"
#include "factory/ui/addtabledialog.h"
#include "qetapp.h"
#include "qetproject.h"

namespace Rs18Defaults {
inline QList<QetGraphicsTableItem *> tables(Diagram *page) {
    QList<QetGraphicsTableItem*> result;for(auto *item:page->items())if(auto *table=dynamic_cast<QetGraphicsTableItem*>(item))result<<table;return result;
}
inline QetGraphicsTableItem *makeTable(Diagram *page,const QString &kind,const QString &query,const QVariantMap &format={}) {
    auto *model=new ProjectDBModel(page->project(),page->project());model->setIdentifier(kind);model->setQuery(query);
    auto *table=new QetGraphicsTableItem;page->addItem(table);table->setModel(model);table->setCustomFormat(format);return table;
}
}

template<class Check> void runDefaults18Tests(Check check,const QString &out,const QString &,const QString &) {
    using namespace QetTableDefaults18;
    clearDefault18("nomenclature");clearDefault18("summary");
    check(!hasSavedDefault18("nomenclature")&&!hasSavedDefault18("summary"),"defaults18 starts with independent built-in defaults");
    check(defaultConfiguration18("summary").value("kind")=="summary"&&defaultConfiguration18("summary").value("query").toString().contains("project_summary_view"),"defaults18 built-in Contents is a real single query");
    check(defaultConfiguration18("nomenclature").value("query").toString().contains("element_nomenclature_view"),"defaults18 built-in BOM uses component query");
    check(defaultConfiguration18("unknown").isEmpty(),"defaults18 does not apply BOM preferences to unrelated table models");
    const QString query="SELECT 1 AS 'No.', 2 AS 'Qty', 'Sensor' AS 'Name'";
    const QString summaryQuery="SELECT '3' AS 'Page', 'Supply' AS 'Page content'";
    const QFont wanted("DejaVu Sans",13,QFont::Bold),heading("DejaVu Sans",11,QFont::Normal,true);
    const QVariantMap format{{"enabled",true},{"widths",QVariantList{37,52,280}},
        {"alignments",QVariantList{int(Qt::AlignHCenter),int(Qt::AlignHCenter),int(Qt::AlignLeft)}},
        {"bodyStyle",QVariantMap{{"font",wanted.toString()},{"color","#80337755"}}},
        {"headerStyle",QVariantMap{{"font",heading.toString()},{"color","#ff334455"}}},
        {"rowHeight",29},{"headerHeight",25},{"paddingX",4},{"paddingY",3},
        {"rowOverrides17",QVariantMap{{"1",QVariantMap{{"bodyBackground","#fff0f4ff"}}}}}};
    auto configuration=QetTableEditor15::configuration("nomenclature",query,format);configuration["columns18"]=QStringList{"No.","Qty","Name"};
    QString error;check(saveDefault18("nomenclature",configuration,&error),qPrintable(error));
    check(savedConfiguration18("nomenclature")==configuration&&hasSavedDefault18("nomenclature"),"defaults18 saves complete fonts/colours/rows/query explicitly");
    QSettings().sync();check(defaultConfiguration18("nomenclature")==configuration,"defaults18 reloads the saved configuration through settings");
    check(!hasSavedDefault18("summary"),"defaults18 saving BOM does not change Contents preference");
    auto summaryConfiguration=QetTableEditor15::configuration("summary",summaryQuery,QVariantMap{{"enabled",true},{"widths",QVariantList{55,450}},{"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",9).toString()}}}});
    summaryConfiguration["columns18"]=QStringList{"Page","Page content"};check(saveDefault18("summary",summaryConfiguration,&error),qPrintable(error));
    check(defaultConfiguration18("summary")==summaryConfiguration&&defaultConfiguration18("nomenclature")==configuration,"defaults18 keeps saved kinds isolated");
    check(!saveDefault18("summary",configuration,&error)&&defaultConfiguration18("summary")==summaryConfiguration,"defaults18 rejects wrong-type configuration without replacing preference");
    auto invalid=configuration;invalid["query"]="DROP TABLE element";check(!saveDefault18("nomenclature",invalid,&error)&&defaultConfiguration18("nomenclature")==configuration,"defaults18 rejects mutating SQL without replacing saved default");

    QETProject project;QETApp::registerProject(&project);auto *page=project.addNewDiagram();
    const QVariantMap originalFormat{{"enabled",true},{"widths",QVariantList{91}},{"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",8).toString()}}}};
    auto *existing=Rs18Defaults::makeTable(page,"nomenclature","SELECT 'Existing' AS 'Keep'",originalFormat);
    const auto originalQuery=qobject_cast<ProjectDBModel*>(existing->model())->queryString();
    {
        QetTableEditor15::Dialog dialog(existing);
        check(dialog.query()==originalQuery&&dialog.format()==originalFormat,"defaults18 reopening an existing table never replaces it with a preferred default");
        check(dialog.findChild<QPushButton*>("tableSaveDefault18")&&dialog.findChild<QPushButton*>("tableLoadDefault18")&&dialog.findChild<QPushButton*>("tableResetDefault18"),"defaults18 exposes explicit save/load/reset controls");
        dialog.resize(1060,850);dialog.grab().save(out+"/new-table-default-controls.png");
        dialog.reject();
    }
    check(existing->customFormat()==originalFormat&&qobject_cast<ProjectDBModel*>(existing->model())->queryString()==originalQuery,"defaults18 Cancel after opening does not apply preferred settings");
    {
        QetTableEditor15::Dialog dialog(existing);auto *previewView=dialog.findChild<QGraphicsView*>("tablePreview15");
        // Graphics-scene ownership is not QObject parenting: findChild on the
        // dialog cannot find the table item. Inspect the actual preview scene.
        QetGraphicsTableItem *preview=nullptr;
        if(previewView&&previewView->scene())for(auto *item:previewView->scene()->items())if(auto *candidate=dynamic_cast<QetGraphicsTableItem*>(item))if(candidate->objectName()=="previewTable15")preview=candidate;
        const QTransform beforeTransform=previewView?previewView->transform():QTransform();
        const QPointF beforeOrigin=previewView?previewView->mapToScene(QPoint(0,0)):QPointF();
        auto resized=dialog.format();resized["widths"]=QVariantList{133};if(preview)preview->setCustomFormat(resized);
        check(previewView&&previewView->transform()==beforeTransform,"defaults18 live preview resize preserves the view transform");
        check(previewView&&previewView->mapToScene(QPoint(0,0))==beforeOrigin,"defaults18 live preview resize preserves pointer scene coordinates");
        check(preview&&dialog.format().value("widths").toList()==QVariantList{133},"defaults18 preview resize signal updates the editor draft");
        check(dialog.configuration().value("format").toMap().value("widths").toList()==QVariantList{133},"defaults18 exported configuration includes interactive preview sizing");
        dialog.applyChanges();check(existing->customFormat().value("widths").toList()==QVariantList{133},"defaults18 Apply retains interactive preview sizing");
        page->undoStack().undo();check(existing->customFormat()==originalFormat,"defaults18 Undo restores sizing before the preview change");
    }


    auto *reordered=Rs18Defaults::makeTable(page,"nomenclature","SELECT 'Sensor' AS 'Name', 2 AS 'Qty'",{});
    check(applyNewTableFormat18(reordered),"defaults18 applies preferences to a newly constructed table");
    check(reordered->customFormat().value("widths").toList()==QVariantList{280,52},"defaults18 remaps widths by actual selected field instead of changing the selected query");
    check(qobject_cast<ProjectDBModel*>(reordered->model())->queryString().contains("SELECT 'Sensor' AS 'Name'"),"defaults18 format application preserves the creation dialog query");
    check(reordered->customFormat().value("bodyStyle").toMap().value("font")==wanted.toString(),"defaults18 retains saved body font on a new table");
    check(!applyNewTableFormat18(reordered),"defaults18 does not silently reapply a default to an already initialized table");

    // The public creation factory uses the same path as the Lists menu, not
    // merely an isolated format-map helper. A real view supplies dialog parent.
    QGraphicsView view(page);bool dialogSeen=false,queryRestored=false,fontRestored=false;
    const auto before=Rs18Defaults::tables(page);
    QTimer::singleShot(0,[&](){
        auto *dialog=qobject_cast<AddTableDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        dialogSeen=true;auto *widget=qobject_cast<ElementQueryWidget*>(dialog->contentWidget());
        queryRestored=widget&&QetTableQuery15::withoutState(widget->queryStr())==QetTableQuery15::withoutState(query);
        fontRestored=dialog->tableFont()==wanted&&dialog->headerFont()==heading;
        dialog->setAdjustTableToFolio(false);dialog->setAddNewTableToNewDiagram(false);dialog->accept();
    });
    QetGraphicsTableFactory::createAndAddNomenclature(page);
    check(dialogSeen&&queryRestored&&fontRestored,"defaults18 actual Add BOM dialog starts with saved fields and fonts");
    const auto after=Rs18Defaults::tables(page);check(after.size()==before.size()+1,"defaults18 accepted creation adds one complete table");
    QetGraphicsTableItem *created=nullptr;for(auto *table:after)if(!before.contains(table))created=table;
    check(created&&created->customFormat().value("bodyStyle").toMap().value("font")==wanted.toString(),"defaults18 actual creation keeps saved formatting");
    check(created&&created->customFormat().value("widths").toList()==QVariantList{37,52,280},"defaults18 actual creation keeps individually saved widths");

    const QFont explicitBody("DejaVu Sans",18),explicitHeader("DejaVu Sans",16,QFont::Bold);
    const auto beforeExplicit=Rs18Defaults::tables(page);bool explicitSeen=false;
    QTimer::singleShot(0,[&](){
        auto *dialog=qobject_cast<AddTableDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        explicitSeen=true;dialog->setTableFont(explicitBody);dialog->setHeaderFont(explicitHeader);
        dialog->setTableAlignment(Qt::AlignRight);dialog->setHeaderAlignment(Qt::AlignLeft);
        dialog->setAdjustTableToFolio(false);dialog->setAddNewTableToNewDiagram(false);dialog->accept();
    });
    QetGraphicsTableFactory::createAndAddNomenclature(page);
    QetGraphicsTableItem *explicitCreated=nullptr;for(auto *table:Rs18Defaults::tables(page))if(!beforeExplicit.contains(table))explicitCreated=table;
    check(explicitSeen&&explicitCreated,"defaults18 actual creation permits an explicit font/alignment choice");
    check(explicitCreated&&explicitCreated->customFormat().value("bodyStyle").toMap().value("font")==explicitBody.toString()&&explicitCreated->customFormat().value("headerStyle").toMap().value("font")==explicitHeader.toString(),"defaults18 creation font controls override their displayed initial defaults");
    check(explicitCreated&&explicitCreated->customFormat().value("alignments").toList()==QVariantList{int(Qt::AlignRight),int(Qt::AlignRight),int(Qt::AlignRight)},"defaults18 explicit table alignment overrides initial per-column defaults");
    check(defaultConfiguration18("nomenclature")==configuration,"defaults18 experimenting in a creation dialog does not silently save a new default");

    // Capture the font stored by the old Display-panel model role as well.
    auto *legacy=Rs18Defaults::makeTable(page,"nomenclature","SELECT 'Legacy' AS 'Name'",{});auto *legacyModel=qobject_cast<ProjectDBModel*>(legacy->model());
    const QFont legacyFont("DejaVu Sans",15,QFont::Bold);legacyModel->setData(legacyModel->index(0,0),legacyFont,Qt::FontRole);
    {
        QetTableEditor15::Dialog dialog(legacy);check(dialog.saveCurrentAsDefault18(&error),qPrintable(error));
        check(defaultConfiguration18("nomenclature").value("format").toMap().value("bodyStyle").toMap().value("font")==legacyFont.toString(),"defaults18 saves the font from legacy Display-panel formatting");
        check(legacy->customFormat().isEmpty(),"defaults18 Save default does not apply formatting to the current legacy table");
        const auto draft=dialog.format();check(dialog.restoreSoftwareDefault18(&error),qPrintable(error));
        check(!hasSavedDefault18("nomenclature")&&dialog.format()==draft,"defaults18 resetting new-table preference leaves current table/draft unchanged");
        check(dialog.loadDefaultConfiguration18(&error),qPrintable(error));
        check(legacy->customFormat().isEmpty()&&legacyModel->queryString().contains("Legacy"),"defaults18 loading a default remains a preview until Apply");
        const auto beforeUndo=page->undoStack().index();dialog.applyChanges();
        // An earlier test left a redo command. A new edit replaces redo
        // history, so count need not grow; the applied index grows by one.
        check(page->undoStack().index()==beforeUndo+1&&page->undoStack().count()==beforeUndo+1,"defaults18 applying a loaded default creates one undoable change");
        page->undoStack().undo();check(page->undoStack().index()==beforeUndo&&legacy->customFormat().isEmpty()&&legacyModel->queryString().contains("Legacy"),"defaults18 Undo restores the previous query and formatting");
    }
    check(defaultConfiguration18("summary")==summaryConfiguration,"defaults18 reset of BOM leaves Contents saved preference intact");
    check(defaultConfiguration18("nomenclature")==softwareConfiguration18("nomenclature"),"defaults18 reset returns the built-in full-column query/format");
    const QUuid existingId=existing->uuid();QString saveError;
    check(project.writeContainerFile(out+"/Defaults_Existing_Table_1.8.qetz",&saveError),qPrintable(saveError));
    {
        QETProject reopened(out+"/Defaults_Existing_Table_1.8.qetz");QETApp::registerProject(&reopened);
        check(reopened.state()==QETProject::Ok,"defaults18 existing-table project reopens");bool retained=false;
        for(auto *folio:reopened.diagrams())for(auto *table:Rs18Defaults::tables(folio))if(table->uuid()==existingId)retained=table->customFormat()==originalFormat&&qobject_cast<ProjectDBModel*>(table->model())->queryString()==originalQuery;
        check(retained,"defaults18 project load preserves existing table despite saved defaults");QETApp::unregisterProject(&reopened);
    }
    clearDefault18("nomenclature");clearDefault18("summary");QETApp::unregisterProject(&project);
}
#endif
