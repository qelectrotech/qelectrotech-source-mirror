/*
	Copyright 2006-2019 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ABOUTQETDIALOG_H
#define ABOUTQETDIALOG_H

#include <QDialog>

class QLabel;

namespace Ui {
	class AboutQETDialog;
}

class AboutQETDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit AboutQETDialog(QWidget *parent = nullptr);
		~AboutQETDialog();

	private:
		void setAbout();
		void setAuthors();
		void setTranslators();
		void setContributors();
		void setVersion();
		void setLibraries();
		void setLicence();
		void addAuthor(QLabel *label, const QString &name, const QString &email, const QString &work);
		void addLibrary(QLabel *label, const QString &name, const QString &link);

	private:
		Ui::AboutQETDialog *ui;
};

#endif // ABOUTQETDIALOG_H
