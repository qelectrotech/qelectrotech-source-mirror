#ifndef CONDUCTOR_PROPERTIES_WIDGET_H
#define CONDUCTOR_PROPERTIES_WIDGET_H
#include "conductor.h"
#include <QtGui>
class ConductorPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ConductorPropertiesWidget(QWidget * = 0);
	ConductorPropertiesWidget(const ConductorProperties &, QWidget * = 0);
	virtual ~ConductorPropertiesWidget();
	
	private:
	ConductorPropertiesWidget(const ConductorPropertiesWidget  &);
	
	// methodes
	public:
	void setConductorProperties(const ConductorProperties &);
	ConductorProperties conductorProperties() const;
	
	private:
	void setConductorType(ConductorProperties::ConductorType);
	
	public slots:
	void updatePreview();
	void updateConfig();
	void updateDisplay();
	
	// attributs prives
	private:
	QButtonGroup *radio_buttons;
	QRadioButton *simple;
	QRadioButton *multiline;
	QLineEdit *text_field;
	QRadioButton *singleline;
	QCheckBox *phase_checkbox;
	QSlider *phase_slider;
	QSpinBox *phase_spinbox;
	QCheckBox *ground_checkbox;
	QCheckBox *neutral_checkbox;
	QLabel *preview;
	
	ConductorProperties properties_;
	
	// methodes privees
	void buildInterface();
	void buildConnections();
	void destroyConnections();
};
#endif
