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
#ifndef COLOR_DIALOG_DOUBLE_CLICK_H
#define COLOR_DIALOG_DOUBLE_CLICK_H

#include <QObject>

class QColorDialog;

/**
	@brief The ColorDialogDoubleClick class
	Lets a double-click on one of a QColorDialog's colour swatches (the
	basic colours and the custom colours) choose that colour and close
	the dialog, as OK does. Qt's dialog only selects the swatch.

	Only Qt's own dialog has swatches to watch. Where the platform shows
	its native dialog instead (macOS, some Linux desktops), this does
	nothing and the native dialog behaves as it always has.
*/
class ColorDialogDoubleClick : public QObject
{
	Q_OBJECT

	public:
		static void install(QColorDialog *dialog);

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private:
		explicit ColorDialogDoubleClick(QColorDialog *dialog);
		QColorDialog *m_dialog;
};

#endif // COLOR_DIALOG_DOUBLE_CLICK_H
