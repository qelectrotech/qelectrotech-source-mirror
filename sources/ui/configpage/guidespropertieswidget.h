#ifndef GUIDESPROPERTIESWIDGET_H
#define GUIDESPROPERTIESWIDGET_H

#include <QWidget>
#include <QList>
#include "../../diagram.h"

class QTableWidget;
class QPushButton;

class GuidesPropertiesWidget : public QWidget {
	Q_OBJECT
public:
	explicit GuidesPropertiesWidget(QWidget *parent = nullptr);
	~GuidesPropertiesWidget() override;

	QList<Diagram::Guide> guides() const;
	void setGuides(const QList<Diagram::Guide> &guides);

private slots:
	void addGuide();
	void removeGuide();

private:
	void setupUi();

	QTableWidget *m_table;
	QPushButton *m_add_btn;
	QPushButton *m_remove_btn;
};

#endif // GUIDESPROPERTIESWIDGET_H
