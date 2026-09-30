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
#include "colordialogdoubleclick.h"

#include <QColorDialog>
#include <QEvent>
#include <QMouseEvent>
#include <QWidget>

/**
	@brief ColorDialogDoubleClick::install
	Watch the swatches of @a dialog. The filter is a child of the dialog
	and is deleted with it.
	@param dialog
*/
void ColorDialogDoubleClick::install(QColorDialog *dialog)
{
	if (!dialog) {
		return;
	}

		//The swatches are looked for when the dialog is shown: Qt builds
		//them only then when the platform offered a native dialog it
		//could not show after all.
	dialog->installEventFilter(new ColorDialogDoubleClick(dialog));
}

ColorDialogDoubleClick::ColorDialogDoubleClick(QColorDialog *dialog) :
	QObject(dialog),
	m_dialog(dialog)
{}

/**
	@brief ColorDialogDoubleClick::eventFilter
	The first click of a double-click has already selected the swatch
	(on its release), so the dialog's current colour is the one
	double-clicked: accept it. Queued, so the swatch finishes handling
	the second click before the dialog closes.
*/
bool ColorDialogDoubleClick::eventFilter(QObject *watched, QEvent *event)
{
	if (watched == m_dialog && event->type() == QEvent::Show) {
			//QWellArray is Qt's private class behind both swatch grids;
			//its name is the only handle on them (in the QtPrivate
			//namespace in recent Qt 6). If a Qt release renames it,
			//nothing matches and double-click just selects, as before.
			//Installing twice only moves the filter to the front.
		const auto children = m_dialog->findChildren<QWidget *>();
		for (QWidget *child : children) {
			if (child->inherits("QtPrivate::QWellArray")
				|| child->inherits("QWellArray")) {
				child->installEventFilter(this);
			}
		}
	}
	else if (event->type() == QEvent::MouseButtonDblClick
		&& static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton)
	{
		QMetaObject::invokeMethod(m_dialog, &QDialog::accept, Qt::QueuedConnection);
	}
	return QObject::eventFilter(watched, event);
}
