// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS17_FOLIO_TESTS_H
#define RS17_FOLIO_TESTS_H
#include "ui/foliolayout13.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "container/qetcontainer.h"
#include <QFile>
#include <QDir>
#include <QTemporaryDir>

namespace Rs17Folio {
struct Project {
    QETProject value;
    Project():value(){QETApp::registerProject(&value);}
    explicit Project(const QString &path):value(path){QETApp::registerProject(&value);}
    ~Project(){QETApp::unregisterProject(&value);}
};
inline QByteArray bytes(const QString &path) {QFile f(path);if(!f.open(QIODevice::ReadOnly))return {};return f.readAll();}
inline bool write(const QString &path,const QByteArray &data) {QFile f(path);return f.open(QIODevice::WriteOnly)&&f.write(data)==data.size();}
inline int elements(Diagram *page) {int n=0;for(auto *i:page->items())if(dynamic_cast<Element*>(i))++n;return n;}
inline int images(Diagram *page) {int n=0;for(auto *i:page->items())if(dynamic_cast<DiagramImageItem*>(i))++n;return n;}
inline int definitions(QETProject &p) {return p.embeddedElementCollection()->root().elementsByTagName("definition").size();}
inline void onlyOtherCategory(QETProject &p) {
    auto root=p.embeddedElementCollection()->root();
    while(!root.firstChild().isNull())root.removeChild(root.firstChild());
    auto category=root.ownerDocument().createElement("category");category.setAttribute("name","rs_existing");root.appendChild(category);
}
template<class Check> void run(const QString &out,const QString &realProject,Check check) {
    QSettings().remove("folioLayouts/default");
    QDir().mkpath(out+"/folio17");const QString dir=out+"/folio17/";
    const QString input=dir+"source-custom-category.qet";
    const QByteArray fixture=R"XML(<project title="Folio layout regression" version="0.200.1" uuid="{69a46ea0-2092-4ebd-81b1-dd07d150380f}">
      <collection><category name="rs_support"><element name="Border.elmt"><definition type="element" link_type="simple" version="0.100.0" width="120" height="30" hotspot_x="0" hotspot_y="0">
        <uuid uuid="{0d811955-2316-4cd0-b78d-5d7f6e9dd5c8}"/><names><name lang="en">Reusable border graphic</name></names>
        <description><line x1="0" y1="0" x2="120" y2="0" style="line-style:normal;line-weight:normal;filling:none;color:black"/></description>
      </definition></element></category></collection>
      <diagram title="Border only" uuid="{40953c4e-5d53-4582-a46b-20498f214d1c}" cols="8" colsize="90" displaycols="true" rows="5" rowsize="75" displayrows="true" order="1" version="0.200.1-dev">
        <elements><element type="embed://rs_support/Border.elmt" uuid="{01b6ef19-1062-4223-b30d-90d1378c6ea1}" x="55" y="80" z="10" orientation="0" is_movable="1"><terminals/><inputs/><elementInformations/><dynamic_texts/><texts_groups/></element></elements>
      </diagram></project>)XML";
    check(write(input,fixture),"folio17 writes synthetic non-import-category fixture");
    Project source(input);check(source.value.state()==QETProject::Ok,"folio17 opens custom-category source");
    auto *page=source.value.diagrams().first();check(elements(page)==1,"folio17 source contains actual embedded border symbol");
    const auto sourceUuid=page->elements().first()->uuid();
    auto *note=new IndependentTextItem("Layout retained text");page->addItem(note);note->setPos(140,180);
    QPixmap pixmap(32,12);pixmap.fill(QColor("#c04020"));auto *image=new DiagramImageItem(pixmap);page->addItem(image);image->setPos(350,260);
    QDomDocument tb;check(tb.setContent(QString("<titleblocktemplate name='rs17-title'><information/><logos/><grid cols='t100%;' rows='35;'><field valign='center' row='0' col='0' name='title' displaylabel='true' align='left'><value><translation lang='en'>%title</translation></value><label><translation lang='en'>Layout title</translation></label></field></grid></titleblocktemplate>")),"folio17 creates embedded title-block definition");
    check(source.value.embeddedTitleBlockTemplatesCollection()->setTemplateXmlDescription("rs17-title",tb.documentElement()),"folio17 stores embedded title-block fixture");page->setTitleBlockTemplate("rs17-title");
    const auto sourceBorder=page->border_and_titleblock.exportBorder();
    QString error;
    const QString qet=dir+"reusable-border.qet",qetz=dir+"reusable-border.qetz";
    check(QetFolioLayouts::save(page,qet,error),qPrintable(error));
    check(QetFolioLayouts::save(page,qetz,error),qPrintable(error));
    check(!QetContainer::isZip(bytes(qet))&&QetContainer::isZip(bytes(qetz)),"folio17 extension dispatch writes XML qet and real ZIP qetz");
    Project target;onlyOtherCategory(target.value);auto *collection=target.value.embeddedElementCollection();
    check(collection->directory("import").isNull()&&!collection->importCategory().isNull(),"folio17 reproduces importCategory fallback with no actual import folder");
    for(const auto &path:{qet,qetz}) {
        Project reopened(path);check(reopened.value.state()==QETProject::Ok,"folio17 saved layout format reopens");
        check(reopened.value.folios().size()==1,"folio17 saved layout holds exactly one folio");
        auto *copy=target.value.addNewDiagram();check(copy!=nullptr,"folio17 creates destination page");
        check(QetFolioLayouts::apply(&target.value,copy,path,error),qPrintable(error));
        check(elements(copy)==1&&images(copy)==1,"folio17 import retains embedded border and image");
        auto border=copy->border_and_titleblock.exportBorder();check(border==sourceBorder,"folio17 import retains border dimensions and header flags");
        check(copy->elements().first()->pos()==QPointF(55,80),"folio17 import retains border graphic position");
        check(copy->elements().first()->uuid()!=sourceUuid,"folio17 imported border has a new instance identity");
        check(!collection->directory("rs_existing").isNull()&&!collection->directory("import").isNull(),"folio17 creates actual import category without replacing existing categories");
        check(definitions(target.value)==1,"folio17 repeated import reuses identical embedded definition");
        const QString templateName=copy->border_and_titleblock.titleBlockTemplateName();
        check(!target.value.embeddedTitleBlockTemplatesCollection()->getTemplateXmlDescription(templateName).isNull(),"folio17 imported page owns its title-block template");
        int notes=0;for(auto *i:copy->items())if(auto *t=dynamic_cast<IndependentTextItem*>(i))if(t->toPlainText()=="Layout retained text")++notes;
        check(notes==1,"folio17 copies text once per destination page");
    }
    // A deliberately occupied content-addressed name may not overwrite a
    // different definition. The helper chooses a suffix and then reuses it.
    const auto definition=source.value.embeddedElementCollection()->element("rs_support/Border.elmt").firstChildElement("definition");
    const QString base="layout_"+QString::fromLatin1(QCryptographicHash::hash(QetFolioLayouts::definitionBytes(definition),QCryptographicHash::Sha256).toHex().left(20));
    Project collision;auto *cc=collision.value.embeddedElementCollection();
    QDomDocument different;different.appendChild(different.importNode(definition,true));different.documentElement().setAttribute("width",121);
    check(cc->addElementDefinition("import",base+".elmt",different.documentElement()),"folio17 creates occupied hash-name fixture");
    auto imported=QetFolioLayouts::importDefinition(cc,&collision.value,definition,error);
    check(imported.endsWith(base+"_1.elmt"),"folio17 chooses a safe suffix for conflicting symbol name");
    check(cc->element("import/"+base+".elmt").firstChildElement("definition").attribute("width")=="121","folio17 preserves conflicting original symbol");
    check(QetFolioLayouts::importDefinition(cc,&collision.value,definition,error)==imported&&definitions(collision.value)==2,"folio17 repeats hash-conflict import without duplicating definitions");
    const QString destination=dir+"imported-layouts.qetz";
    check(target.value.writeContainerFile(destination,&error),qPrintable(error));
    {Project saved(destination);check(saved.value.state()==QETProject::Ok,"folio17 destination qetz reopens");check(saved.value.diagrams().size()==2,"folio17 destination qetz retains both imported folios");for(auto *p:saved.value.diagrams())check(elements(p)==1&&images(p)==1,"folio17 destination save/reopen retains symbols and pictures");}
    QSettings().setValue("folioLayouts/default",qetz);auto *defaultPage=target.value.addNewDiagram();QSettings().remove("folioLayouts/default");
    check(defaultPage&&elements(defaultPage)==1,"folio17 true qetz is usable as default new-folio layout");
    QDomDocument broken;check(broken.setContent(bytes(qet)),"folio17 parses invalid-definition fixture");auto collectionNode=broken.documentElement().firstChildElement("collection");broken.documentElement().removeChild(collectionNode);
    const QString missing=dir+"missing-definition.qet";check(write(missing,broken.toByteArray()),"folio17 writes missing-definition fixture");
    auto *rejected=target.value.addNewDiagram();const int previousDefinitions=definitions(target.value);
    check(!QetFolioLayouts::apply(&target.value,rejected,missing,error)&&error.contains("definition"),"folio17 rejects missing embedded definition with meaningful error");
    check(elements(rejected)==0&&definitions(target.value)==previousDefinitions,"folio17 rejected definition import leaves target page/resources unchanged");
    if(!realProject.isEmpty()&&QFileInfo::exists(realProject)) {
        const QByteArray before=bytes(realProject);Project real(realProject);check(real.value.state()==QETProject::Ok,"folio17 opens real RS NGX qetz");
        auto pages=real.value.diagrams();Diagram *chosen=nullptr;for(auto *p:pages)if(elements(p)>0){chosen=p;break;}
        check(chosen!=nullptr,"folio17 real project contains reusable embedded graphics");
        QTemporaryDir privateLayout;
        check(privateLayout.isValid(),"folio17 creates temporary private real-project layout folder");
        const int count=elements(chosen);const QString realLayout=privateLayout.filePath("RS_NGX_saved_layout.qetz");
        check(QetFolioLayouts::save(chosen,realLayout,error),qPrintable(error));
        auto *added=real.value.addNewDiagram();check(added!=nullptr,"folio17 creates real-project destination folio");
        check(QetFolioLayouts::apply(&real.value,added,realLayout,error),qPrintable(error));
        check(elements(added)==count,"folio17 layout from real qetz imports all symbol instances back into same custom-category project");
        check(bytes(realProject)==before,"folio17 real input project remains byte-for-byte unchanged");
    }
}
}
#endif
