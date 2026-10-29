// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef RS19_FORMULA_TESTS_H
#define RS19_FORMULA_TESTS_H
#include <QtWidgets>
#include <QtSql>
#include "ui/formulabuilder19.h"
#include "ui/tablequery15.h"

template<class Check> void runFormula19Tests(Check check,const QString &out,const QString &,const QString &) {
    using namespace QetFormula19;
    const QList<Field> fields{{"Description","description"},{"Designation","designation"},{"Quantity","quantity"},{"Date","date"},{"Reference","reference"}};
    const auto description=field("description"),designation=field("designation"),quantity=field("quantity"),date=field("date"),reference=field("reference");
    const QString connection="rs19-formula-tests";
    {
        auto db=QSqlDatabase::addDatabase("QSQLITE",connection);db.setDatabaseName(":memory:");check(db.open(),"formula19 opens isolated SQLite fixture");QSqlQuery query(db);
        check(query.exec("CREATE TABLE parts(description TEXT, designation TEXT, quantity TEXT, date TEXT, reference TEXT)"),"formula19 creates typed source fixture");
        query.prepare("INSERT INTO parts VALUES(?,?,?,?,?)");for(const QVariant &value:QVariantList{QString(" Door's sensor "),QString("BNS-260"),QString("2"),QString("2026-10-28"),QString("-S1")})query.addBindValue(value);check(query.exec(),"formula19 apostrophe data is bound safely");
        check(query.exec("INSERT INTO parts VALUES(NULL,'Relay','3','2026-10-29','-K1'),('   ','Actuator','1','invalid','-S2')"),"formula19 adds blank, NULL and date cases");
        auto sql=[&](const QVariantMap &node,bool grouped=false){QString error;auto value=compile(node,fields,grouped,&error);check(!value.isEmpty()&&error.isEmpty(),qPrintable("formula19 compiles "+node.value("op").toString()+": "+error));return value;};
        auto first=[&](const QVariantMap &node){check(query.exec("SELECT "+sql(node)+" FROM parts ORDER BY rowid"),qPrintable(query.lastError().text()));check(query.next(),"formula19 expression has a first result");return query.value(0);};
        check(first(operation("trim",{description})).toString()=="Door's sensor","formula19 trim removes surrounding spaces");
        check(first(operation("upper",{designation})).toString()=="BNS-260","formula19 uppercase function executes");
        check(first(operation("lower",{designation})).toString()=="bns-260","formula19 lowercase function executes");
        check(first(operation("join",{operation("trim",{description}),reference},{{"separator"," / "}})).toString()=="Door's sensor / -S1","formula19 combines fields with a chosen separator");
        check(first(operation("replace",{description,text("sensor"),text("Customer's switch; -- /* */")})).toString()==" Door's Customer's switch; -- /* */ ","formula19 replacement quotes punctuation without SQL injection");
        check(first(operation("eq",{quantity,number(2)})).toBool(),"formula19 numeric equality handles TEXT quantities");
        const auto calculatedTwo=operation("multiply",{number(1),number(2)});
        check(first(operation("eq",{quantity,calculatedTwo})).toBool(),"formula19 TEXT quantity equals an arithmetic result with REAL output");
        check(!first(operation("ne",{quantity,calculatedTwo})).toBool(),"formula19 differs comparison infers calculated numeric type");
        check(first(operation("eq",{quantity,operation("if",{operation("notempty",{description}),calculatedTwo,number(0)})})).toBool(),"formula19 nested numeric IF result keeps numeric comparison");
        check(!first(operation("eq",{text("002"),text("2")})).toBool(),"formula19 pure text comparison preserves leading zeros");
        check(first(operation("eq",{operation("null"),text("")})).toBool(),"formula19 empty text and NULL comparison semantics remain unchanged");
        check(!first(operation("eq",{operation("null"),calculatedTwo})).toBool(),"formula19 NULL is not equal to a calculated number");
        check(first(operation("ne",{designation,text("Other")})).toBool(),"formula19 differs comparison executes");
        check(first(operation("gt",{quantity,number(1)})).toBool(),"formula19 greater-than uses numeric values");
        check(!first(operation("gt",{quantity,number(10)})).toBool(),"formula19 numeric comparison does not compare text lexically");
        check(first(operation("ge",{quantity,number(2)})).toBool(),"formula19 greater/equal includes boundary");
        check(first(operation("lt",{quantity,number(3)})).toBool(),"formula19 smaller-than executes");
        check(first(operation("le",{quantity,number(2)})).toBool(),"formula19 smaller/equal includes boundary");
        check(first(operation("between",{quantity,number(1),number(2)})).toBool(),"formula19 between includes limits");
        check(first(operation("contains",{description,text("SENSOR")})).toBool(),"formula19 contains handles ordinary case differences");
        check(!first(operation("contains",{description,text("%")})).toBool(),"formula19 percent signs are literal search text");
        check(first(operation("starts",{designation,text("bns")})).toBool(),"formula19 starts-with comparison executes");
        check(first(operation("ends",{designation,text("260")})).toBool(),"formula19 ends-with comparison executes");
        check(first(operation("contains",{text("x_y"),text("_")})).toBool(),"formula19 underscore is literal rather than LIKE wildcard");
        check(first(operation("empty",{operation("null")})).toBool(),"formula19 NULL is empty");
        check(first(operation("empty",{text("   ")})).toBool(),"formula19 spaces are empty");
        check(first(operation("notempty",{description})).toBool(),"formula19 nonempty text is detected");
        const auto fallback=operation("fallback",{description,designation});check(query.exec("SELECT "+sql(fallback)+" FROM parts ORDER BY rowid"),"formula19 fallback query executes");query.next();query.next();check(query.value(0).toString()=="Relay","formula19 NULL description uses fallback");query.next();check(query.value(0).toString()=="Actuator","formula19 whitespace description uses fallback");
        const auto nested=operation("if",{operation("and",{operation("gt",{quantity,number(1)}),operation("not",{operation("empty",{description})})}),operation("join",{operation("trim",{description}),reference},{{"separator"," - "}}),designation});
        check(first(nested).toString()=="Door's sensor - -S1","formula19 nested IF AND NOT composes functions and parentheses");
        check(first(operation("or",{operation("eq",{quantity,number(99)}),operation("eq",{quantity,number(2)})})).toBool(),"formula19 OR groups conditions");
        check(first(operation("add",{quantity,number(1)})).toDouble()==3,"formula19 addition executes");
        check(first(operation("add",{text("1,5"),number(.5)})).toDouble()==2,"formula19 decimal-comma numeric values are handled");
        check(first(operation("subtract",{quantity,number(1)})).toDouble()==1,"formula19 subtraction executes");
        check(first(operation("multiply",{quantity,number(3)})).toDouble()==6,"formula19 multiplication executes");
        check(first(operation("divide",{quantity,number(4)})).toDouble()==.5,"formula19 division keeps fractional result");
        check(first(operation("divide",{quantity,number(0)})).isNull(),"formula19 division by zero gives NULL");
        check(first(operation("round",{number(1.236),number(2)})).toDouble()==1.24,"formula19 round accepts decimal places");
        check(first(operation("date",{date},{{"format","%d.%m.%Y"}})).toString()=="28.10.2026","formula19 date-format dropdown syntax executes");
        check(first(operation("day",{date})).toString()=="28","formula19 extracts day");
        check(first(operation("month",{date})).toString()=="10","formula19 extracts month");
        check(first(operation("year",{date})).toString()=="2026","formula19 extracts year");
        check(query.exec("SELECT "+sql(operation("sum",{quantity}),true)+","+sql(operation("count"),true)+","+sql(operation("countdistinct",{designation}),true)+","+sql(operation("listdistinct",{designation}),true)+" FROM parts"),qPrintable(query.lastError().text()));check(query.next()&&query.value(0).toDouble()==6&&query.value(1).toInt()==3&&query.value(2).toInt()==3&&query.value(3).toString().contains("Relay"),"formula19 explicitly grouped functions execute on real rows");
        QString error;check(compile(operation("sum",{quantity}),fields,false,&error).isEmpty()&&!error.isEmpty(),"formula19 aggregation is refused outside grouped context");
        error.clear();check(compile(operation("sum",{operation("count")}),fields,true,&error).isEmpty(),"formula19 nested aggregate misuse is rejected");
        error.clear();check(compile(field("unavailable"),fields,false,&error).isEmpty()&&!error.isEmpty(),"formula19 rejects fields absent from actual source catalogue");
        error.clear();check(compile(operation("date",{date},{{"format","user SQL"}}),fields,false,&error).isEmpty(),"formula19 date format is chosen from supported formats");
        error.clear();check(compile(operation("unknown"),fields,false,&error).isEmpty(),"formula19 unsupported operations cannot generate SQL");
        check(QetTableQuery15::fieldValid("Reusable","Calculated",sql(nested),&error),"formula19 generated nested expression is accepted as reusable custom field");
        auto encoded=QJsonDocument::fromVariant(nested).toJson();check(QJsonDocument::fromJson(encoded).object().toVariantMap()==nested,"formula19 expression tree round-trips for saved formulas");
        Dialog dialog(nullptr,fields);dialog.setExpressionTree(nested);check(dialog.expression()==compile(nested,fields),"formula19 graphical tree and compiler agree");
        auto *result=dialog.findChild<QLabel*>("formula19Preview");check(result&&result->text().contains("Example result"),"formula19 example input preview is clearly labelled");
        auto *samples=dialog.findChild<QTableWidget*>("formula19Samples");samples->item(0,1)->setText("Sensor");samples->item(2,1)->setText("2");samples->item(4,1)->setText("-S7");check(result->text().contains("Sensor - -S7"),"formula19 sample preview recomputes from user inputs using SQLite");
        dialog.resize(1000,800);check(dialog.grab().save(out+"/formula-builder19.png"),"formula19 saves an actual builder preview");
        dialog.setExpressionTree(operation("eq",{quantity,text("2")}));samples->item(2,1)->setText("002");check(result->text().endsWith("0"),"formula19 sample preview preserves field text leading zeros");
        dialog.setExpressionTree(field("unavailable"));check(!dialog.findChild<QPushButton*>("formula19Accept")->isEnabled(),"formula19 invalid tree disables Use formula");
        Dialog filter(nullptr,fields,QString(),true);filter.setExpressionTree(nested);check(!filter.findChild<QPushButton*>("formula19Accept")->isEnabled(),"formula19 filter requires a top-level condition");filter.setExpressionTree(operation("gt",{quantity,number(1)}));check(filter.findChild<QPushButton*>("formula19Accept")->isEnabled(),"formula19 valid filter can be used");
        QString selected="keep this SQL";QTimer::singleShot(0,[&](){if(auto *active=dynamic_cast<Dialog*>(QApplication::activeModalWidget()))active->reject();});check(!editExpression19(nullptr,fields,selected)&&selected=="keep this SQL","formula19 Cancel leaves existing expression unchanged");
        QTimer::singleShot(0,[&](){if(auto *active=dynamic_cast<Dialog*>(QApplication::activeModalWidget())){active->setExpressionTree(nested);active->findChild<QPushButton*>("formula19Accept")->click();}});check(editExpression19(nullptr,fields,selected)&&selected==compile(nested,fields),"formula19 Use formula returns generated expression through modal UI");
        bool fullCatalog=false;QTimer::singleShot(0,[&](){auto *active=qobject_cast<QDialog*>(QApplication::activeModalWidget());if(!active)return;auto *source=active->findChild<QComboBox*>("table18ExampleSource");fullCatalog=source&&source->findData("reference")>=0&&source->count()==fields.size();active->reject();});QVariantMap cancelledField;check(!QetTableQuery15::editField(nullptr,cancelledField,"nomenclature",{},fields)&&fullCatalog,"formula19 custom field builder uses complete caller-provided catalogue");
        NodeDialog choose(nullptr,fields,field("description"),false);auto *categories=choose.findChild<QComboBox*>("formula19Category");categories->setCurrentText(QObject::tr("Text functions"));auto *operations=choose.findChild<QComboBox*>("formula19Operation");operations->setCurrentIndex(operations->findData("join"));choose.findChild<QLineEdit*>("formula19Separator")->setText(" / ");choose.findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();check(choose.node().value("op")=="join"&&choose.node().value("separator")==" / ","formula19 operation dropdown stores function parameters without SQL");
        NodeDialog grouped(nullptr,fields,field("quantity"),false);grouped.findChild<QComboBox*>("formula19Category")->setCurrentText(QObject::tr("Grouping"));auto *groupOps=grouped.findChild<QComboBox*>("formula19Operation");check(!qobject_cast<QStandardItemModel*>(groupOps->model())->item(groupOps->findData("sum"))->isEnabled(),"formula19 grouped options explain unavailability for ordinary result rows");
    }
    QSqlDatabase::removeDatabase(connection);
}
#endif
