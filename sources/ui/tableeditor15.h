// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEEDITOR15_H
#define QET_TABLEEDITOR15_H
#include <QtWidgets>
#include <QSqlError>
#include "tableformat13.h"
#include "tabledefaults18.h"
#include "../diagram.h"
#include "../qetproject.h"
#include "../dataBase/ui/elementquerywidget.h"
#include "../dataBase/ui/summaryquerywidget.h"
#include "../qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicsheaderitem.h"

namespace QetTableEditor15 {
inline QVariantMap configuration(const QString &kind,const QString &query,const QVariantMap &format) {
 return {{"schema","qet-table-configuration"},{"version",1},{"kind",kind},{"query",query},{"format",format}};
}
inline bool validConfiguration(const QVariantMap &v,const QString &kind,QString *error=nullptr) {
 QString message;
 if(v.value("schema")!="qet-table-configuration" || v.value("version").toInt()!=1)message=QObject::tr("This is not a supported table configuration.");
 else if(v.value("kind").toString()!=kind)message=QObject::tr("This configuration belongs to a different table type.");
 else if(!projectDataBase::isReadOnlySelect(v.value("query").toString(),&message)){}
 else return true;
 if(error)*error=message;return false;
}
class Change : public QUndoCommand {
 QPointer<ProjectDBModel> model;QPointer<QetGraphicsTableItem> table;
 QString beforeQuery,afterQuery;QVariantMap beforeFormat,afterFormat;
 void set(const QString &q,const QVariantMap &f){if(model){const auto old=model->property("qetEditDefinition15");model->setProperty("qetEditDefinition15",true);model->setQuery(q);model->setProperty("qetEditDefinition15",old);}if(table)table->setCustomFormat(f);}
public:
 Change(QetGraphicsTableItem *t,const QString &q,const QVariantMap &f):QUndoCommand(QObject::tr("Edit table definition and formatting")),model(qobject_cast<ProjectDBModel*>(t->model())),table(t),beforeQuery(model?model->queryString():QString()),afterQuery(q),beforeFormat(t->customFormat()),afterFormat(f){}
 void redo()override{set(afterQuery,afterFormat);}void undo()override{set(beforeQuery,beforeFormat);}
};
class Dialog : public QDialog {
 QetGraphicsTableItem *table;ProjectDBModel *original,*draft;
 ElementQueryWidget *bom=nullptr;SummaryQueryWidget *summary=nullptr;QPlainTextEdit *advanced=nullptr;
 QVariantMap currentFormat;QGraphicsScene *scene;QetGraphicsTableItem *preview;QGraphicsView *view;
 QLabel *status;QTabWidget *tabs;QComboBox *presets;QStringList modelHeaders,modelFieldIds19;QString lastQueryError18;QTimer *previewTimer19=nullptr;bool syncingPreview19=false;
 QStringList headers() const {return QetTableDefaults18::columnKeys18(draft);}
 void setQuery(const QString &q){QScopedValueRollback<bool> guard(syncingPreview19,true);if(previewTimer19)previewTimer19->stop();if(bom)bom->setQuery(q);else if(summary)summary->setQuery(q);else advanced->setPlainText(q);}
 QStringList fieldIds19() const {return bom?bom->selectedFieldIds19():summary?summary->selectedFieldIds19():QStringList();}
 void schedulePreview19(){if(syncingPreview19||!previewTimer19)return;status->setStyleSheet(QString());status->setText(tr("Updating preview from the current fields and filters..."));previewTimer19->start();}
 void remapColumns(const QStringList &next) {
  QStringList nextIds=fieldIds19();if(nextIds.size()!=next.size())nextIds=next;
  if(modelHeaders==next&&modelFieldIds19==nextIds)return;
  QVector<int> previousColumns;QSet<int> usedColumns;
  for(int i=0;i<next.size();++i){int match=-1;
   for(int c=0;c<modelHeaders.size();++c)if(!usedColumns.contains(c)&&c<modelFieldIds19.size()&&!nextIds[i].isEmpty()&&modelFieldIds19[c]==nextIds[i]){match=c;break;}
   if(match<0)for(int c=0;c<modelHeaders.size();++c)if(!usedColumns.contains(c)&&modelHeaders[c]==next[i]){match=c;break;}
   if(match>=0)usedColumns.insert(match);previousColumns<<match;
  }
  for(const QString &key:QStringList{"widths","headers","alignments","headerAlignments","autoWidths","wraps","overflowModes","columnOverrides17"}) {
   auto old=currentFormat.value(key).toList();if(old.isEmpty())continue;QVariantList values;
   for(int i=0;i<next.size();++i){int index=previousColumns[i];QVariant fallback;
    if(key=="widths")fallback=100;else if(key=="headers")fallback=next[i];else if(key=="alignments")fallback=int(Qt::AlignLeft);else if(key=="headerAlignments")fallback=int(Qt::AlignHCenter);else if(key=="overflowModes")fallback="clip";else if(key=="columnOverrides17")fallback=QVariantMap();else fallback=false;
    QVariant value=(index>=0&&index<old.size())?old[index]:fallback;
    // A stored caption equal to the old SQL heading is the automatic caption,
    // so a heading edit must update it too. Preserve a separately customised
    // display caption rather than silently replacing it during a reorder.
    if(key=="headers"&&index>=0&&index<modelHeaders.size()&&value.toString()==modelHeaders[index])value=next[i];
    values<<value;
   }currentFormat[key]=values;
  }auto cells=currentFormat.value("cellOverrides20").toMap();QVariantMap remapped;
  for(auto it=cells.cbegin();it!=cells.cend();++it){auto parts=it.key().split(':');if(parts.size()!=2)continue;int oldColumn=parts[1].toInt(),newColumn=previousColumns.indexOf(oldColumn);if(newColumn>=0)remapped[parts[0]+":"+QString::number(newColumn)]=it.value();}if(currentFormat.contains("cellOverrides20"))currentFormat["cellOverrides20"]=remapped;
  modelHeaders=next;modelFieldIds19=nextIds;
 }
 QString presetKey() const{return "table15/configurations/"+original->identifier();}
 void reloadPresets(){QString previous=presets->currentText();presets->clear();presets->addItems(QSettings().value(presetKey()).toMap().keys());presets->setCurrentText(previous);}
 void savePreset(){if(!refreshPreview())return;bool ok=false;QString name=QInputDialog::getText(this,tr("Save table configuration"),tr("Configuration name"),QLineEdit::Normal,presets->currentText(),&ok).trimmed();if(!ok||name.isEmpty())return;auto all=QSettings().value(presetKey()).toMap();all[name]=configuration();QSettings().setValue(presetKey(),all);reloadPresets();presets->setCurrentText(name);}
 void loadPreset(){QString error;if(!loadConfiguration(QSettings().value(presetKey()).toMap().value(presets->currentText()).toMap(),&error))QMessageBox::warning(this,tr("Table configuration"),error);}
 void exportConfig(){if(!refreshPreview())return;QString path=QFileDialog::getSaveFileName(this,tr("Export table configuration"),QString(),tr("Table configuration (*.qettable.json)"));if(path.isEmpty())return;if(!path.endsWith(".json"))path+=".qettable.json";QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)||file.write(QJsonDocument::fromVariant(configuration()).toJson())<0||!file.commit())QMessageBox::warning(this,tr("Save configuration"),file.errorString());}
 void importConfig(){QString path=QFileDialog::getOpenFileName(this,tr("Import table configuration"),QString(),tr("Table configuration (*.qettable.json *.json)"));if(path.isEmpty())return;QFile file(path);QString error;if(!file.open(QIODevice::ReadOnly)){QMessageBox::warning(this,tr("Open configuration"),file.errorString());return;}QJsonParseError pe;auto doc=QJsonDocument::fromJson(file.readAll(),&pe);if(pe.error!=QJsonParseError::NoError||!loadConfiguration(doc.toVariant().toMap(),&error))QMessageBox::warning(this,tr("Table configuration"),pe.error!=QJsonParseError::NoError?pe.errorString():error);}
 void formatTable(){if(!refreshPreview())return;QVector<int> widths;for(int i=0;i<draft->columnCount();++i)widths<<preview->headerItem()->sectionSize(i);auto f=currentFormat;
  qreal available=0;if(table->diagram())available=table->diagram()->border_and_titleblock.borderAndTitleBlockRect().right()-table->scenePos().x();
  QFont font=draft->index(0,0).data(Qt::FontRole).value<QFont>();
  if(QetFormatting::editTableFormat(this,f,draft,widths,preview->displayNRow(),font,available)){currentFormat=f;preview->setCustomFormat(f);updatePreviewView();}
 }
 void updatePreviewView(){scene->setSceneRect(preview->boundingRect().adjusted(-8,-8,8,8));view->fitInView(scene->sceneRect(),Qt::KeepAspectRatio);}
 void showQueryError18(){tabs->setCurrentIndex(0);status->setStyleSheet("color:#b02020;font-weight:bold;");if(auto *line=findChild<QLineEdit*>("m_sql_query"))if(line->isEnabled())line->setFocus();}
 bool queryFailure18(const QString &error){lastQueryError18=error;status->setStyleSheet("color:#b02020;font-weight:bold;");status->setText(tr("Cannot apply this table: %1\nYour previous table is unchanged. Correct the fields/query or choose a predefined table.").arg(error));return false;}

