/*
	Copyright 2006-2008 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef CONDUCTOR_PROPERTIES_WIDGET_H
#define CONDUCTOR_PROPERTIES_WIDGET_H
#include "conductor.h"
#include <QtGui>
/**
	Ce widget permet a l utilisateur d'editer les proprietes d'un conducteur.
	Par proprietes, on entend non pas le trajet effectue par le conducteur mais
	les options supplementaires : symboles unifilaires, presence ou non d'un
	champ de texte, contenu de ce champ de texte, etc.
*/
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
