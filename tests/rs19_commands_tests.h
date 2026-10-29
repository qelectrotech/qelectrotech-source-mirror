// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS19_COMMANDS_TESTS_H
#define RS19_COMMANDS_TESTS_H
#include <QtWidgets>
#include "ui/customcommands19.h"
#include "ui/configpage/toolbarcommandsconfigpage.h"
#include "shortcutbarsettings.h"
#include "gesturesettings.h"
#include "shortcutmanager.h"
#include "diagramtoolbarsettings.h"

template<class Check> void runCommands19Tests(Check check,const QString &out,const QString &sourceDir,const QString &) {
 using namespace QetCustomCommands19;
 auto read=[&](const QString &relative){QFile file(sourceDir+"/"+relative);check(file.open(QIODevice::ReadOnly),"commands19 opens production integration source");return QString::fromUtf8(file.readAll());};
 const auto editorSource=read("sources/qetdiagrameditor.cpp");
 const QStringList mainIds{"text_alignment_reference","combine_tables","table_definition","table_format","grouped_bom","table_save_default","table_load_default","conductor_pattern","apply_project_pe_pattern","project_languages","save_folio_layout","add_folio_layout","default_folio_layout","clear_folio_layout","text_style"};
 for(const auto &id:mainIds)check(editorSource.contains("\"diagrameditor."+id+"\""),"commands19 production editor registers every new public command ID");
 const int menu=editorSource.indexOf("\tsetUpMenu();");const int refill=editorSource.indexOf("\trebuildToolBars();",menu);
 check(menu>=0&&refill>menu&&refill<editorSource.indexOf("\ttabifyDockWidget",menu),"commands19 fresh diagram window refills saved toolbar after menu command registration");
 const auto symbolSource=read("sources/editor/ui/qetelementeditor.cpp");
 const auto titleSource=read("sources/titleblock/qettemplateeditor.cpp");
 check(symbolSource.contains("\"elementeditor.part_appearance\"")&&symbolSource.contains("\"elementeditor.part_properties\"")&&symbolSource.contains("StyleEditor(this,nullptr,&dialog)"),"commands19 symbol command opens actual part/dash appearance editor");
 check(titleSource.contains("\"titleblockeditor.cell_text_style\"")&&titleSource.contains("titleBlockTextStyle13"),"commands19 title-block command invokes actual caption/value style controls");
 check(symbolSource.contains("QETElementEditor::rebuildToolBars")&&titleSource.contains("QETTitleBlockTemplateEditor::rebuildToolBars"),"commands19 both non-diagram editors rebuild persisted contextual toolbars");

 const auto oldCustom=DiagramToolbarSettings::customToolbars();
 QStringList settingKeys{"diagrameditor/toolbars/elementeditor_commands19","diagrameditor/toolbars/titleblockeditor_commands19","diagrameditor/toolbars/custom_9919","diagrameditor/toolbars/elementeditor_custom_9919","shortcuts/diagrameditor.text_alignment_reference","diagrameditor/shortcut_bar/selection","diagrameditor/gestures/selection"};
 QVariantMap saved;QSet<QString> existed;QSettings settings;
 for(const auto &key:settingKeys){if(settings.contains(key)){existed.insert(key);saved[key]=settings.value(key);}}
 const QString actionId="diagrameditor.text_alignment_reference";
 QPixmap pixmap(16,16);pixmap.fill(Qt::darkBlue);QIcon icon(pixmap);
 QMainWindow first;first.setObjectName("commands19-first-window");
 auto *alignment=new QAction("Text alignment and reference...",&first);int triggered=0;QObject::connect(alignment,&QAction::triggered,&first,[&](){++triggered;});
 registerCommand(alignment,actionId,"Diagram editors",icon,"Align selected texts with a fixed reference",&first,"texts");
 check(first.actions().contains(alignment)&&alignment->shortcutContext()==Qt::WindowShortcut,"commands19 command remains a window shortcut when its toolbar is hidden");
 check(alignment->property("qetRequirement19")=="texts"&&!alignment->toolTip().isEmpty()&&!alignment->icon().isNull(),"commands19 context requirement, help and icon are retained");
 check(DiagramToolbarSettings::availableCommandIds().contains(actionId)&&ShortcutBarSettings::availableIds().contains(actionId),"commands19 new command is offered to toolbars and shortcut bar");
 ShortcutManager::instance().setSequence(actionId,QKeySequence("Ctrl+Alt+9"));
 check(alignment->shortcut()==QKeySequence("Ctrl+Alt+9"),"commands19 assigning a shortcut updates the actual QAction");
 DiagramToolbarSettings::setCustomToolbars({{"custom_9919","My QET tools",true},{"elementeditor_custom_9919","My symbol tools",true}});
 DiagramToolbarSettings::setIds("custom_9919",{actionId});
 QToolBar firstBar;DiagramToolbarSettings::fill(&firstBar,DiagramToolbarSettings::ids("custom_9919"),[&](const QString &id){return ShortcutManager::instance().action(id,&first);});
 check(firstBar.actions()==QList<QAction*>{alignment},"commands19 custom diagram toolbar contains the same registered action");
 alignment->setEnabled(false);alignment->trigger();check(triggered==0,"commands19 a context-disabled toolbar command cannot execute");alignment->setEnabled(true);alignment->trigger();check(triggered==1,"commands19 enabled custom button dispatches once");
 QMainWindow second;auto *secondAlignment=new QAction("Text alignment and reference...",&second);registerCommand(secondAlignment,actionId,"Diagram editors",icon,"Align texts",&second,"texts");
 QToolBar secondBar;DiagramToolbarSettings::fill(&secondBar,DiagramToolbarSettings::ids("custom_9919"),[&](const QString &id){return ShortcutManager::instance().action(id,&second);});
 check(secondBar.actions()==QList<QAction*>{secondAlignment}&&secondAlignment->shortcut()==QKeySequence("Ctrl+Alt+9"),"commands19 fresh window restores saved button and key binding using its own action");
 check(ShortcutManager::instance().action(actionId,&first)==alignment&&ShortcutManager::instance().action(actionId,&second)==secondAlignment,"commands19 two windows resolve their own action, never another window's command");
 ShortcutBarSettings::setIds(ShortcutBarSettings::Selection,{actionId});check(ShortcutBarSettings::ids(ShortcutBarSettings::Selection)==QStringList{actionId},"commands19 shortcut bar persists the new action ID");
 const auto oldGesture=GestureSettings::ids(ShortcutBarSettings::Selection);const bool customGesture=GestureSettings::isCustom(ShortcutBarSettings::Selection);
 GestureSettings::setIds(ShortcutBarSettings::Selection,{actionId});check(GestureSettings::ids(ShortcutBarSettings::Selection).value(0)==actionId,"commands19 gesture slots accept the same registered command");
 if(customGesture)GestureSettings::setIds(ShortcutBarSettings::Selection,oldGesture);else GestureSettings::reset(ShortcutBarSettings::Selection);

 const QString symbolId="elementeditor.part_appearance",titleId="titleblockeditor.cell_text_style";
 QMainWindow symbolWindow,titleWindow;
 auto *symbolAction=new QAction("Selected part appearance and dash pattern...",&symbolWindow);registerCommand(symbolAction,symbolId,"Element editors",icon,"Edit symbol dash pattern",&symbolWindow,"graphic-parts");
 auto *titleAction=new QAction("Cell caption/value fonts, colours and opacity...",&titleWindow);registerCommand(titleAction,titleId,"Title block template editors",icon,"Edit caption/value fonts",&titleWindow,"text-cell");
 check(DiagramToolbarSettings::builtInNames().contains("elementeditor_commands19")&&DiagramToolbarSettings::builtInNames().contains("titleblockeditor_commands19"),"commands19 customization provides explicit symbol and title-block toolbar targets");
 check(!DiagramToolbarSettings::availableCommandIds().contains(symbolId)&&DiagramToolbarSettings::availableCommandIds("elementeditor").contains(symbolId),"commands19 symbol-only commands do not leak into diagram toolbar catalog");
 check(DiagramToolbarSettings::availableCommandIds("titleblockeditor").contains(titleId)&&!DiagramToolbarSettings::availableCommandIds("titleblockeditor").contains(actionId),"commands19 title-block toolbar shows its own valid command context");
 check(DiagramToolbarSettings::contextForToolbar("elementeditor_custom_9919")=="elementeditor"&&DiagramToolbarSettings::contextForToolbar("custom_9919")=="diagrameditor","commands19 custom toolbar names retain editor context");
 DiagramToolbarSettings::setIds("elementeditor_commands19",{symbolId});DiagramToolbarSettings::setIds("elementeditor_custom_9919",{symbolId});DiagramToolbarSettings::setIds("titleblockeditor_commands19",{titleId});
 rebuildEditorToolbars(&symbolWindow,"elementeditor");rebuildEditorToolbars(&titleWindow,"titleblockeditor");
 auto *symbolBar=symbolWindow.findChild<QToolBar*>("elementeditor_commands19");auto *titleBar=titleWindow.findChild<QToolBar*>("titleblockeditor_commands19");
 check(symbolBar&&symbolBar->actions()==QList<QAction*>{symbolAction}&&titleBar&&titleBar->actions()==QList<QAction*>{titleAction},"commands19 editor toolbars resolve real commands in the correct editor window");
 check(symbolWindow.findChild<QToolBar*>("elementeditor_custom_9919")&&!symbolWindow.findChild<QToolBar*>("custom_9919"),"commands19 symbol editor rebuilds only its own custom toolbar");
 QMainWindow reopenedSymbol;auto *reopenedAction=new QAction("Selected part appearance and dash pattern...",&reopenedSymbol);registerCommand(reopenedAction,symbolId,"Element editors",icon,"Edit symbol dash pattern",&reopenedSymbol,"graphic-parts");rebuildEditorToolbars(&reopenedSymbol,"elementeditor");
 check(reopenedSymbol.findChild<QToolBar*>("elementeditor_commands19")->actions()==QList<QAction*>{reopenedAction},"commands19 fresh symbol window restores persisted toolbar using its fresh action");
 ToolbarCommandsConfigPage page;auto *combo=page.findChild<QComboBox*>("toolbarCombo");auto *available=page.findChild<QListWidget*>("availableList");auto *chosen=page.findChild<QListWidget*>("chosenList");
 check(combo&&available&&chosen,"commands19 real toolbar customization UI is available");
 combo->setCurrentIndex(combo->findData("elementeditor_commands19"));check(chosen->count()==1&&chosen->item(0)->data(Qt::UserRole).toString()==symbolId,"commands19 customization restores symbol toolbar choices");
 bool diagramLeaked=false;for(int i=0;i<available->count();++i)diagramLeaked|=available->item(i)->data(Qt::UserRole).toString().startsWith("diagrameditor.");check(!diagramLeaked,"commands19 symbol toolbar available list excludes diagram-only commands");
 combo->setCurrentIndex(combo->findData("toolbar"));bool foundAlignment=false;for(int i=0;i<available->count();++i)foundAlignment|=available->item(i)->data(Qt::UserRole).toString()==actionId;check(foundAlignment,"commands19 diagram available list displays the new alignment command");
 page.resize(1000,700);check(page.grab().save(out+"/toolbar-customization19.png"),"commands19 records toolbar customization example");

 DiagramToolbarSettings::setCustomToolbars(oldCustom);
 ShortcutManager::instance().resetToDefault(actionId);
 for(const auto &key:settingKeys){if(existed.contains(key))settings.setValue(key,saved.value(key));else settings.remove(key);}
}
#endif
