#ifndef DIALOGCONDUCTORAUTONUM_H
#define DIALOGCONDUCTORAUTONUM_H

#include <QDialog>

namespace Ui {
	class DialogConductorAutoNum;
}

class DialogConductorAutoNum : public QDialog
{
	Q_OBJECT
	
	public:
	explicit DialogConductorAutoNum(QWidget *parent = 0);
	~DialogConductorAutoNum();
	
	private:
	Ui::DialogConductorAutoNum *ui;
};

#endif // DIALOGCONDUCTORAUTONUM_H
