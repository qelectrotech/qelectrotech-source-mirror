#include "reportpropertiewidget.h"
#include "ui_reportpropertiewidget.h"

ReportPropertieWidget::ReportPropertieWidget(QString value, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ReportPropertieWidget)
{
	ui->setupUi(this);
	ui->line_edit->setText(value);
}

ReportPropertieWidget::~ReportPropertieWidget()
{
	delete ui;
}

void ReportPropertieWidget::toSettings(QSettings &settings, const QString &prefix) {
	settings.setValue(prefix + "label", ui->line_edit->text());
}

void ReportPropertieWidget::setReportProperties(QString label) {
	ui->line_edit->setText(label);
}

QString ReportPropertieWidget::ReportProperties() const {
	return ui->line_edit->text();
}
