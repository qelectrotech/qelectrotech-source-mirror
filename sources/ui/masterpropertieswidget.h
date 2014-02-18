#ifndef MASTERPROPERTIESWIDGET_H
#define MASTERPROPERTIESWIDGET_H

#include <QWidget>

namespace Ui {
	class MasterPropertiesWidget;
}

class MasterPropertiesWidget : public QWidget
{
	Q_OBJECT

	public:
	explicit MasterPropertiesWidget(QWidget *parent = 0);
	~MasterPropertiesWidget();

	private:
	Ui::MasterPropertiesWidget *ui;
};

#endif // MASTERPROPERTIESWIDGET_H
