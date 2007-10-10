#ifndef ORIENTATION_SET_WIDGET_H
#define ORIENTATION_SET_WIDGET_H
#include <QtGui>
#include "orientationset.h"
/**
	Widget permettant d'editer un OrientationSet
*/
class OrientationSetWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	OrientationSetWidget(QWidget * = 0);
	virtual ~OrientationSetWidget() {};
	
	private:
	OrientationSetWidget(const OrientationSetWidget &);
	
	// attributs
	private:
	OrientationSet ori;
	QComboBox *north_orientation;
	QComboBox *east_orientation;
	QComboBox *south_orientation;
	QComboBox *west_orientation;
	QRadioButton *north_default;
	QRadioButton *east_default;
	QRadioButton *south_default;
	QRadioButton *west_default;
	QButtonGroup *default_radios;
	
	// methodes
	public:
	OrientationSet orientationSet() const;
	void setOrientationSet(const OrientationSet &);
	
	private:
	void updateForm();
	
	public slots:
	void slot_defaultChanged(QAbstractButton *);
	void updateOrientationSet();
};
#endif
