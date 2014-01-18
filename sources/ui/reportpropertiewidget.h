#ifndef REPORTPROPERTIEWIDGET_H
#define REPORTPROPERTIEWIDGET_H

#include <QWidget>
#include <QSettings>


namespace Ui {
	class ReportPropertieWidget;
}

class ReportPropertieWidget : public QWidget
{
	Q_OBJECT

	public:
	explicit ReportPropertieWidget(QString value, QWidget *parent = 0);
	~ReportPropertieWidget();
	void toSettings (QSettings &settings, const QString &prefix);
	void setReportProperties (QString label);
	QString ReportProperties () const;

	private:
	Ui::ReportPropertieWidget *ui;
};

#endif // REPORTPROPERTIEWIDGET_H
