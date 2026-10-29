// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_CUSTOMCOMMANDS19_H
#define QET_CUSTOMCOMMANDS19_H
#include <QAction>
#include <QMainWindow>
#include <QToolBar>
#include "../shortcutmanager.h"
#include "../diagramtoolbarsettings.h"
#include "../toolbarsettings.h"

namespace QetCustomCommands19 {
inline void registerCommand(QAction *action,const QString &id,const QString &category,
                            const QIcon &icon,const QString &tip,QWidget *window,
                            const QString &requirement=QString()) {
 if(!action||!window)return;
 action->setIcon(icon);action->setToolTip(tip);action->setStatusTip(tip);
 action->setProperty("qetCommand19",id);action->setProperty("qetRequirement19",requirement);
 if(action->objectName().isEmpty())action->setObjectName(id);
 // An action pinned only to a hidden toolbar must still have an owner for
 // WindowShortcut dispatch. It is the same QAction, never a duplicate trigger.
 window->addAction(action);
 ShortcutManager::instance().registerAction(action,id,category,QKeySequence());
}
inline void rebuildEditorToolbars(QMainWindow *window,const QString &context) {
 if(!window)return;
 QStringList keep;
 for(const auto &definition:DiagramToolbarSettings::toolbars()) {
  if(DiagramToolbarSettings::contextForToolbar(definition.name)!=context)continue;
  keep<<definition.name;
  auto *toolbar=window->findChild<QToolBar*>(definition.name,Qt::FindDirectChildrenOnly);
  if(!toolbar){toolbar=new QToolBar(definition.title,window);toolbar->setObjectName(definition.name);toolbar->setMovable(!ToolbarSettings::locked());window->addToolBar(Qt::TopToolBarArea,toolbar);toolbar->setProperty("qetEditorCommands19",true);}
  const bool wasEmpty=toolbar->actions().isEmpty();
  toolbar->setWindowTitle(definition.title);
  DiagramToolbarSettings::fill(toolbar,DiagramToolbarSettings::ids(definition.name),[window](const QString &id){return ShortcutManager::instance().action(id,window);});
  if(toolbar->actions().isEmpty())toolbar->hide();else if(wasEmpty)toolbar->show();
 }
 for(auto *toolbar:window->findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly))
  if(toolbar->property("qetEditorCommands19").toBool()&&!keep.contains(toolbar->objectName())){window->removeToolBar(toolbar);toolbar->deleteLater();}
 DiagramToolbarSettings::markWindow(window);
}
}
#endif
