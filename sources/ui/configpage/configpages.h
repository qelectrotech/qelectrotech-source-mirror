/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "configpage.h"
#include "../projectpropertiesdialog.h"
#include "../titleblockpropertieswidget.h"
#include "../autoNum/numerotationcontext.h"

#include <QDialog>
#include <QtWidgets>
class BorderPropertiesWidget;
class ConductorPropertiesWidget;
class TitleBlockPropertiesWidget;
class ExportPropertiesWidget;
class ReportPropertieWidget;
class XRefPropertiesWidget;
class GuidesPropertiesWidget;
class QETProject;
class TitleBlockProperties;
class SelectAutonumW;

/**
	@brief The NewDiagramPage class
	This configuration page enables users to define the properties of new
	diagrams to come.
*/
class NewDiagramPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	NewDiagramPage(QETProject *project = nullptr,
		       QWidget * = nullptr,
		       ProjectPropertiesDialog *teste = nullptr);
	~NewDiagramPage() override;
	private:
	NewDiagramPage(const NewDiagramPage &);
	bool eventFilter(QObject *obj, QEvent *event) override;
public slots:
	void changeToAutoFolioTab();
	void setFolioAutonum(QString);
	void saveCurrentTbp();
	void loadSavedTbp();

	
	// methods
	public:
	void applyConf() override;
	QString title() const override;
	QIcon icon() const override;
	
	// attributes
	private:
	ProjectPropertiesDialog    *ppd_;
	QETProject                 *m_project; ///< Project to edit properties
	BorderPropertiesWidget     *bpw;       ///< Widget to edit default diagram dimensions
	TitleBlockPropertiesWidget *ipw;       ///< Widget to edit default title block properties
	ConductorPropertiesWidget  *m_cpw;     ///< Widget to edit default conductor properties
	ReportPropertieWidget      *rpw;       ///< Widget to edit default report label
	XRefPropertiesWidget       *xrefpw;    ///< Widget to edit default xref properties
	GuidesPropertiesWidget     *m_gpw;     ///< Widget to edit guides
	TitleBlockProperties       savedTbp;   ///< Used to save current TBP and retrieve later
	QTabWidget                *m_tab_widget;     ///< Main tab widget (stored for later access)

	// auto-numbering tab data
	struct AutoNumTab {
		SelectAutonumW            *widget = nullptr;
		QHash<QString, NumerotationContext> contexts;
		QString                    prefix;
	};
	AutoNumTab m_autonum_conductor;
	AutoNumTab m_autonum_element;
	AutoNumTab m_autonum_folio;

	void initAutoNumTab(AutoNumTab &tab, SelectAutonumW *w, const QString &prefix);
	void loadAutoNumTab(AutoNumTab &tab, QSettings &settings);
	void saveAutoNumContext(AutoNumTab &tab);
	void removeAutoNumContext(AutoNumTab &tab);
	void persistAutonumSettings();

	static bool isPlaceholder(QComboBox *combo, const QString &name);
};

/**
	@brief The ExportConfigPage class
	This configuration page enables users to set default export options.
*/
class ExportConfigPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	ExportConfigPage(QWidget * = nullptr);
	~ExportConfigPage() override;
	private:
	ExportConfigPage(const ExportConfigPage &);
	
	// methods
	public:
	void applyConf() override;
	QString title() const override;
	QIcon icon() const override;
	
	// attributes
	public:
	ExportPropertiesWidget *epw;
};

/**
	@brief The PrintConfigPage class
	This configuration page enables users to set default printing options.
*/
class PrintConfigPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	PrintConfigPage(QWidget * = nullptr);
	~PrintConfigPage() override;
	private:
	PrintConfigPage(const PrintConfigPage &);
	
	// methods
	public:
	void applyConf() override;
	QString title() const override;
	QIcon icon() const override;
	
	// attributes
	public:
	ExportPropertiesWidget *epw;
};
#endif
