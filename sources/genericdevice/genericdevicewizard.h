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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef GENERICDEVICEWIZARD_H
#define GENERICDEVICEWIZARD_H

#include "genericdevice.h"

#include <QWizard>

class GenericDevicePreview;

/**
	@brief The GenericDeviceWizard class
	Three pages, layout, terminals and information, that fill one
	GenericDevice::Spec, with a preview of the box beside every page.
	Finish is offered from the first page: a box with numbered pins needs
	nothing more.
*/
class GenericDeviceWizard : public QWizard
{
	Q_OBJECT

	public:
		enum { LayoutPageId, TerminalsPageId, InformationPageId };

		GenericDeviceWizard(const GenericDevice::Fonts &fonts,
				    QWidget *parent = nullptr);

		GenericDevice::Spec spec() const;
		GenericDevice::Spec &editableSpec() { return m_spec; }
		const GenericDevice::Fonts &fonts() const { return m_fonts; }

			/// Redraw the preview and tell the pages the spec changed.
		void specChanged();

	signals:
		void specUpdated();

	private:
		GenericDevice::Spec m_spec;
		GenericDevice::Fonts m_fonts;
		GenericDevicePreview *m_preview = nullptr;
};

#endif // GENERICDEVICEWIZARD_H
