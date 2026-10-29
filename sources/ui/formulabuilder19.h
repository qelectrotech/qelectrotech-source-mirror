// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef QET_FORMULABUILDER19_H
#define QET_FORMULABUILDER19_H
#include <QtWidgets>
#include <QtSql>
#include <functional>
#include <cmath>

// A deliberately bounded, read-only expression language. Fields come from the
// caller's actual query context; neither identifiers nor literal values are
// assembled from the user's SQL syntax. The same tree drives UI and tests.
namespace QetFormula19 {
struct Field { QString label, expression; };
using Preview = std::function<QString(const QString&)>;
inline QString quoteText(QString value) {value.replace('\'',"''");return '\''+value+'\'';}
inline QVariantMap field(const QString &expression) {return {{"op","field"},{"value",expression}};}
inline QVariantMap text(const QString &value) {return {{"op","text"},{"value",value}};}
inline QVariantMap number(double value) {return {{"op","number"},{"value",value}};}
inline QVariantMap operation(const QString &op,const QVariantList &arguments={},const QVariantMap &parameters={}) {
    auto node=parameters;node["op"]=op;node["args"]=arguments;return node;
}
struct Function {QString id,category,label,help;int minimum,maximum;QStringList arguments;};
inline QList<Function> functions() {
    return {
        {"field",QObject::tr("Values"),QObject::tr("Use a field"),QObject::tr("Choose information available in this table's current source."),0,0,{}},
        {"text",QObject::tr("Values"),QObject::tr("Fixed text"),QObject::tr("Enter ordinary text. Apostrophes and other punctuation are quoted automatically."),0,0,{}},
        {"number",QObject::tr("Values"),QObject::tr("Number"),QObject::tr("Enter a numeric value using the number control."),0,0,{}},
        {"null",QObject::tr("Values"),QObject::tr("Empty / NULL value"),QObject::tr("An absent value; use Is empty to test absent or blank text."),0,0,{}},
        {"eq",QObject::tr("Comparisons"),QObject::tr("Equals"),QObject::tr("Compare two values. A Number operand uses numeric comparison."),2,2,{QObject::tr("Value"),QObject::tr("Equals")}},
        {"ne",QObject::tr("Comparisons"),QObject::tr("Differs from"),QObject::tr("True when the two values differ."),2,2,{QObject::tr("Value"),QObject::tr("Differs from")}},
        {"gt",QObject::tr("Comparisons"),QObject::tr("Greater than"),QObject::tr("Compare numeric values."),2,2,{QObject::tr("Number"),QObject::tr("Greater than")}},
        {"ge",QObject::tr("Comparisons"),QObject::tr("Greater than or equal to"),QObject::tr("Compare numeric values."),2,2,{QObject::tr("Number"),QObject::tr("At least")}},
        {"lt",QObject::tr("Comparisons"),QObject::tr("Smaller than"),QObject::tr("Compare numeric values."),2,2,{QObject::tr("Number"),QObject::tr("Smaller than")}},
        {"le",QObject::tr("Comparisons"),QObject::tr("Smaller than or equal to"),QObject::tr("Compare numeric values."),2,2,{QObject::tr("Number"),QObject::tr("At most")}},
        {"between",QObject::tr("Comparisons"),QObject::tr("Between two numbers"),QObject::tr("The lower and upper limits are included."),3,3,{QObject::tr("Number"),QObject::tr("Lower limit"),QObject::tr("Upper limit")}},
        {"empty",QObject::tr("Text conditions"),QObject::tr("Is empty"),QObject::tr("True for NULL, empty text or text containing only spaces."),1,1,{QObject::tr("Value")}},
        {"notempty",QObject::tr("Text conditions"),QObject::tr("Is not empty"),QObject::tr("True when a value contains non-space characters."),1,1,{QObject::tr("Value")}},
        {"contains",QObject::tr("Text conditions"),QObject::tr("Contains text"),QObject::tr("Search literal text; % and _ are ordinary characters. SQLite case conversion applies."),2,2,{QObject::tr("Text"),QObject::tr("Contains")}},
        {"starts",QObject::tr("Text conditions"),QObject::tr("Starts with"),QObject::tr("Compare the beginning of the text with a literal value."),2,2,{QObject::tr("Text"),QObject::tr("Starts with")}},
        {"ends",QObject::tr("Text conditions"),QObject::tr("Ends with"),QObject::tr("Compare the end of the text with a literal value."),2,2,{QObject::tr("Text"),QObject::tr("Ends with")}},
        {"if",QObject::tr("Logic"),QObject::tr("IF ... THEN ... ELSE"),QObject::tr("Choose a condition, the result when true, and the result otherwise. Results may contain further functions."),3,3,{QObject::tr("IF condition"),QObject::tr("THEN use"),QObject::tr("ELSE use")}},
        {"and",QObject::tr("Logic"),QObject::tr("AND - all conditions"),QObject::tr("All child conditions must be true. Add conditions or nest AND/OR groups."),2,99,{QObject::tr("Condition 1"),QObject::tr("Condition 2")}},
        {"or",QObject::tr("Logic"),QObject::tr("OR - any condition"),QObject::tr("At least one child condition must be true. Parentheses are generated automatically."),2,99,{QObject::tr("Condition 1"),QObject::tr("Condition 2")}},
        {"not",QObject::tr("Logic"),QObject::tr("NOT - reverse condition"),QObject::tr("True when its condition is false."),1,1,{QObject::tr("Condition")}},
        {"fallback",QObject::tr("Text functions"),QObject::tr("If empty, use another value"),QObject::tr("Use the first value unless it is NULL, empty or only spaces; otherwise use the fallback."),2,2,{QObject::tr("Preferred value"),QObject::tr("Fallback value")}},
        {"join",QObject::tr("Text functions"),QObject::tr("Combine text / fields"),QObject::tr("Join two or more values using the chosen separator. Empty values are retained."),2,99,{QObject::tr("Text / field 1"),QObject::tr("Text / field 2")}},
        {"replace",QObject::tr("Text functions"),QObject::tr("Replace text"),QObject::tr("Replace each occurrence of the search text with the replacement."),3,3,{QObject::tr("Original text"),QObject::tr("Find text"),QObject::tr("Replacement")}},
        {"upper",QObject::tr("Text functions"),QObject::tr("UPPER CASE"),QObject::tr("Convert text using SQLite's built-in uppercase conversion."),1,1,{QObject::tr("Text")}},
        {"lower",QObject::tr("Text functions"),QObject::tr("lower case"),QObject::tr("Convert text using SQLite's built-in lowercase conversion."),1,1,{QObject::tr("Text")}},
        {"trim",QObject::tr("Text functions"),QObject::tr("Remove surrounding spaces"),QObject::tr("Remove spaces from the beginning and end of text."),1,1,{QObject::tr("Text")}},
        {"add",QObject::tr("Numbers"),QObject::tr("Add"),QObject::tr("Add numeric values."),2,2,{QObject::tr("Number 1"),QObject::tr("Number 2")}},
        {"subtract",QObject::tr("Numbers"),QObject::tr("Subtract"),QObject::tr("Subtract the second number from the first."),2,2,{QObject::tr("Number 1"),QObject::tr("Subtract")}},
        {"multiply",QObject::tr("Numbers"),QObject::tr("Multiply"),QObject::tr("Multiply two numeric values."),2,2,{QObject::tr("Number 1"),QObject::tr("Multiply by")}},
        {"divide",QObject::tr("Numbers"),QObject::tr("Divide safely"),QObject::tr("Divide by the second number. A zero denominator gives an empty value instead of an error."),2,2,{QObject::tr("Number"),QObject::tr("Divide by")}},
        {"round",QObject::tr("Numbers"),QObject::tr("Round number"),QObject::tr("Round to the requested number of decimal places."),2,2,{QObject::tr("Number"),QObject::tr("Decimal places")}},
        {"date",QObject::tr("Dates"),QObject::tr("Format date"),QObject::tr("Choose a display format for an ISO date, such as 2026-10-28. An invalid date gives an empty result."),1,1,{QObject::tr("Date")}},
        {"day",QObject::tr("Dates"),QObject::tr("Day of month"),QObject::tr("Extract the day from an ISO date."),1,1,{QObject::tr("Date")}},
        {"month",QObject::tr("Dates"),QObject::tr("Month"),QObject::tr("Extract the month number from an ISO date."),1,1,{QObject::tr("Date")}},
        {"year",QObject::tr("Dates"),QObject::tr("Year"),QObject::tr("Extract the year from an ISO date."),1,1,{QObject::tr("Date")}},
        {"sum",QObject::tr("Grouping"),QObject::tr("Sum values"),QObject::tr("Sum numeric values in the current group. Available only in an explicitly grouped context."),1,1,{QObject::tr("Number field")}},
        {"count",QObject::tr("Grouping"),QObject::tr("Count rows"),QObject::tr("Count rows in the current group."),0,0,{}},
        {"countdistinct",QObject::tr("Grouping"),QObject::tr("Count distinct values"),QObject::tr("Count different non-NULL values in the group."),1,1,{QObject::tr("Field")}},
        {"listdistinct",QObject::tr("Grouping"),QObject::tr("List distinct values"),QObject::tr("List different nonblank values in the group, separated by commas."),1,1,{QObject::tr("Field")}}
    };
}
inline Function function(const QString &id) {for(const auto &f:functions())if(f.id==id)return f;return {};}
inline bool aggregate(const QString &op) {return QStringList{"sum","count","countdistinct","listdistinct"}.contains(op);}
inline bool condition(const QString &op) {return QStringList{"eq","ne","gt","ge","lt","le","between","empty","notempty","contains","starts","ends","and","or","not"}.contains(op);}
// SQL arithmetic produces REAL values even when a source field is TEXT.
// Infer the result of a calculation, not just a direct numeric literal, so
// Quantity='2' also equals (1 * 2). Plain text stays textual (002 != 2).
inline bool numericResult(const QVariantMap &node,int depth=0) {
    if(depth>40)return false;
    const QString op=node.value("op").toString();
    if(QStringList{"number","add","subtract","multiply","divide","round","sum","count","countdistinct"}.contains(op)||condition(op))return true;
    const auto args=node.value("args").toList();
    auto numericOrNull=[&](int index){const auto child=args.value(index).toMap();return child.value("op")=="null"||numericResult(child,depth+1);};
    if(op=="if"&&args.size()==3)return numericOrNull(1)&&numericOrNull(2);
    if(op=="fallback"&&args.size()==2)return numericOrNull(0)&&numericOrNull(1);
    return false;
}
inline QString textValue(const QString &sql) {return "COALESCE(CAST(("+sql+") AS TEXT),'')";}
inline QString numericValue(const QString &sql) {return "CAST(REPLACE(TRIM(CAST(("+sql+") AS TEXT)),',','.') AS REAL)";}
inline QString compile(const QVariantMap &node,const QList<Field> &fields,bool allowAggregate=false,QString *error=nullptr,int depth=0) {
    auto fail=[&](const QString &why){if(error)*error=why;return QString();};
    if(depth>40)return fail(QObject::tr("The formula is too deeply nested (maximum 40 levels)."));
    const QString op=node.value("op").toString();const auto f=function(op);if(f.id.isEmpty())return fail(QObject::tr("Choose a supported operation."));
    const auto arguments=node.value("args").toList();if(arguments.size()<f.minimum||arguments.size()>f.maximum)return fail(QObject::tr("%1 needs %2 to %3 inputs.").arg(f.label).arg(f.minimum).arg(f.maximum));
    if(aggregate(op)&&!allowAggregate)return fail(QObject::tr("Grouping functions require an explicitly grouped source. Use the grouped-BOM field controls for this table."));
    if(op=="field") {const QString expression=node.value("value").toString();for(const auto &field:fields)if(field.expression==expression&&!expression.isEmpty())return "("+expression+")";return fail(QObject::tr("This field is unavailable in the current table source. Choose a listed field."));}
    if(op=="text")return quoteText(node.value("value").toString());
    if(op=="number") {bool ok=false;const double n=node.value("value").toDouble(&ok);if(!ok||!std::isfinite(n))return fail(QObject::tr("Enter a finite number."));return QString::number(n,'g',16);}
    if(op=="null")return "NULL";
    QStringList a;for(const auto &child:arguments){QString reason;auto sql=compile(child.toMap(),fields,aggregate(op)?false:allowAggregate,&reason,depth+1);if(sql.isEmpty())return fail(reason);a<<sql;}
    const auto tx=[&](int i){return textValue(a.value(i));};const auto nm=[&](int i){return numericValue(a.value(i));};
    if(op=="eq"||op=="ne") {const bool numeric=numericResult(arguments[0].toMap())||numericResult(arguments[1].toMap());return "("+(numeric?nm(0):tx(0))+(op=="eq"?" = ":" <> ")+(numeric?nm(1):tx(1))+")";}
    if(QStringList{"gt","ge","lt","le"}.contains(op)){const QMap<QString,QString> operators{{"gt",">"},{"ge",">="},{"lt","<"},{"le","<="}};return "("+nm(0)+" "+operators.value(op)+" "+nm(1)+")";}
    if(op=="between")return "("+nm(0)+" BETWEEN "+nm(1)+" AND "+nm(2)+")";
    if(op=="empty"||op=="notempty")return "(TRIM("+tx(0)+") "+(op=="empty"?"=":"<>")+" '')";
    if(op=="contains")return "(INSTR(LOWER("+tx(0)+"),LOWER("+tx(1)+")) > 0)";
    if(op=="starts")return "(SUBSTR(LOWER("+tx(0)+"),1,LENGTH("+tx(1)+")) = LOWER("+tx(1)+"))";
    if(op=="ends")return "(SUBSTR(LOWER("+tx(0)+"),LENGTH("+tx(0)+")-LENGTH("+tx(1)+")+1) = LOWER("+tx(1)+"))";
    if(op=="if")return "(CASE WHEN COALESCE(("+a[0]+"),0) THEN ("+a[1]+") ELSE ("+a[2]+") END)";
    if(op=="and"||op=="or"){QStringList terms;for(const auto &sql:a)terms<<"COALESCE(("+sql+"),0)";return '('+terms.join(op=="and"?" AND ":" OR ")+')';}
    if(op=="not")return "(NOT COALESCE(("+a[0]+"),0))";
    if(op=="fallback")return "(CASE WHEN TRIM("+tx(0)+") = '' THEN ("+a[1]+") ELSE ("+a[0]+") END)";
    if(op=="join") {QStringList parts;for(int i=0;i<a.size();++i)parts<<tx(i);return '('+parts.join(" || "+quoteText(node.value("separator"," ").toString())+" || ")+')';}
    if(op=="replace")return "REPLACE("+tx(0)+","+tx(1)+","+tx(2)+")";
    if(op=="upper"||op=="lower"||op=="trim")return op.toUpper()+"("+tx(0)+")";
    if(QStringList{"add","subtract","multiply","divide"}.contains(op)){const QMap<QString,QString> operators{{"add","+"},{"subtract","-"},{"multiply","*"},{"divide","/"}};return "("+nm(0)+" "+operators.value(op)+" "+(op=="divide"?"NULLIF("+nm(1)+",0)":nm(1))+")";}
    if(op=="round")return "ROUND("+nm(0)+",CAST(("+a[1]+") AS INTEGER))";
    if(op=="date"||op=="day"||op=="month"||op=="year") {QString format=op=="day"?"%d":op=="month"?"%m":op=="year"?"%Y":node.value("format","%d.%m.%Y").toString();if(!QStringList{"%d.%m.%Y","%Y-%m-%d","%d/%m/%Y","%m/%d/%Y","%Y-%m-%d %H:%M","%d","%m","%Y"}.contains(format))return fail(QObject::tr("Choose a listed date format."));return "strftime("+quoteText(format)+","+a[0]+")";}
    if(op=="sum")return "SUM("+nm(0)+")";
    if(op=="count")return "COUNT(*)";
    if(op=="countdistinct")return "COUNT(DISTINCT ("+a[0]+"))";
    if(op=="listdistinct")return "GROUP_CONCAT(DISTINCT NULLIF(TRIM("+tx(0)+"),''))";
    return fail(QObject::tr("Unsupported operation."));
}
inline QVariantMap defaultNode(const QString &op,const QList<Field> &fields) {
    const auto first=fields.isEmpty()?text(""):field(fields.first().expression);
    QVariantMap result=operation(op);QVariantList args;const auto f=function(op);
    for(int i=0;i<f.minimum;++i)args<<QVariant(first);
    if(op=="field")result["value"]=fields.isEmpty()?QString():fields.first().expression;
    if(op=="text")result["value"]="";
    if(op=="number")result["value"]=1;
    if(op=="if")args={operation("empty",{first}),text(""),first};
    if(op=="and"||op=="or")args={operation("notempty",{first}),operation("notempty",{first})};
    if(op=="not")args={operation("notempty",{first})};
    if(QStringList{"eq","ne","contains","starts","ends"}.contains(op))args={first,text("")};
    if(QStringList{"gt","ge","lt","le","add","subtract","multiply","divide"}.contains(op))args={first,number(1)};
    if(op=="between")args={first,number(0),number(100)};
    if(op=="replace")args={first,text(""),text("")};
    if(op=="round")args={first,number(2)};
    result["args"]=args;return result;
}

class NodeDialog : public QDialog {
    QList<Field> fields;bool allowAggregate;QComboBox *category,*functionBox,*fieldBox,*dateFormat;
    QLineEdit *literal,*separator;QDoubleSpinBox *numeric;QLabel *help;
    QVariantMap original,result;
    void updateFunctions(const QString &wanted=QString()) {
        QSignalBlocker blocker(functionBox);functionBox->clear();
        for(const auto &f:functions())if(f.category==category->currentText()) {functionBox->addItem(f.label,f.id);auto *model=qobject_cast<QStandardItemModel*>(functionBox->model());if(aggregate(f.id)&&!allowAggregate){auto *item=model->item(functionBox->count()-1);item->setEnabled(false);item->setToolTip(tr("Use the grouped-BOM field controls; this table already contains result rows."));}}
        int index=functionBox->findData(wanted);if(index<0)index=0;functionBox->setCurrentIndex(index);updateInputs();
    }
    void updateInputs() {
        const QString op=functionBox->currentData().toString();const auto f=function(op);help->setText(f.help);
        fieldBox->setVisible(op=="field");literal->setVisible(op=="text");numeric->setVisible(op=="number");separator->setVisible(op=="join");dateFormat->setVisible(op=="date");
    }
public:
    NodeDialog(QWidget *parent,const QList<Field> &catalog,const QVariantMap &node,bool aggregates):QDialog(parent),fields(catalog),allowAggregate(aggregates),original(node) {
        setObjectName("formula19NodeDialog");setWindowTitle(tr("Choose an operation or value"));resize(610,340);
        auto *layout=new QVBoxLayout(this);auto *form=new QFormLayout;layout->addLayout(form);
        category=new QComboBox;category->setObjectName("formula19Category");for(const auto &f:functions())if(category->findText(f.category)<0)category->addItem(f.category);
        functionBox=new QComboBox;functionBox->setObjectName("formula19Operation");form->addRow(tr("Category"),category);form->addRow(tr("Operation"),functionBox);
        fieldBox=new QComboBox;fieldBox->setObjectName("formula19Field");for(const auto &f:fields)fieldBox->addItem(f.label,f.expression);form->addRow(fieldBox);
        literal=new QLineEdit(node.value("value").toString());literal->setObjectName("formula19Text");literal->setPlaceholderText(tr("Enter ordinary text"));form->addRow(literal);
        numeric=new QDoubleSpinBox;numeric->setObjectName("formula19Number");numeric->setDecimals(6);numeric->setRange(-1e12,1e12);numeric->setValue(node.value("value",1).toDouble());form->addRow(numeric);
        separator=new QLineEdit(node.value("separator"," ").toString());separator->setObjectName("formula19Separator");separator->setPlaceholderText(tr("Separator between values (for example: space / dash)"));form->addRow(separator);
        dateFormat=new QComboBox;dateFormat->setObjectName("formula19DateFormat");for(const auto &pair:QList<QPair<QString,QString>>{{"28.10.2026","%d.%m.%Y"},{"2026-10-28","%Y-%m-%d"},{"28/10/2026","%d/%m/%Y"},{"10/28/2026","%m/%d/%Y"},{"2026-10-28 14:30","%Y-%m-%d %H:%M"}})dateFormat->addItem(pair.first,pair.second);dateFormat->setCurrentIndex(qMax(0,dateFormat->findData(node.value("format","%d.%m.%Y"))));form->addRow(dateFormat);
        help=new QLabel;help->setWordWrap(true);layout->addWidget(help);layout->addStretch();
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
        connect(buttons,&QDialogButtonBox::accepted,this,[this](){const QString op=functionBox->currentData().toString();if(aggregate(op)&&!allowAggregate)return;result=defaultNode(op,fields);if(original.value("op")==op)result=original;const auto f=function(op);auto args=result.value("args").toList();while(args.size()<f.minimum)args<<text("");while(args.size()>f.maximum)args.removeLast();result["args"]=args;if(op=="field")result["value"]=fieldBox->currentData();if(op=="text")result["value"]=literal->text();if(op=="number")result["value"]=numeric->value();if(op=="join")result["separator"]=separator->text();if(op=="date")result["format"]=dateFormat->currentData();accept();});
        connect(category,&QComboBox::currentTextChanged,this,[this](){updateFunctions();});connect(functionBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](){updateInputs();});
        const auto f=function(node.value("op","field").toString());category->setCurrentText(f.category);updateFunctions(f.id);int fi=fieldBox->findData(node.value("value"));if(fi>=0)fieldBox->setCurrentIndex(fi);
    }
    QVariantMap node() const{return result;}
};

