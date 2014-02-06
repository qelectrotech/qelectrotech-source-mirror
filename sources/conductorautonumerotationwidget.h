#ifndef CONDUCTORAUTONUMEROTATIONWIDGET_H
#define CONDUCTORAUTONUMEROTATIONWIDGET_H

#include <QtGui>
#include <QSet>
#include <QMultiMap>
#include <QString>
#include "qetgraphicsitem/conductor.h"
#include "diagram.h"

class ConductorAutoNumerotationWidget : public QDialog
{
	Q_OBJECT
	public:
	explicit ConductorAutoNumerotationWidget(QSet <Conductor *>, QWidget *parent = 0);
	QMultiMap <int, QString> conductorsTextToMap (QSet <Conductor *>);
	
	public slots:
	void setText (QString);
	void accept();

	signals:
	void textIsSelected (QString);

	private:
	//methods
	void buildInterface();
	QVBoxLayout* buildRadioList();

	//attributes
	QSet<Conductor *> c_list; //liste des conducteurs au même potentiel
	QLineEdit *text_field;
	QString text_;
	QSignalMapper *sm_;
	
};

#endif // CONDUCTORAUTONUMEROTATIONWIDGET_H
