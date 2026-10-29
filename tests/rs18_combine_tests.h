// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS18_COMBINE_TESTS_H
#define RS18_COMBINE_TESTS_H
#include "ui/tablecombine18.h"
#include "container/qetcontainer.h"
#include <QPdfWriter>
#include <QTemporaryDir>

namespace Rs18CombineTests {
struct Project {
    QETProject value;
    Project(){QETApp::registerProject(&value);}
    explicit Project(const QString &path):value(path){QETApp::registerProject(&value);}
    ~Project(){QETApp::unregisterProject(&value);}
};
inline QList<QetGraphicsTableItem*> tables(Diagram *page) {
    QList<QetGraphicsTableItem*> result;for(auto *item:page->items())if(auto *t=dynamic_cast<QetGraphicsTableItem*>(item))result<<t;
    std::sort(result.begin(),result.end(),[](QetGraphicsTableItem *a,QetGraphicsTableItem *b){return a->pos().x()<b->pos().x();});return result;
}
inline QByteArray bytes(const QString &path){QFile f(path);return f.open(QIODevice::ReadOnly)?f.readAll():QByteArray();}
inline void title(Diagram *page,const QString &text){auto props=page->border_and_titleblock.exportTitleBlock();props.title=text;page->border_and_titleblock.importTitleBlock(props);}
inline QVariantMap format(int width,int alignment=Qt::AlignLeft) {
    return {{"enabled",true},{"rows",0},{"widths",QVariantList{width}},{"rowHeight",24},{"headerHeight",22},
        {"paddingX",3},{"paddingY",1},{"alignments",QVariantList{alignment}},
        {"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",9).toString()},{"color","#ff000000"}}},
        {"headerStyle",QVariantMap{{"font",QFont("DejaVu Sans",9,QFont::Bold).toString()},{"color","#ffffffff"}}},
        {"headerBackground","#ff244876"},
        {"rowOverrides17",QVariantMap{{"1",QVariantMap{{"bodyStyle",QVariantMap{{"font",QFont("DejaVu Sans",10,QFont::Bold).toString()}}}}}}}};
}
inline QetGraphicsTableItem *piece(Diagram *page,const QString &query,int x,int width,int alignment=Qt::AlignLeft) {
    auto *table=new QetGraphicsTableItem;page->addItem(table);table->setPos(x,80);table->setTableName("Legacy column piece");
    auto *model=new ProjectDBModel(page->project(),table);model->setIdentifier("summary");model->setQuery(query);table->setModel(model);
    table->setCustomFormat(format(width,alignment));table->setSelected(true);return table;
}
template<class Check> void run(Check check,const QString &out,const QString &realProject) {
    QSettings().remove("folioLayouts/default");
    using namespace QetTableCombine18;
    Projection projection;QString error;
    check(parse("WITH source AS (SELECT 'a,b' AS a, 2 AS b) SELECT a FROM source",projection,error)&&projection.columns==QStringList{"a"},"combine parser identifies outer projection after CTE and quoted comma");
    check(!parse("SELECT DISTINCT title FROM project_summary_view",projection,error),"combine refuses DISTINCT projection rather than changing row semantics");
    check(!parse("SELECT title FROM project_summary_view ORDER BY 1",projection,error),"combine refuses positional ORDER BY that would change after appending fields");
    check(!parse("SELECT title FROM project_summary_view UNION SELECT author FROM project_summary_view",projection,error),"combine refuses outer compound query");
    check(!parse("SELECT * FROM project_summary_view",projection,error),"combine refuses wildcard projections");
    check(comparable("SELECT 'a  b' FROM source")!=comparable("SELECT 'a b' FROM source"),"combine comparison preserves significant whitespace inside SQL values");

    Project project;auto *page=project.value.addNewDiagram();title(page,"Combined contents example");
    auto *second=project.value.addNewDiagram();title(second,"Power distribution");
    const QString prefix="WITH source AS (SELECT folio AS \"Page\", title AS \"Page content\", '' AS \"Comments\", strftime('%d.%m.%Y',date) AS \"Date\" FROM project_summary_view ORDER BY pos) ";
    QList<QetGraphicsTableItem*> originals;
    originals<<piece(page,prefix+"SELECT \"Page\" FROM source",50,50,Qt::AlignHCenter);
    originals<<piece(page,prefix+"SELECT \"Page content\" FROM source",100,220);
    originals<<piece(page,prefix+"SELECT \"Comments\" FROM source",320,300);
    originals<<piece(page,prefix+"SELECT \"Date\" FROM source",620,100,Qt::AlignHCenter);
    auto special=originals[1]->customFormat();special["bodyStyle"]=QVariantMap{{"font",QFont("DejaVu Sans",9,QFont::Normal,true).toString()},{"color","#ff1040a0"}};special["bodyBackground"]="#ffeaf2ff";special["autoRowHeight"]=true;originals[1]->setCustomFormat(special);
    auto tall=originals[0]->customFormat();tall["rowHeights18"]=QVariantMap{{"0",38}};originals[0]->setCustomFormat(tall);
    check(tables(page).size()==4,"combine fixture has four real adjacent single-column table objects");
    const int rowCount=originals.first()->model()->rowCount();check(rowCount==2,"combine contents fixture queries actual project folios");
    const auto originalSecondFormat=originals[1]->customFormat();const int undoCount=page->undoStack().count();
    QetGraphicsTableItem *merged=nullptr;
    check(combine(page,originals,error,&merged),qPrintable(error));
    check(merged&&tables(page).size()==1&&merged->model()->columnCount()==4,"combine replaces four objects with one four-column live table");
    check(page->undoStack().count()==undoCount+1,"combining all columns creates one Undo entry");
    check(merged->pos()==QPointF(50,80),"combined table stays at the original leftmost body origin");
    check(merged->customFormat().value("widths").toList()==QVariantList{50,220,300,100},"combined table preserves individual column widths");
    check(merged->customFormat().value("alignments").toList()==QVariantList{int(Qt::AlignHCenter),int(Qt::AlignLeft),int(Qt::AlignLeft),int(Qt::AlignHCenter)},"combined table preserves centred Page/Date and left descriptive columns");
    check(QetFormatting::color(QetFormatting::bodyCellStyle17(merged->customFormat(),1,0))==QColor("#1040a0"),"combined table preserves a different source column colour");
    check(QetFormatting::bodyCellBackground17(merged->customFormat(),1,0)==QColor("#eaf2ff"),"combined table preserves a source column background");
    check(QetFormatting::font(QetFormatting::bodyCellStyle17(merged->customFormat(),1,1),QFont()).bold(),"combined table preserves shared body-row font override");
    check(merged->customFormat().value("autoRowHeight").toBool(),"combined table keeps automatic row fitting when a source uses it");
    check(merged->customFormat().value("rowHeights18").toMap().value("0").toInt()==38&&!merged->customFormat().value("rowHeights18").toMap().contains("1"),"combined manual row height synchronizes all columns without freezing automatic rows");
    auto *mergedModel=qobject_cast<ProjectDBModel*>(merged->model());
    check(mergedModel&&mergedModel->queryString().startsWith("WITH source AS")&&!mergedModel->queryString().contains("ROW_NUMBER"),"combined query preserves common live source rather than joining results by ordinal rows");
    for(int r=0;r<rowCount;++r)for(int c=0;c<4;++c)
        check(merged->model()->index(r,c).data(QetLanguages16::SourceTextRole16)==originals[c]->model()->index(r,0).data(QetLanguages16::SourceTextRole16),"combined cell equals the original source cell");
    auto *third=project.value.addNewDiagram();title(third,"External signals");project.value.dataBase()->updateDB();
    check(merged->model()->rowCount()==3,"combined table remains live when a folio is added");
    // Folios and graphics share the project Undo stack. First undo that
    // independent AddDiagramCommand, then exercise the single combine Undo.
    page->undoStack().undo();
    check(tables(page).size()==1&&merged->scene()==page,"undoing a later folio addition does not undo table combination");
    page->undoStack().undo();
    check(tables(page).size()==4&&merged->scene()==nullptr,"one Undo restores all original table objects");
    check(originals[1]->customFormat()==originalSecondFormat&&originals[1]->isSelected(),"Undo restores original formatting and selection");
    page->undoStack().redo();check(tables(page).size()==1&&merged->scene()==page,"one Redo restores the unified table");
    const QString sample=out+"/Combined_Contents_1.8.qetz";
    check(project.value.writeContainerFile(sample,&error),qPrintable(error));
    check(QetContainer::isZip(bytes(sample)),"combined sample uses a true QETZ container");
    {Project reopened(sample);auto pages=reopened.value.diagrams();auto saved=tables(pages.first());
        check(saved.size()==1&&saved.first()->model()->columnCount()==4,"QETZ save/reopen retains one unified contents table");
        check(saved.first()->customFormat().value("columnOverrides17")==merged->customFormat().value("columnOverrides17"),"QETZ round trip retains merged column styles");
    }
    {QPdfWriter pdf(out+"/Combined_Contents_1.8.pdf");pdf.setResolution(96);pdf.setPageSize(QPageSize(QPageSize::A4));pdf.setPageOrientation(QPageLayout::Landscape);QPainter p(&pdf);p.translate(30,80);QetFormatting::paintTable(p,merged->customFormat(),merged->model(),0,merged->model()->rowCount(),QFont("DejaVu Sans",9));}
    check(QFileInfo(out+"/Combined_Contents_1.8.pdf").size()>1000,"combined table generates vector PDF output");

    page->undoStack().undo();
    originals[1]->setPos(101,100);const auto beforeCount=tables(page).size();
    check(!combine(page,originals,error)&&tables(page).size()==beforeCount,"misaligned source tables are rejected without removing objects");
    originals[1]->setPos(100,80);
    auto different=originals[1]->customFormat();different["rowOverrides17"]=QVariantMap();originals[1]->setCustomFormat(different);
    check(!combine(page,originals,error)&&error.contains("row styles"),"incompatible row overrides are rejected rather than silently discarded");
    originals[1]->setCustomFormat(originalSecondFormat);
    auto *differentModel=qobject_cast<ProjectDBModel*>(originals[1]->model());const auto beforeQuery=differentModel->queryString();
    differentModel->setQuery(prefix+"SELECT \"Page content\" FROM source WHERE \"Page\" <> 'never'");
    check(!combine(page,originals,error)&&error.contains("source query"),"different source filtering is rejected despite equal current row count");differentModel->setQuery(beforeQuery);
    project.value.setReadOnly(true);check(!combine(page,originals,error),"read-only projects cannot combine tables");project.value.setReadOnly(false);

    if(!realProject.isEmpty()&&QFileInfo::exists(realProject)){
        const QByteArray originalBytes=bytes(realProject);Project actual(realProject);QTemporaryDir temporary;
        check(actual.value.state()==QETProject::Ok,"combine opens actual RS NGX QETZ without changing it");
        int summaries=0,boms=0;
        for(auto *realPage:actual.value.diagrams()){
            auto pieces=tables(realPage);if(pieces.size()<2)continue;
            const QString kind=qobject_cast<ProjectDBModel*>(pieces.first()->model())->identifier();
            const int cols=kind=="summary"?4:kind=="nomenclature"?6:0;if(!cols)continue;
            const int originalRows=pieces.first()->model()->rowCount();QetGraphicsTableItem *combined=nullptr;
            check(combine(realPage,pieces,error,&combined),qPrintable(error));
            check(tables(realPage).size()==1&&combined->model()->columnCount()==cols&&combined->model()->rowCount()==originalRows,"actual legacy column pieces combine with identical live data dimensions");
            realPage->undoStack().undo();check(tables(realPage).size()==pieces.size(),"actual RS table combination supports one-step Undo");
            realPage->undoStack().redo();
            if(kind=="summary")++summaries;else ++boms;
        }
        check(summaries==1&&boms==1,"actual project combines four contents pieces and two BOM pieces");
        check(actual.value.writeContainerFile(temporary.filePath("combined-rs-copy.qetz"),&error),qPrintable(error));
        check(bytes(realProject)==originalBytes,"actual RS project remains byte-for-byte unchanged on disk");
    }
}
}
template<class Check> inline void runCombine18Tests(Check check,const QString &out,const QString &,const QString &realProject) {
    Rs18CombineTests::run(check,out,realProject);
}
#endif
