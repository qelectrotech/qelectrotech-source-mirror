#ifndef SELECTAUTONUMW_H
#define SELECTAUTONUMW_H

#include <QWidget>
#include "diagram.h"
#include "ui/numparteditorw.h"

namespace Ui {
	class SelectAutonumW;
}

class SelectAutonumW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
	explicit SelectAutonumW(const QList <Diagram *> &, Diagram * = 0, QWidget *parent = 0);
	~SelectAutonumW();
	
	private:
	void initDiagramChooser();
	void setCurrentContext ();
	NumerotationContext toNumContext() const;

	//SLOT
	private slots:
	void on_add_button_clicked();
	void on_remove_button_clicked();
	void on_diagram_chooser_activated();
	void on_buttonBox_clicked(QAbstractButton *);
	void applyEnable (bool = true);

	//ATTRIBUTS
	private:
	Ui::SelectAutonumW *ui;
	const QList <Diagram *> diagram_list;
	QList <NumPartEditorW *> num_part_list_;
};

#endif // SELECTAUTONUMW_H
