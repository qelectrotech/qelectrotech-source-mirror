/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef CONFIG_PAGES_H
#define CONFIG_PAGES_H
#include <QtGui>
class BorderPropertiesWidget;
class ConductorPropertiesWidget;
class TitleBlockPropertiesWidget;
class ExportPropertiesWidget;
/**
	Cette classe abstraite contient les methodes que toutes les pages de
	configuration doivent implementer.
*/
class ConfigPage : public QWidget {
	Q_OBJECT
	public:
	/**
		Constructeur
		@param parent QWidget parent
	*/
	ConfigPage(QWidget *parent) : QWidget(parent) {};
	/// Destructeur
	virtual ~ConfigPage() {};
	/// Applique la configuration saisie par l'utilisateur dans la page de configuration
	virtual void applyConf() = 0;
	/// @return le titre de la page de configuration
	virtual QString title() const = 0;
	/// @return l'icone de la page de configuration
	virtual QIcon icon() const = 0;
};

/**
	Cette classe represente la page de configuration des nouveaux schemas.
*/
class NewDiagramPage : public ConfigPage {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	NewDiagramPage(QWidget * = 0);
	virtual ~NewDiagramPage();
	private:
	NewDiagramPage(const NewDiagramPage &);
	
	// methodes
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributs
	public:
	BorderPropertiesWidget *bpw;    ///< Widget d'edition des dimensions du schema
	TitleBlockPropertiesWidget *ipw;     ///< Widget d'edition des proprietes par defaut du cartouche
	ConductorPropertiesWidget *cpw; ///< Widget d'edition des proprietes par defaut des conducteurs
};

/**
	Cette classe represente la page de configuration generale.
*/
class GeneralConfigurationPage : public ConfigPage {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	GeneralConfigurationPage(QWidget * = 0);
	virtual ~GeneralConfigurationPage();
	private:
	GeneralConfigurationPage(const GeneralConfigurationPage &);
	
	// methodes
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributs
	public:
	QLabel *title_label_;
	QFrame *horiz_line_;
	QGroupBox *appearance_;
	QCheckBox *use_system_colors_;
	QGroupBox *projects_view_mode_;
	QRadioButton *windowed_mode_;
	QRadioButton *tabbed_mode_;
	QLabel *warning_view_mode_;
	QGroupBox *elements_management_;
	QCheckBox *integrate_elements_;
	QCheckBox *highlight_integrated_elements_;
	QLabel    *default_element_infos_label_;
	QTextEdit *default_element_infos_textfield_;
};

/**
	Cette classe represente la page de configuration du dialogue d'exportation
*/
class ExportConfigPage : public ConfigPage {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ExportConfigPage(QWidget * = 0);
	virtual ~ExportConfigPage();
	private:
	ExportConfigPage(const ExportConfigPage &);
	
	// methodes
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributs
	public:
	ExportPropertiesWidget *epw;
};

/**
	Cette classe represente la page de configuration du dialogue d'impression
*/
class PrintConfigPage : public ConfigPage {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	PrintConfigPage(QWidget * = 0);
	virtual ~PrintConfigPage();
	private:
	PrintConfigPage(const PrintConfigPage &);
	
	// methodes
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributs
	public:
	ExportPropertiesWidget *epw;
};
#endif
