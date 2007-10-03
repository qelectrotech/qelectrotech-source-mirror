#ifndef CONDUCER_PROPERTIES_WIDGET_H
#define CONDUCER_PROPERTIES_WIDGET_H
#include "conducer.h"
#include <QtGui>
class ConducerPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ConducerPropertiesWidget(QWidget * = 0);
	virtual ~ConducerPropertiesWidget();
	
	private:
	ConducerPropertiesWidget(const ConducerPropertiesWidget  &);
	
	// methodes
	public:
	bool isSingleLine() const;
	void setSingleLineProperties(const SingleLineProperties &);
	SingleLineProperties singleLineProperties() const;
	QString conducerText() const;
	void setConducerText(const QString &);
	
	public slots:
	void updatePreview();
	void updateSingleLineConfig();
	void updateSingleLineDisplay();
	void setSingleLine(bool);
	
	// attributs prives
	private:
	QRadioButton *multiline;
	QLineEdit *text_field;
	QRadioButton *singleline;
	QCheckBox *phase_checkbox;
	QSlider *phase_slider;
	QSpinBox *phase_spinbox;
	QCheckBox *ground_checkbox;
	QCheckBox *neutral_checkbox;
	QLabel *preview;
	
	SingleLineProperties slp;
	QString conducer_text;
	
	// methodes privees
	void buildInterface();
	void buildConnections();
	void destroyConnections();
};
#endif
