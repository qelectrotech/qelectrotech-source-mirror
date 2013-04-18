#ifndef DIALOGCONDUCTORAUTONUM_H
#define DIALOGCONDUCTORAUTONUM_H

#include <QDialog>

#include "diagram.h"

namespace Ui {
	class DialogConductorAutoNum;
}

class DialogConductorAutoNum : public QDialog
{
	Q_OBJECT
	
	public:
	explicit DialogConductorAutoNum(Diagram *dg, QWidget *parent = 0);
	~DialogConductorAutoNum();
	
	private slots:
	void on_pushButton_delete_clicked();
	
	void on_pushButton_close_clicked();
	
	private:
	Ui::DialogConductorAutoNum *ui;
	Diagram *dg_;
};

#endif // DIALOGCONDUCTORAUTONUM_H
