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
#ifndef MATERIALENTRYDIALOG_H
#define MATERIALENTRYDIALOG_H

#include "../materiallist/materiallist.h"

#include <QDialog>
#include <QList>

class QLineEdit;

/**
	@brief The form used to add an article to the material file.

	One field per column of the file, including the columns QElectroTech
	does not know : whatever the user typed there is written back to the
	file untouched when a later row is appended.
*/
class MaterialEntryDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit MaterialEntryDialog(const QStringList &columns, QWidget *parent = nullptr);
		~MaterialEntryDialog() override;

		/** @brief the article described by the form */
		MaterialRecord record() const;

		void accept() override;

		//ATTRIBUTES
	private:
		QStringList      m_columns;
		QList<QLineEdit*> m_edits;
};

#endif // MATERIALENTRYDIALOG_H