class Dialog : public QDialog {
    QList<Field> fields;bool conditionOnly,allowAggregate;Preview preview;
    QVariantMap root;QTreeWidget *tree;QPlainTextEdit *sqlView;QLabel *help,*previewResult,*error;
    QTableWidget *sample;QPushButton *addArgument,*removeArgument,*acceptButton;
    QString compiled;
    QList<int> itemPath(QTreeWidgetItem *item) const {QList<int> path;while(item&&item->parent()){auto *parent=item->parent();path.prepend(parent->indexOfChild(item));item=parent;}return path;}
    QVariantMap at(const QList<int> &path)const {auto result=root;for(int i:path)result=result.value("args").toList().value(i).toMap();return result;}
    static QVariantMap replaceAt(QVariantMap node,const QList<int> &path,const QVariantMap &replacement,int depth=0) {if(depth==path.size())return replacement;auto args=node.value("args").toList();if(path[depth]>=0&&path[depth]<args.size())args[path[depth]]=replaceAt(args[path[depth]].toMap(),path,replacement,depth+1);node["args"]=args;return node;}
    void buildTree(QTreeWidgetItem *parent,const QVariantMap &node,const QString &argument=QString()) {
        const auto f=function(node.value("op").toString());auto *item=parent?new QTreeWidgetItem(parent):new QTreeWidgetItem(tree);
        QString value;if(f.id=="field"){for(const auto &field:fields)if(field.expression==node.value("value"))value=field.label;}
        else if(f.id=="text")value='"'+node.value("value").toString()+'"';else if(f.id=="number")value=node.value("value").toString();else if(f.id=="join")value=tr("Separator: %1").arg(node.value("separator"," ").toString());else if(f.id=="date")value=node.value("format","%d.%m.%Y").toString();
        item->setText(0,argument.isEmpty()?f.label:argument+": "+f.label);item->setText(1,value);item->setToolTip(0,f.help);const auto children=node.value("args").toList();for(int i=0;i<children.size();++i)buildTree(item,children[i].toMap(),f.arguments.value(i,tr("Input %1").arg(i+1)));
    }
    void refreshTree(const QList<int> &selection={}) {tree->clear();buildTree(nullptr,root);tree->expandAll();auto *item=tree->topLevelItem(0);for(int i:selection)if(item&&i<item->childCount())item=item->child(i);tree->setCurrentItem(item);refresh();}
    void editSelected() {auto *item=tree->currentItem();if(!item)return;const auto path=itemPath(item);NodeDialog dialog(this,fields,at(path),allowAggregate);if(dialog.exec()!=QDialog::Accepted)return;const auto node=dialog.node();if(path.isEmpty()&&conditionOnly&&!condition(node.value("op").toString())){error->setText(tr("A filter needs a comparison or an AND / OR / NOT condition at the top."));return;}root=replaceAt(root,path,node);refreshTree(path);}
    void refreshSelection() {auto *item=tree->currentItem();auto path=itemPath(item);const auto f=function(at(path).value("op").toString());help->setText(f.help);addArgument->setEnabled(f.maximum>f.minimum&&at(path).value("args").toList().size()<f.maximum);removeArgument->setEnabled(item&&item->parent()&&at(itemPath(item->parent())).value("args").toList().size()>function(at(itemPath(item->parent())).value("op").toString()).minimum);}
    QVariantMap sampleTree(QVariantMap node)const {if(node.value("op")=="field"){const QString expression=node.value("value").toString();for(int row=0;row<fields.size();++row)if(fields[row].expression==expression){const QString value=sample->item(row,1)->text();return text(value);}}auto args=node.value("args").toList();for(int i=0;i<args.size();++i)args[i]=sampleTree(args[i].toMap());node["args"]=args;return node;}
    void refresh() {
        QString reason;compiled=compile(root,fields,allowAggregate,&reason);if(conditionOnly&&!condition(root.value("op").toString()))reason=tr("Choose a comparison or logical group for this filter.");error->setText(reason);sqlView->setPlainText(compiled);acceptButton->setEnabled(reason.isEmpty()&&!compiled.isEmpty());
        if(!reason.isEmpty()){previewResult->clear();return;}
        if(preview){previewResult->setText(preview(compiled));return;}
        QString sampleSql=compile(sampleTree(root),{},allowAggregate,&reason);const QString connection="formula19-preview-"+QUuid::createUuid().toString(QUuid::WithoutBraces);QString result;
        {auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(":memory:");if(db.open()){QSqlQuery query(db);if(query.exec("SELECT "+sampleSql)&&query.next())result=query.value(0).isNull()?tr("(empty / NULL)"):query.value(0).toString();else result=tr("Unable to calculate this example: %1").arg(query.lastError().text());}else result=tr("Example preview is unavailable.");}
        QSqlDatabase::removeDatabase(connection);previewResult->setText(tr("Example result (one sample row): %1").arg(result));
    }
public:
    Dialog(QWidget *parent,const QList<Field> &catalog,const QString &initialSql=QString(),bool conditions=false,bool aggregates=false,const Preview &callback={}):QDialog(parent),fields(catalog),conditionOnly(conditions),allowAggregate(aggregates),preview(callback) {
        setObjectName("formula19Builder");setWindowTitle(conditions?tr("Build filter without writing SQL"):tr("Build a formula without writing SQL"));resize(940,760);
        root=defaultNode(conditions?"eq":"field",fields);auto *layout=new QVBoxLayout(this);
        auto *intro=new QLabel(tr("Select a row and choose Change selected to pick a field, value or function. Inputs appear underneath it. Functions can contain other functions; brackets and quotation marks are generated automatically."));intro->setWordWrap(true);layout->addWidget(intro);
        if(!initialSql.trimmed().isEmpty()){auto *existing=new QLabel(tr("Your current expression is preserved until you click Use formula. The builder starts a new expression; it does not guess a tree from existing SQL."));existing->setWordWrap(true);layout->addWidget(existing);}
        tree=new QTreeWidget;tree->setObjectName("formula19Tree");tree->setHeaderLabels({tr("Operation / input"),tr("Chosen value")});tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);tree->header()->setStretchLastSection(true);layout->addWidget(tree,2);
        auto *buttons=new QHBoxLayout;auto *edit=new QPushButton(tr("Change selected..."));edit->setObjectName("formula19EditNode");buttons->addWidget(edit);addArgument=new QPushButton(tr("Add input / condition"));addArgument->setObjectName("formula19AddInput");buttons->addWidget(addArgument);removeArgument=new QPushButton(tr("Remove input"));removeArgument->setObjectName("formula19RemoveInput");buttons->addWidget(removeArgument);buttons->addStretch();layout->addLayout(buttons);
        help=new QLabel;help->setWordWrap(true);layout->addWidget(help);
        connect(edit,&QPushButton::clicked,this,[this](){editSelected();});connect(tree,&QTreeWidget::itemDoubleClicked,this,[this](){editSelected();});connect(tree,&QTreeWidget::currentItemChanged,this,[this](){refreshSelection();});
        connect(addArgument,&QPushButton::clicked,this,[this](){auto path=itemPath(tree->currentItem());auto node=at(path);auto args=node.value("args").toList();const auto f=function(node.value("op").toString());if(args.size()>=f.maximum)return;args<<QVariant(node.value("op")=="and"||node.value("op")=="or"?defaultNode("eq",fields):text(""));node["args"]=args;root=replaceAt(root,path,node);refreshTree(path);});
        connect(removeArgument,&QPushButton::clicked,this,[this](){auto *item=tree->currentItem();if(!item||!item->parent())return;auto path=itemPath(item);int index=path.takeLast();auto node=at(path);auto args=node.value("args").toList();if(args.size()<=function(node.value("op").toString()).minimum)return;args.removeAt(index);node["args"]=args;root=replaceAt(root,path,node);refreshTree(path);});
        auto *sampleGroup=new QGroupBox(tr("Example input values (does not change project data)"));auto *sampleLayout=new QVBoxLayout(sampleGroup);sample=new QTableWidget(fields.size(),2);sample->setObjectName("formula19Samples");sample->setHorizontalHeaderLabels({tr("Field"),tr("Example value")});sample->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);sample->setMaximumHeight(145);
        for(int row=0;row<fields.size();++row){auto *label=new QTableWidgetItem(fields[row].label);label->setFlags(label->flags()&~Qt::ItemIsEditable);sample->setItem(row,0,label);auto value=fields[row].label.contains("date",Qt::CaseInsensitive)?"2026-10-28":fields[row].label.contains("Qty",Qt::CaseInsensitive)||fields[row].label.contains("quantity",Qt::CaseInsensitive)?"2":"Example";sample->setItem(row,1,new QTableWidgetItem(value));}sampleLayout->addWidget(sample);sampleGroup->setVisible(!bool(preview));layout->addWidget(sampleGroup);connect(sample,&QTableWidget::cellChanged,this,[this](){refresh();});
        previewResult=new QLabel;previewResult->setObjectName("formula19Preview");previewResult->setWordWrap(true);layout->addWidget(previewResult);error=new QLabel;error->setObjectName("formula19Error");error->setStyleSheet("color:#b00020");error->setWordWrap(true);layout->addWidget(error);
        auto *advanced=new QGroupBox(tr("Generated SQL (read only)"));advanced->setCheckable(true);advanced->setChecked(false);auto *advancedLayout=new QVBoxLayout(advanced);sqlView=new QPlainTextEdit;sqlView->setObjectName("formula19Sql");sqlView->setReadOnly(true);sqlView->setMaximumHeight(100);sqlView->setVisible(false);advancedLayout->addWidget(sqlView);connect(advanced,&QGroupBox::toggled,sqlView,&QWidget::setVisible);layout->addWidget(advanced);
        auto *box=new QDialogButtonBox(QDialogButtonBox::Cancel);acceptButton=box->addButton(conditions?tr("Use filter"):tr("Use formula"),QDialogButtonBox::AcceptRole);acceptButton->setObjectName("formula19Accept");layout->addWidget(box);connect(box,&QDialogButtonBox::rejected,this,&QDialog::reject);connect(box,&QDialogButtonBox::accepted,this,&QDialog::accept);refreshTree();
    }
    QString expression()const{return compiled;}
    QVariantMap expressionTree()const{return root;}
    void setExpressionTree(const QVariantMap &node){root=node;refreshTree();}
};
inline bool editExpression19(QWidget *parent,const QList<Field> &fields,QString &sql,bool conditionOnly=false,bool allowAggregate=false,const Preview &preview={}) {
    Dialog dialog(parent,fields,sql,conditionOnly,allowAggregate,preview);if(dialog.exec()!=QDialog::Accepted)return false;sql=dialog.expression();return !sql.isEmpty();
}
}
#endif
