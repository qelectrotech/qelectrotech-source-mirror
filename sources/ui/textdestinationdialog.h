// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef TEXTDESTINATIONDIALOG_H
#define TEXTDESTINATIONDIALOG_H

#include <QDialog>
#include <QUuid>

class QComboBox;
class QETProject;

/**
	@brief The TextDestinationDialog class
	Asks which folio a reference text (such as /12.3) leads to. Navigation
	only: Ctrl-click on the text shows the folio, and the PDF export makes
	the text a link.
*/
class TextDestinationDialog : public QDialog
{
	Q_OBJECT

	public:
		TextDestinationDialog(QETProject *project, const QUuid &current, QWidget *parent = nullptr);

		QUuid destination() const;

	private:
		QComboBox *m_folios;
		bool m_remove = false;
};

#endif // TEXTDESTINATIONDIALOG_H
