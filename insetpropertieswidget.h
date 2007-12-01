#ifndef INSET_PROPERTIES_WIDGET_H
#define INSET_PROPERTIES_WIDGET_H
#include <QtGui>
#include "insetproperties.h"
/**
	Ce widget permet d'editer un objet InsetProperties, c'est-a-dire les
	valeurs affichees par le cartouche d'un schema.
*/
class InsetPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	InsetPropertiesWidget(const InsetProperties &inset = InsetProperties(), bool = false, QWidget * = 0);
	virtual ~InsetPropertiesWidget();
	private:
	InsetPropertiesWidget(const InsetPropertiesWidget &);
	
	// methodes
	public:
	InsetProperties insetProperties() const;
	void setInsetProperties(const InsetProperties &);
	bool displayCurrentDate() const;
	
	// attributs
	private:
	QLineEdit    *inset_title;
	QLineEdit    *inset_author;
	QDateEdit    *inset_date;
	QLineEdit    *inset_filename;
	QLineEdit    *inset_folio;
	QRadioButton *inset_no_date;
	QRadioButton *inset_current_date;
	QRadioButton *inset_fixed_date;
	bool display_current_date;
};
#endif
