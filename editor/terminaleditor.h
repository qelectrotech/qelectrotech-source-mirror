#ifndef TERMINAL_EDITOR_H
#define TERMINAL_EDITOR_H
#include <QtGui>
#include "elementitemeditor.h"
class PartTerminal;
/**
	Cette classe represente un editeur de borne.
	Elle permet d'editer a travers une interface graphique les
	proprietes d'une borne d'element.
*/
class TerminalEditor : public ElementItemEditor {
	Q_OBJECT
	// Constructeurs, destructeur
	public:
	TerminalEditor(QETElementEditor *, PartTerminal *, QWidget * = 0);
	virtual ~TerminalEditor();
	private:
	TerminalEditor(const TerminalEditor &);
	
	// attributs
	private:
	PartTerminal *part;
	QLineEdit *qle_x, *qle_y;
	QComboBox *orientation;
	
	// methodes
	public slots:
	void updateTerminal();
	void updateTerminalX();
	void updateTerminalY();
	void updateTerminalO();
	void updateForm();
	
	private:
	void activeConnections(bool);
};
#endif
