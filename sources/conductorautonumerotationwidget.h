#ifndef CONDUCTORAUTONUMEROTATIONWIDGET_H
#define CONDUCTORAUTONUMEROTATIONWIDGET_H

#include <QtGui>
#include <QList>
#include <QSet>
#include <QMultiMap>
#include <QString>
#include "conductor.h"

class ConductorAutoNumerotationWidget : public QDialog
{
	Q_OBJECT
	public:
	explicit ConductorAutoNumerotationWidget(Conductor *, QSet <Conductor *>, QWidget *parent = 0);
	QMultiMap <int, QString> conductorsTextToMap (QSet <Conductor *>);
	
	public slots:
	void setText (QString);
	void accept();

	private:
	//methods
	void buildInterface();
	QVBoxLayout* buildRadioList();
	void applyText();

	//attributes
	Conductor *conductor_;
	QSet<Conductor *> c_list; //liste des conducteurs au même potentiel
	QList <QRadioButton *> *radio_List;
	QLineEdit *text_field;
	QString text_;
	QSignalMapper *sm_;
	
};

#endif // CONDUCTORAUTONUMEROTATIONWIDGET_H
