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
#ifndef SCALEELEMENTDIALOG_H
#define SCALEELEMENTDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;

/**
	@brief The ScaleElementDialog class
	Asks for the factor to scale the whole element by. Only the factors
	that leave every terminal on the folio grid are offered
	(see SymbolScale::safeFactors()).
*/
class ScaleElementDialog : public QDialog
{
	Q_OBJECT

	public:
		ScaleElementDialog(const QList<QPointF> &terminals, QWidget *parent = nullptr);

		qreal factor() const;
		bool scaleText() const;

	private:
		QComboBox *m_factor = nullptr;
		QCheckBox *m_scale_text = nullptr;
};

#endif // SCALEELEMENTDIALOG_H
