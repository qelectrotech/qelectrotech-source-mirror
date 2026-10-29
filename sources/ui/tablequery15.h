// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_TABLEQUERY15_H
#define QET_TABLEQUERY15_H
#include <QtWidgets>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "formulabuilder19.h"

namespace QetTableQuery15 {
// Read SQL tokens only at the outer level: quoted commas, nested calls and
// subqueries must not become table columns or clause delimiters.
struct Token { QString word; int start; int end; };
inline QList<Token> tokens(const QString &sql) {
 QList<Token> result;int depth=0;QChar quote;
 for(int i=0;i<sql.size();++i){QChar c=sql[i];
  if(!quote.isNull()){if(c==quote || (quote=='['&&c==']')){if(i+1<sql.size()&&sql[i+1]==c&&quote!='['){++i;continue;}quote=QChar();}continue;}
  if(c=='\''||c=='"'||c=='`'||c=='['){quote=c;continue;}
  if(c=='-'&&i+1<sql.size()&&sql[i+1]=='-'){while(i<sql.size()&&sql[i]!='\n')++i;continue;}
  if(c=='/'&&i+1<sql.size()&&sql[i+1]=='*'){int end=sql.indexOf("*/",i+2);if(end<0)break;i=end+1;continue;}
  if(c=='('){++depth;continue;}if(c==')'){--depth;continue;}
  if(depth==0&&(c.isLetter()||c=='_')){int start=i;while(i+1<sql.size()&&(sql[i+1].isLetterOrNumber()||sql[i+1]=='_'))++i;result<<Token{sql.mid(start,i-start+1).toUpper(),start,i+1};}
 }
 return result;
}
inline QStringList splitColumns(const QString &sql) {
 QStringList parts;int start=0,depth=0;QChar quote;
 for(int i=0;i<sql.size();++i){QChar c=sql[i];
  if(!quote.isNull()){if(c==quote||(quote=='['&&c==']')){if(i+1<sql.size()&&sql[i+1]==c&&quote!='['){++i;continue;}quote=QChar();}continue;}
  if(c=='\''||c=='"'||c=='`'||c=='['){quote=c;continue;}
  if(c=='-'&&i+1<sql.size()&&sql[i+1]=='-'){while(i<sql.size()&&sql[i]!='\n')++i;continue;}
  if(c=='/'&&i+1<sql.size()&&sql[i+1]=='*'){int end=sql.indexOf("*/",i+2);if(end<0)break;i=end+1;continue;}
  if(c=='(')++depth;else if(c==')')--depth;else if(c==','&&depth==0){parts<<sql.mid(start,i-start).trimmed();start=i+1;}
 }
 if(start<sql.size())parts<<sql.mid(start).trimmed();return parts;
}
inline int statementEnd(const QString &sql,int start) {
 int depth=0;QChar quote;
 for(int i=start;i<sql.size();++i){QChar c=sql[i];
  if(!quote.isNull()){if(c==quote||(quote=='['&&c==']')){if(i+1<sql.size()&&sql[i+1]==c&&quote!='['){++i;continue;}quote=QChar();}continue;}
  if(c=='\''||c=='"'||c=='`'||c=='['){quote=c;continue;}
  if(c=='-'&&i+1<sql.size()&&sql[i+1]=='-'){while(i<sql.size()&&sql[i]!='\n')++i;continue;}
  if(c=='/'&&i+1<sql.size()&&sql[i+1]=='*'){int end=sql.indexOf("*/",i+2);if(end<0)break;i=end+1;continue;}
  if(c=='(')++depth;else if(c==')')--depth;else if(c==';'&&depth==0)return i;
 }
 return sql.size();
}
inline QStringList selectColumns(const QString &sql) {
 int start=-1;for(const auto &t:tokens(sql)){
  if(start<0&&t.word=="SELECT"){start=t.end;continue;}
  if(start>=0&&QStringList{"FROM","UNION","EXCEPT","INTERSECT","ORDER","GROUP","LIMIT","WHERE","HAVING","WINDOW"}.contains(t.word))return splitColumns(sql.mid(start,t.start-start));
 }
 return start>=0?splitColumns(sql.mid(start,statementEnd(sql,start)-start)):QStringList();
}
inline QString normalized(const QString &s){return s.trimmed().simplified();}
inline QString alias(const QString &expression) {
 auto ts=tokens(expression);for(int i=ts.size()-1;i>=0;--i)if(ts[i].word=="AS")return expression.mid(ts[i].end).trimmed();return QString();
}
inline QString baseExpression(const QString &expression) {
 auto ts=tokens(expression);for(int i=ts.size()-1;i>=0;--i)if(ts[i].word=="AS")return expression.left(ts[i].start).trimmed();return expression;
}
inline QString quotedName(QString name){name.replace('"',"\"\"");return '"'+name+'"';}
inline QString displayName(const QString &expression){
 auto a=alias(expression);if(a.isEmpty())return expression;
 if(a.size()>1&&((a.front()=='"'&&a.back()=='"')||(a.front()=='\''&&a.back()=='\'')||(a.front()=='['&&a.back()==']')||(a.front()=='`'&&a.back()=='`'))){QChar quote=a.front();a=a.mid(1,a.size()-2);if(quote!='[')a.replace(QString(2,quote),QString(quote));}
 return a;
}
inline QRegularExpression statePattern(){return QRegularExpression("\\s*/\\*qet-table15:([A-Za-z0-9_-]+)\\*/\\s*$");}
inline QString withoutState(QString sql){sql.remove(statePattern());return sql;}
inline QVariantMap state(const QString &sql){auto m=statePattern().match(sql);if(!m.hasMatch())return {};return QJsonDocument::fromJson(QByteArray::fromBase64(m.captured(1).toLatin1(),QByteArray::Base64UrlEncoding)).object().toVariantMap();}
inline QString withState(QString sql,const QVariantMap &state){sql=withoutState(sql);if(sql.trimmed().endsWith(';')){sql=sql.trimmed();sql.chop(1);}return sql+" /*qet-table15:"+QString::fromLatin1(QJsonDocument::fromVariant(state).toJson(QJsonDocument::Compact).toBase64(QByteArray::Base64UrlEncoding|QByteArray::OmitTrailingEquals))+"*/";}
inline QStringList selected(QListWidget *list){QStringList result;for(int i=0;i<list->count();++i)result<<list->item(i)->data(Qt::UserRole).toString();return result;}
inline QListWidgetItem *addItem(QListWidget *available,QList<QListWidgetItem*> &all,const QString &expression,const QString &label=QString()){
 for(auto *item:all)if(item->data(Qt::UserRole).toString()==expression)return item;
 auto *item=new QListWidgetItem(label.isEmpty()?displayName(expression):label,available);item->setData(Qt::UserRole,expression);item->setToolTip(expression);all<<item;return item;
}
inline void restoreSelected(const QStringList &columns,QListWidget *available,QListWidget *chosen,QList<QListWidgetItem*> &all){
 while(auto *item=chosen->takeItem(0))available->addItem(item);
 for(const auto &expr:columns){
  QListWidgetItem *item=nullptr;
  for(int row=0;row<available->count();++row)if(available->item(row)->data(Qt::UserRole).toString()==expr){item=available->takeItem(row);break;}
  if(!item){auto *existing=addItem(available,all,expr);int row=available->row(existing);if(row>=0)item=available->takeItem(row);else {item=new QListWidgetItem(*existing);all<<item;}}
  chosen->addItem(item);
 }
}
inline QVariantList savedFields(const QString &context){return QSettings().value("table15/fields/"+context).toList();}
inline void saveFields(const QString &context,const QVariantList &fields){QSettings().setValue("table15/fields/"+context,fields);}
inline QString fieldExpression(const QVariantMap &field){return field.value("expression").toString()+" AS "+quotedName(field.value("heading",field.value("name")).toString());}
inline void installFields(const QVariantList &fields,QListWidget *available,QList<QListWidgetItem*> &all){for(const auto &f:fields){auto field=f.toMap();addItem(available,all,fieldExpression(field),field.value("name").toString()+QObject::tr(" (custom)"));}}
inline bool hasUnquotedControl18(const QString &sql) {
 QChar quote;
 for(int i=0;i<sql.size();++i){const QChar c=sql[i];
  if(!quote.isNull()){if(c==quote||(quote=='['&&c==']')){if(i+1<sql.size()&&sql[i+1]==c&&quote!='['){++i;continue;}quote=QChar();}continue;}
  if(c=='\''||c=='"'||c=='`'||c=='['){quote=c;continue;}
  if(c==';'||(i+1<sql.size()&&((c=='-'&&sql[i+1]=='-')||(c=='/'&&sql[i+1]=='*'))))return true;
 }
 return false;
}
inline bool fieldValid(const QString &name,const QString &heading,const QString &expression,QString *error=nullptr){
 QString e;if(name.trimmed().isEmpty()||heading.trimmed().isEmpty()||expression.trimmed().isEmpty())e=QObject::tr("Name, heading and SQL expression are required.");
 else if(hasUnquotedControl18(expression)||splitColumns(expression).size()!=1||!alias(expression).isEmpty())e=QObject::tr("Enter one SQL expression without AS, SQL comments or an unquoted semicolon; the heading supplies its alias.");
 else {for(const auto &t:tokens(expression))if(QStringList{"FROM","WHERE","GROUP","ORDER","LIMIT","UNION","INSERT","UPDATE","DELETE","DROP","ATTACH","PRAGMA"}.contains(t.word)){e=QObject::tr("The field must be a single read-only expression, not a full SQL query.");break;}}
 if(error)*error=e;return e.isEmpty();
}
inline bool editField(QWidget *parent,QVariantMap &field,const QString &context,const QStringList &sourceColumns18=QStringList(),const QList<QetFormula19::Field> &catalog19={}){
 QDialog d(parent);d.setObjectName("table15CustomFieldDialog");d.setWindowTitle(QObject::tr("Add reusable custom field"));auto *layout=new QFormLayout(&d);
 auto *info=new QLabel(QObject::tr("Define a column once, then add it from Available fields. The expression uses existing data; it does not create a new device property."));info->setWordWrap(true);layout->addRow(info);
 auto *name=new QLineEdit(field.value("name").toString());name->setObjectName("customFieldName");auto *heading=new QLineEdit(field.value("heading").toString());heading->setObjectName("customFieldHeading");auto *expression=new QLineEdit(field.value("expression").toString());expression->setObjectName("customFieldExpression");
 layout->addRow(QObject::tr("Available-field name"),name);layout->addRow(QObject::tr("Column heading"),heading);layout->addRow(QObject::tr("SQL expression (advanced)"),expression);layout->addRow(QObject::tr("Table type"),new QLabel(context=="summary"?QObject::tr("Contents / summary"):QObject::tr("BOM / nomenclature")));
 auto *examples=new QGroupBox(QObject::tr("Build a field without writing SQL"));auto *examplesLayout=new QFormLayout(examples);
 auto *source=new QComboBox;source->setObjectName("table18ExampleSource");
 if(!catalog19.isEmpty())for(const auto &column:catalog19)source->addItem(column.label,column.expression);
 else if(!sourceColumns18.isEmpty())for(const auto &column:sourceColumns18)source->addItem(column,quotedName(column));
 else if(context=="summary")for(const auto &column:QList<QPair<QString,QString>>{{QObject::tr("Page"),"folio"},{QObject::tr("Page content"),"title"},{QObject::tr("Author"),"author"},{QObject::tr("Date"),"date"},{QObject::tr("Revision"),"indexrev"},{QObject::tr("Plant"),"plant"},{QObject::tr("Location"),"locmach"}})source->addItem(column.first,quotedName(column.second));
 else for(const auto &column:QList<QPair<QString,QString>>{{QObject::tr("Description"),"description"},{QObject::tr("Part number"),"manufacturer_reference"},{QObject::tr("Manufacturer"),"manufacturer"},{QObject::tr("Reference / label"),"label"},{QObject::tr("Quantity"),"quantity"},{QObject::tr("Unit"),"unity"},{QObject::tr("Designation"),"designation"},{QObject::tr("Comments"),"comment"}})source->addItem(column.first,quotedName(column.second));
 auto *operation=new QComboBox;operation->setObjectName("table18ExampleOperation");operation->addItems({QObject::tr("Use field as it is"),QObject::tr("UPPER CASE"),QObject::tr("lower case"),QObject::tr("Fixed text")});
 auto *literal=new QLineEdit;literal->setObjectName("table18ExampleLiteral");literal->setPlaceholderText(QObject::tr("Text for the Fixed text option"));
 auto *use=new QPushButton(QObject::tr("Use this example"));use->setObjectName("table18UseExample");
 examplesLayout->addRow(QObject::tr("Source field"),source);examplesLayout->addRow(QObject::tr("Transformation"),operation);examplesLayout->addRow(QObject::tr("Fixed text"),literal);examplesLayout->addRow(use);layout->addRow(examples);
 QObject::connect(use,&QPushButton::clicked,&d,[&](){QString sql=source->currentData().toString();if(operation->currentIndex()==1)sql="UPPER("+sql+")";else if(operation->currentIndex()==2)sql="LOWER("+sql+")";else if(operation->currentIndex()==3){sql=literal->text();sql.replace('\'',"''");sql="'"+sql+"'";}expression->setText(sql);if(name->text().trimmed().isEmpty())name->setText(operation->currentIndex()==3?QObject::tr("Fixed text"):source->currentText());if(heading->text().trimmed().isEmpty())heading->setText(name->text());});
 QVariantMap formulaTree19=field.value("formulaTree19").toMap();
 auto *build19=new QPushButton(QObject::tr("Formula builder: fields, operators and logical functions..."));build19->setObjectName("table19FormulaBuilder");layout->addRow(build19);
 QObject::connect(build19,&QPushButton::clicked,&d,[&](){QList<QetFormula19::Field> catalog;for(int i=0;i<source->count();++i)catalog<<QetFormula19::Field{source->itemText(i),source->itemData(i).toString()};QetFormula19::Dialog builder(&d,catalog,expression->text());if(!formulaTree19.isEmpty())builder.setExpressionTree(formulaTree19);if(builder.exec()!=QDialog::Accepted)return;formulaTree19=builder.expressionTree();expression->setText(builder.expression());if(name->text().trimmed().isEmpty())name->setText(QObject::tr("Calculated field"));if(heading->text().trimmed().isEmpty())heading->setText(name->text());});
 QObject::connect(expression,&QLineEdit::textChanged,&d,[&](){if(formulaTree19.isEmpty())return;QList<QetFormula19::Field> catalog;for(int i=0;i<source->count();++i)catalog<<QetFormula19::Field{source->itemText(i),source->itemData(i).toString()};if(QetFormula19::compile(formulaTree19,catalog)!=expression->text())formulaTree19.clear();});
 auto *save=new QCheckBox(QObject::tr("Also save for other projects on this computer"));save->setObjectName("customFieldSaveForReuse");save->setChecked(true);layout->addRow(save);
 auto *error=new QLabel;error->setWordWrap(true);error->setStyleSheet("color:#b00020");layout->addRow(error);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addRow(buttons);
 QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);QObject::connect(buttons,&QDialogButtonBox::accepted,&d,[&](){QString reason;if(!fieldValid(name->text(),heading->text(),expression->text(),&reason)){error->setText(reason);return;}field={{"name",name->text().trimmed()},{"heading",heading->text().trimmed()},{"expression",expression->text().trimmed()},{"context",context},{"saveForReuse",save->isChecked()}};if(!formulaTree19.isEmpty())field["formulaTree19"]=formulaTree19;d.accept();});d.resize(720,640);return d.exec()==QDialog::Accepted;
}
}
#endif