public:
 Dialog(QetGraphicsTableItem *t,QWidget *parent=nullptr):QDialog(parent),table(t),original(qobject_cast<ProjectDBModel*>(t->model())),currentFormat(t->customFormat()) {
  setObjectName("tableEditor15");setWindowTitle(tr("Table definition and properties"));resize(1060,780);auto *layout=new QVBoxLayout(this);tabs=new QTabWidget(this);layout->addWidget(tabs);
  auto *definition=new QWidget;auto *dl=new QVBoxLayout(definition);
  if(original->identifier()=="nomenclature"){bom=new ElementQueryWidget(definition);bom->setObjectName("tableDefinition15");dl->addWidget(bom);}else if(original->identifier()=="summary"){summary=new SummaryQueryWidget(definition);summary->setObjectName("tableDefinition15");dl->addWidget(summary);}else {advanced=new QPlainTextEdit;advanced->setObjectName("tableDefinition15");dl->addWidget(new QLabel(tr("Advanced read-only SQL query")));dl->addWidget(advanced);}
  setQuery(original->queryString());auto *definitionScroll=new QScrollArea;definitionScroll->setObjectName("tableDefinitionScroll18");definitionScroll->setWidgetResizable(true);definitionScroll->setFrameShape(QFrame::NoFrame);definitionScroll->setWidget(definition);tabs->addTab(definitionScroll,tr("Fields, filters and SQL"));
  auto *appearance=new QWidget;auto *al=new QVBoxLayout(appearance);auto *format=new QPushButton(tr("Column widths, wrapping, rows, fonts and colours..."));format->setObjectName("tableFormat15");al->addWidget(format);
  status=new QLabel;status->setObjectName("tableStatus15");status->setWordWrap(true);status->setTextFormat(Qt::PlainText);layout->addWidget(status);
  scene=new QGraphicsScene(this);draft=new ProjectDBModel(*original);draft->setParent(this);draft->setIdentifier(original->identifier());modelHeaders=headers();modelFieldIds19=fieldIds19();preview=new QetGraphicsTableItem;preview->setObjectName("previewTable15");scene->addItem(preview);preview->setModel(draft);preview->setDisplayNRow(t->displayNRow());preview->setCustomFormat(currentFormat);
  view=new QGraphicsView(scene);view->setObjectName("tablePreview15");view->setBackgroundBrush(Qt::white);view->setRenderHint(QPainter::Antialiasing);al->addWidget(view,1);auto *refresh=new QPushButton(tr("Update preview from current fields and SQL"));al->addWidget(refresh);tabs->addTab(appearance,tr("Layout and preview"));
  // A resize gesture uses scene coordinates from the current view. Fitting
  // the view (or recentering its scene rect) on each mouse move shifts the
  // pointer's reference. Only repaint/sync during the gesture; the explicit
  // Update preview action updates the scene rect and fits afterwards.
  connect(preview,&QetGraphicsTableItem::customFormatChanged18,this,[this](const QVariantMap &format){currentFormat=format;view->viewport()->update();});
  connect(refresh,&QPushButton::clicked,this,[this](){refreshPreview();});connect(format,&QPushButton::clicked,this,[this](){formatTable();});connect(tabs,&QTabWidget::currentChanged,this,[this](int index){if(index==1)refreshPreview();});
  previewTimer19=new QTimer(this);previewTimer19->setObjectName("tableLivePreviewTimer19");previewTimer19->setSingleShot(true);previewTimer19->setInterval(150);
  connect(previewTimer19,&QTimer::timeout,this,[this](){refreshPreview();});
  if(bom)connect(bom,&ElementQueryWidget::definitionChanged19,this,[this](){schedulePreview19();});
  if(summary)connect(summary,&SummaryQueryWidget::definitionChanged19,this,[this](){schedulePreview19();});
  if(advanced)connect(advanced,&QPlainTextEdit::textChanged,this,[this](){schedulePreview19();});
  auto *reuse=new QHBoxLayout;presets=new QComboBox;presets->setMinimumWidth(180);reuse->addWidget(presets,1);reloadPresets();
  for(auto label:QStringList{tr("Save configuration..."),tr("Load"),tr("Export..."),tr("Import...")}){auto *b=new QPushButton(label);reuse->addWidget(b);int i=reuse->count()-2;connect(b,&QPushButton::clicked,this,[this,i](){if(i==0)savePreset();else if(i==1)loadPreset();else if(i==2)exportConfig();else importConfig();});}layout->addLayout(reuse);
  if(QetTableDefaults18::supportedKind18(original->identifier())) {
   auto *defaults=new QHBoxLayout;defaults->addWidget(new QLabel(tr("New-table defaults:")));
   auto *save=new QPushButton(tr("Save current as default"));save->setObjectName("tableSaveDefault18");defaults->addWidget(save);
   auto *load=new QPushButton(tr("Load default"));load->setObjectName("tableLoadDefault18");defaults->addWidget(load);
   auto *reset=new QPushButton(tr("Restore built-in default"));reset->setObjectName("tableResetDefault18");defaults->addWidget(reset);defaults->addStretch();layout->addLayout(defaults);
   save->setToolTip(tr("Explicitly save this query and formatting for new tables of this type. Saving is retained even if this dialog is later cancelled."));
   reset->setToolTip(tr("Remove the saved preference for new tables of this type. This does not change the current table."));
   connect(save,&QPushButton::clicked,this,[this](){QString error;if(saveCurrentAsDefault18(&error))status->setText(tr("Default saved for new %1 tables. Apply/OK is still needed to change this table. Row overrides remain positional.").arg(original->identifier()=="summary"?tr("Contents"):tr("BOM")));else queryFailure18(error);});
   connect(load,&QPushButton::clicked,this,[this](){QString error;if(!loadDefaultConfiguration18(&error))queryFailure18(error);});
   connect(reset,&QPushButton::clicked,this,[this](){QString error;if(restoreSoftwareDefault18(&error)){status->setStyleSheet(QString());status->setText(tr("New tables of this type now use the built-in default. The current table/draft is unchanged; use Load default to preview it."));}else queryFailure18(error);});
  }
  auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);buttons->button(QDialogButtonBox::Apply)->setObjectName("tableApply15");layout->addWidget(buttons);
  connect(buttons,&QDialogButtonBox::accepted,this,[this](){if(refreshPreview()){applyChanges();accept();}else showQueryError18();});connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(buttons->button(QDialogButtonBox::Apply),&QPushButton::clicked,this,[this](){if(refreshPreview())applyChanges();else showQueryError18();});refreshPreview();
 }
 QString query() const {return bom?bom->queryStr():summary?summary->queryStr():advanced->toPlainText();}
 QVariantMap format() const{return currentFormat;}
 QVariantMap configuration() const{auto result=QetTableEditor15::configuration(original->identifier(),query(),currentFormat);result["columns18"]=headers();return result;}
 bool saveCurrentAsDefault18(QString *error=nullptr){if(!refreshPreview()){if(error)*error=lastQueryError18;return false;}auto saved=configuration();saved["format"]=QetTableDefaults18::snapshotFormat18(table,draft,currentFormat);return QetTableDefaults18::saveDefault18(original->identifier(),saved,error);}
 bool loadDefaultConfiguration18(QString *error=nullptr){return loadConfiguration(QetTableDefaults18::defaultConfiguration18(original->identifier()),error);}
 bool restoreSoftwareDefault18(QString *error=nullptr){return QetTableDefaults18::clearDefault18(original->identifier(),error);}
 bool loadConfiguration(const QVariantMap &v,QString *error=nullptr){if(!validConfiguration(v,original->identifier(),error))return false;setQuery(v.value("query").toString());draft->setQuery(v.value("query").toString());modelHeaders=headers();modelFieldIds19=fieldIds19();currentFormat=v.value("format").toMap();const bool ok=refreshPreview();if(!ok&&error)*error=lastQueryError18;return ok;}
 bool refreshPreview(){if(previewTimer19)previewTimer19->stop();QScopedValueRollback<bool> guard(syncingPreview19,true);QString error=bom?bom->validationMessage19():summary?summary->validationMessage19():QString();if(!error.isEmpty())return queryFailure18(error);QString q=query();if(!projectDataBase::isReadOnlySelect(q,&error))return queryFailure18(error);auto result=original->project()->dataBase()->newQuery(q,&error);if(!result.isActive()||result.lastError().isValid())return queryFailure18(error.isEmpty()?result.lastError().text():error);lastQueryError18.clear();status->setStyleSheet(QString());draft->setQuery(q);remapColumns(headers());preview->setCustomFormat(currentFormat);QString message=tr("%1 data rows, %2 columns. Preview only; changes take effect with Apply or OK.").arg(draft->rowCount()).arg(draft->columnCount());if(table->diagram()){auto available=table->diagram()->border_and_titleblock.borderAndTitleBlockRect().right()-table->scenePos().x();if(preview->boundingRect().width()>available)message+="\n"+tr("Warning: the table is wider than the available sheet space.");}if(currentFormat.value("enabled").toBool()){const auto g=QetFormatting::tableLayout(currentFormat,draft,0,preview->formattedRows(),draft->index(0,0).data(Qt::FontRole).value<QFont>());if(!g.warnings.isEmpty()){auto warnings=g.warnings.mid(0,3);if(g.warnings.size()>3)warnings<<tr("More fit warnings are available in the layout preview.");message+="\n"+warnings.join("\n");}}status->setText(message);updatePreviewView();return true;}
 void openFormatting19(){QTimer::singleShot(0,this,[this](){tabs->setCurrentIndex(1);formatTable();});}
 void openGrouped19(){QTimer::singleShot(0,this,[this](){tabs->setCurrentIndex(0);if(auto *button=findChild<QPushButton*>("groupedBom13Button"))button->click();});}
 void applyChanges(){QetTableDefaults18::rememberFormat20(original->identifier(),currentFormat);if(query()==original->queryString()&&currentFormat==table->customFormat())return;auto *command=new Change(table,query(),currentFormat);if(table->diagram())table->diagram()->undoStack().push(command);else {command->redo();delete command;}}
};
inline bool edit(QetGraphicsTableItem *table,QWidget *parent=nullptr){if(!table||!qobject_cast<ProjectDBModel*>(table->model())||(table->diagram()&&table->diagram()->isReadOnly()))return false;Dialog dialog(table,parent);if(dialog.exec()!=QDialog::Accepted)return false;if(!dialog.refreshPreview())return false;dialog.applyChanges();return true;}
inline bool editFormatting19(QetGraphicsTableItem *table,QWidget *parent=nullptr){if(!table||!qobject_cast<ProjectDBModel*>(table->model())||(table->diagram()&&table->diagram()->isReadOnly()))return false;Dialog dialog(table,parent);dialog.openFormatting19();return dialog.exec()==QDialog::Accepted;}
inline bool editGroupedBom19(QetGraphicsTableItem *table,QWidget *parent=nullptr){auto *model=table?qobject_cast<ProjectDBModel*>(table->model()):nullptr;if(!model||model->identifier()!="nomenclature"||(table->diagram()&&table->diagram()->isReadOnly()))return false;Dialog dialog(table,parent);dialog.openGrouped19();return dialog.exec()==QDialog::Accepted;}
}
#endif
