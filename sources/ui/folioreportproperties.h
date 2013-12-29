#ifndef FOLIOREPORTPROPERTIES_H
#define FOLIOREPORTPROPERTIES_H

#include <QWidget>
#include <qetgraphicsitem/element.h>

namespace Ui {
	class FolioReportProperties;
}

class FolioReportProperties : public QWidget
{
	Q_OBJECT

	public:
	explicit FolioReportProperties(Element *elmt, QWidget *parent = 0);
	~FolioReportProperties();
	void BuildRadioList();
	void Apply();

	private slots:
	void linkToElement(const int i) {element_to_link = element_list.at(i);}

	private:
	Element *element_, *element_to_link;
	QList <Element *> element_list;
	Ui::FolioReportProperties *ui;
	QSignalMapper *sm_;


};

#endif // FOLIOREPORTPROPERTIES_H
