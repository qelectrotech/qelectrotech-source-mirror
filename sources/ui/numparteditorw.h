#ifndef NUMPARTEDITORW_H
#define NUMPARTEDITORW_H

#include <QWidget>
#include <QValidator>
#include "numerotationcontext.h"

/**
 *This class represent a single part num widget. By this widget, we can define and edit
 *how the num auto must work .
 *This widget is called by selectautonumw.
 */
namespace Ui {
	class NumPartEditorW;
}

class NumPartEditorW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
	explicit NumPartEditorW(QWidget *parent = 0);
	NumPartEditorW (NumerotationContext &, int, QWidget *parent=0);
	~NumPartEditorW();

	enum type {unit,ten,hundred,string,folio};
	NumerotationContext toNumContext();
	bool isValid ();


	private slots:
	void on_type_combo_activated(int);
	void on_value_field_textEdited();
	void on_increase_spinBox_valueChanged();
	void setType (NumPartEditorW::type t, bool=false);

	signals:
	void changed ();
	
	private:
	Ui::NumPartEditorW *ui;
	QValidator *intValidator;
	type type_;

};

#endif // NUMPARTEDITORW_H
