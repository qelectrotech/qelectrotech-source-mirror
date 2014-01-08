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
	void buildRadioList();
	void buildUnlinkButton();
	void Apply();

	private slots:
	void linkToElement(const int i) {element_to_link = element_list.at(i);}
	void unlinkClicked();
	void showDiagram(const int i);

	private:
	Element *element_, *element_to_link;
	QList <Element *> element_list;
	Ui::FolioReportProperties *ui;
	QSignalMapper *sm_, *sm_show_;
	QWidget *unlink_widget;
	bool unlink;
};

#endif // FOLIOREPORTPROPERTIES_H
