#ifndef ELEMENTPROPERTIESWIDGET_H
#define ELEMENTPROPERTIESWIDGET_H

#include <QtGui>
#include <qetgraphicsitem/element.h>
#include <diagram.h>

class elementpropertieswidget : public QDialog
{
	Q_OBJECT
	public:
	explicit elementpropertieswidget(Element *elmt, QWidget *parent = 0);

	private:
	QWidget* generalWidget();

	signals:
	/// Signal emitted when users wish to locate an element from the diagram within elements collection
	void findElementRequired(const ElementsLocation &);
	/// Signal emitted when users wish to edit an element from the diagram
	void editElementRequired(const ElementsLocation &);

	public slots:
	void standardButtonClicked (QAbstractButton *);
	void findInPanel ();
	void editElement ();

	private:
	QDialogButtonBox *dbb;
	Element *element_;
	Diagram *diagram_;
	QTabWidget *tab_;
	QPushButton *find_in_panel, *edit_element;

};

#endif // ELEMENTPROPERTIESWIDGET_H
