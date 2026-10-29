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
#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H
#include <QDialog>
#include <QIcon>
#include <functional>
#define QET_LAZY_CONFIG23 1
class ConfigPage;
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QDialogButtonBox;
/**
	This class represents the configuration dialog for QElectroTech.
	It displays "configuration pages",
	each page having to provide an icon and a title.
*/
class ConfigDialog : public QDialog {
	Q_OBJECT
	// constructors, destructor
	public:
		ConfigDialog(QWidget * = nullptr);
		~ConfigDialog() override;
	private:
		ConfigDialog(const ConfigDialog &);

	public:
		QList<ConfigPage *> pages;

	
	// methods
	public slots:
		void applyConf();
        void addLazyPage23(const QString &, const QIcon &, std::function<ConfigPage *()>);
        void addApplicationPages23();
		void addPage(ConfigPage *);
		void setCurrentPage(const int index);
	
	private:
		void showEvent(QShowEvent *) override;
        void showPage23(int);
		void addPageToList(ConfigPage *);
	
	// attributes
	private:
		QList<std::function<ConfigPage *()>> m_factories23;
        QListWidget *pages_list;
		QStackedWidget *pages_widget;
		QDialogButtonBox *buttons;



};
#endif
