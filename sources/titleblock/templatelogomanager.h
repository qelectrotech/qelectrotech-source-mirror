/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_LOGO_MANAGER
#define TITLEBLOCK_SLASH_TEMPLATE_LOGO_MANAGER
#include <QtGui>
class TitleBlockTemplate;

/**
	This widget allows users to manage (list, add, edit, delete) logos embedded
	within a title block template.
*/
class TitleBlockTemplateLogoManager : public QWidget {
	Q_OBJECT
	// Constructor, destructor
	public:
	TitleBlockTemplateLogoManager(TitleBlockTemplate *, QWidget * = 0);
	virtual ~TitleBlockTemplateLogoManager();
	
	// methods
	public:
	QString currentLogo() const;
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	signals:
	void logosChanged(const TitleBlockTemplate *);
	
	protected:
	void emitLogosChangedSignal();
	
	private:
	void initWidgets();
	void fillView();
	QSize iconsize() const;
	QString confirmLogoName(const QString &);
	
	private slots:
	void updateLogoInformations(QListWidgetItem *, QListWidgetItem *);
	void addLogo();
	void exportLogo();
	void removeLogo();
	void renameLogo();
	
	// attributes
	private:
	TitleBlockTemplate *managed_template_; ///< title block template which this class manages logos
	QVBoxLayout *vlayout0_, *vlayout1_;    ///< vertical layouts
	QHBoxLayout *hlayout0_, *hlayout1_;    ///< horizontal layouts
	QLabel *logos_label_;                  ///< simple displayed label
	QListWidget *logos_view_;              ///< area showing the logos
	QPushButton *add_button_;              ///< button to add a new logo
	QPushButton *export_button_;           ///< button to export an embedded logo
	QPushButton *delete_button_;           ///< button to delete an embedded logo
	QGroupBox *logo_box_;                  ///< current logo properties box
	QLabel *logo_name_label_;              ///< "name:" label
	QLineEdit *logo_name_;                 ///< current logo name
	QPushButton *rename_button_;           ///< button to rename the current logo
	QLabel *logo_type_;                    ///< current logo type
	QDialogButtonBox *buttons_;            ///< ok/cancel buttons
	QDir open_dialog_dir_;                 ///< last opened directory
	bool read_only_;                       ///< Whether this logo manager should allow logo edition (renaming, addition, deletion)
};
#endif
