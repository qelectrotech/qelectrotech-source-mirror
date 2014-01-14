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
	void Apply();

	private slots:
	void linkToElement(const int i) {element_to_link = element_list.at(i);}
	void unlinkClicked();
	void showElement(Element *elmt);
	void showElementFromList (const int i);
	void on_button_this_clicked();
	void on_button_linked_clicked();

	private:
	void buildRadioList();
	void buildUnlinkButton();

	Element *element_, *element_to_link;
	QList <Element *> element_list;
	Ui::FolioReportProperties *ui;
	QSignalMapper *sm_, *sm_show_;
	QWidget *unlink_widget;
	bool unlink;
};

#endif // FOLIOREPORTPROPERTIES_H
