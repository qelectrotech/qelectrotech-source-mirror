/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef QET_FOLIOLAYOUT13_H
#define QET_FOLIOLAYOUT13_H
#include "../qetproject.h"
#include "../diagram.h"
#include "../qetapp.h"
#include "../titleblock/templatescollection.h"
#include "../ElementsCollection/xmlelementcollection.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../container/qetzwriter.h"
#include <QSaveFile>
#include <functional>
#include <QCryptographicHash>
#include <QMessageBox>

namespace QetFolioLayouts {
inline bool loading=false;
inline void visitElements(QDomElement root,const std::function<void(QDomElement)> &fn) {
    fn(root);for(auto child=root.firstChildElement();!child.isNull();child=child.nextSiblingElement())visitElements(child,fn);
}
inline void renewIdentities(QDomElement root) {
    QHash<QString,QString> ids;visitElements(root,[&](QDomElement e){QString id=e.attribute("uuid");if(!QUuid(id).isNull()&&!ids.contains(id))ids[id]=QUuid::createUuid().toString();});
    visitElements(root,[&](QDomElement e){auto attrs=e.attributes();for(int i=0;i<attrs.size();++i){auto a=attrs.item(i).toAttr();QString v=a.value();for(auto it=ids.cbegin();it!=ids.cend();++it)v.replace(it.key(),it.value());e.setAttribute(a.name(),v);}
        for(auto n=e.firstChild();!n.isNull();n=n.nextSibling())if(n.isText()){QString v=n.nodeValue();for(auto it=ids.cbegin();it!=ids.cend();++it)v.replace(it.key(),it.value());n.setNodeValue(v);}
    });
}
inline void detachTables(Diagram *source,QDomDocument &doc) {
    QHash<QString,QetGraphicsTableItem*> tables;for(auto *item:source->items())if(auto *t=dynamic_cast<QetGraphicsTableItem*>(item))tables[t->uuid().toString()]=t;
    auto nodes=doc.elementsByTagName("graphics_table");for(int i=0;i<nodes.size();++i){auto e=nodes.at(i).toElement();auto *t=tables.value(e.attribute("uuid"));auto old=e.firstChildElement("previous_table");if(!old.isNull())e.removeChild(old);
        if(t && e.firstChildElement("model").isNull())if(auto *model=dynamic_cast<ProjectDBModel*>(t->model())){auto m=doc.createElement("model");m.appendChild(model->toXml(doc));e.appendChild(m);}
    }
}
inline bool save(Diagram *source,const QString &path,QString &error) {
    error.clear();
    if(!source||!source->project()){error=QObject::tr("There is no folio to save as a layout.");return false;}
    auto doc=source->project()->toXml();auto root=doc.documentElement();
    for(auto e=root.firstChildElement("diagram");!e.isNull();){auto next=e.nextSiblingElement("diagram");root.removeChild(e);e=next;}
    auto page=source->toXml(true,false);detachTables(source,page);auto e=doc.importNode(page.documentElement(),true).toElement();e.setAttribute("order",1);root.appendChild(e);root.setAttribute("title","Reusable folio layout");root.setAttribute("folio-layout-version",1);
    if(path.endsWith(QLatin1String(".qetz"),Qt::CaseInsensitive))return QetzWriter::write(doc,path,&error);
    QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)){error=file.errorString();return false;}auto bytes=doc.toByteArray(2);if(file.write(bytes)!=bytes.size()||!file.commit()){error=file.errorString();return false;}return true;
}
// Serialize a definition in its own document so equality does not depend on
// which project's DOM owns it. Content-addressed names are reused only when
// the definition is equal; an existing, different symbol is never replaced.
inline QByteArray definitionBytes(const QDomElement &definition) {
    QDomDocument doc;doc.appendChild(doc.importNode(definition,true));return doc.toByteArray(0);
}
inline QString importDefinition(XmlElementCollection *collection,QETProject *target,
                                const QDomElement &definition,QString &error) {
    if(!collection||definition.tagName()!=QLatin1String("definition")) {
        error=QObject::tr("The folio layout contains a missing or invalid symbol definition.");return {};
    }
    // importCategory() deliberately falls back to another top-level category
    // for old projects. That is not proof that an actual "import" folder
    // exists: addElementDefinition("import",...) requires that exact folder.
    if(collection->directory(QStringLiteral("import")).isNull()) {
        auto root=collection->root();auto owner=root.ownerDocument();
        auto category=owner.createElement(QStringLiteral("category"));
        category.setAttribute(QStringLiteral("name"),QStringLiteral("import"));
        root.appendChild(category);
        collection->directorieAdded(QStringLiteral("import"));
    }
    const auto bytes=definitionBytes(definition);
    const QString base=QStringLiteral("layout_")+QString::fromLatin1(
                QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex().left(20));
    for(int suffix=0;;++suffix) {
        const QString name=base+(suffix?QStringLiteral("_%1").arg(suffix):QString())+QStringLiteral(".elmt");
        const QString relative=QStringLiteral("import/")+name;
        const auto existing=collection->element(relative);
        if(!existing.isNull()) {
            if(definitionBytes(existing.firstChildElement(QStringLiteral("definition")))==bytes)
                return QStringLiteral("embed://")+relative;
            continue;
        }
        if(!collection->addElementDefinition(QStringLiteral("import"),name,definition)) {
            error=QObject::tr("Could not import the layout symbol %1.").arg(name);return {};
        }
        ElementsLocation placed(QStringLiteral("embed://")+relative,target);
        if(placed.xml().tagName()!=QLatin1String("definition")) {
            error=QObject::tr("The imported layout symbol %1 could not be read back.").arg(name);return {};
        }
        return placed.collectionPath();
    }
}
inline bool apply(QETProject *target,Diagram *page,const QString &path,QString &error) {
    error.clear();
    if(!target||!page||page->project()!=target||target->isReadOnly()) {
        error=QObject::tr("Choose an editable destination project for the folio layout.");return false;
    }
    if(loading)return true;QScopedValueRollback<bool> guard(loading,true);
    QETProject source(path);if(source.state()!=QETProject::Ok){error=QObject::tr("Could not open the folio layout.");return false;}
    auto pages=source.diagrams();if(pages.size()!=1){error=QObject::tr("Choose a layout containing exactly one folio.");return false;}
    // Whole-folio serialization retains any unresolved symbol/wire records.
    // Copy-command serialization omits those records and could silently turn
    // a damaged layout into an apparently successful, incomplete import.
    auto *original=pages.first();auto doc=original->toXml(true,false);detachTables(original,doc);auto root=doc.documentElement();
    auto *collection=target->embeddedElementCollection();QHash<QString,QString> imported;
    // Validate every source definition before changing the destination. A
    // real .qetz layout is loaded through QETProject just like a .qet layout.
    QHash<QString,QDomElement> definitions;
    auto sourceElements=root.elementsByTagName("element");
    for(int i=0;i<sourceElements.size();++i) {
        const QString type=sourceElements.at(i).toElement().attribute("type");
        if(!type.startsWith("embed://")||definitions.contains(type))continue;
        const auto definition=ElementsLocation(type,&source).xml();
        if(definition.tagName()!=QLatin1String("definition")) {
            error=QObject::tr("Could not read the symbol definition %1 from the folio layout.").arg(type);return false;
        }
        definitions.insert(type,definition);
    }
    auto elements=root.elementsByTagName("element");for(int i=0;i<elements.size();++i){auto e=elements.at(i).toElement();QString type=e.attribute("type");if(!type.startsWith("embed://"))continue;
        if(!imported.contains(type)) {
            const QString destination=importDefinition(collection,target,definitions.value(type),error);
            if(destination.isEmpty())return false;
            imported[type]=destination;
        }e.setAttribute("type",imported.value(type));
    }
    root.removeAttribute("projectId");renewIdentities(root);
    auto tb=original->border_and_titleblock.exportTitleBlock();auto bp=original->border_and_titleblock.exportBorder();
    QString templateName=original->border_and_titleblock.titleBlockTemplateName();auto description=source.embeddedTitleBlockTemplatesCollection()->getTemplateXmlDescription(templateName);
    if(!description.isNull()){
        QDomDocument templateDoc;templateDoc.appendChild(templateDoc.importNode(description,true));
        QString newName="layout_"+QString::fromLatin1(QCryptographicHash::hash(templateDoc.toByteArray(),QCryptographicHash::Sha256).toHex().left(20));
        if(!target->embeddedTitleBlockTemplatesCollection()->setTemplateXmlDescription(newName,description)){error=QObject::tr("Could not import the title block.");return false;}
        templateName=newName;
    }
    page->border_and_titleblock.importBorder(bp);page->border_and_titleblock.importTitleBlock(tb);page->setTitleBlockTemplate(templateName);
    page->defaultConductorProperties=original->defaultConductorProperties;
    if(!page->fromXml(root,QPointF(),false,nullptr)){error=QObject::tr("Could not read the folio layout content.");return false;}
    QList<Element*> copies;for(auto *item:page->items())if(auto *element=dynamic_cast<Element*>(item))copies<<element;
    for(auto *element:copies)element->initLink(copies);
    for(auto *element:copies)if(element->linkType()==Element::Slave || element->linkType() & Element::AllReport)page->restoreText(element);
    return true;
}
}
#endif
