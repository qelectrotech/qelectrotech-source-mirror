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

	private:
	Element *element_;
	Ui::FolioReportProperties *ui;
};

#endif // FOLIOREPORTPROPERTIES_H
