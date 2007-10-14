#ifndef CONDUCTOR_PROPERTIES_WIDGET_H
#define CONDUCTOR_PROPERTIES_WIDGET_H
#include "conductor.h"
#include <QtGui>
class ConductorPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ConductorPropertiesWidget(QWidget * = 0);
	virtual ~ConductorPropertiesWidget();
	
	private:
	ConductorPropertiesWidget(const ConductorPropertiesWidget  &);
	
	// methodes
	public:
	Conductor::ConductorType conductorType() const;
	void setSingleLineProperties(const SingleLineProperties &);
	SingleLineProperties singleLineProperties() const;
	QString conductorText() const;
	void setConductorText(const QString &);
	
	public slots:
	void updatePreview();
	void updateSingleLineConfig();
	void updateSingleLineDisplay();
	void setConductorType(Conductor::ConductorType);
	void setConductorType(int);
	
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
	
	Conductor::ConductorType type_;
	SingleLineProperties slp;
	QString conductor_text;
	
	// methodes privees
	void buildInterface();
	void buildConnections();
	void destroyConnections();
};
#endif
