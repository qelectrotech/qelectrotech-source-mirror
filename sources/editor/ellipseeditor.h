/*
	Copyright 2006-2019 The QElectroTech Team
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
#ifndef ELLIPSE_EDITOR_H
#define ELLIPSE_EDITOR_H

#include "elementitemeditor.h"

class PartEllipse;
class StyleEditor;
class QDoubleSpinBox;

/**
	This class provides a widget to edit ellipses within the element editor.
*/
class EllipseEditor : public ElementItemEditor
{
	Q_OBJECT

		// constructors, destructor
	public:
		EllipseEditor(QETElementEditor *, PartEllipse * = nullptr, QWidget * = nullptr);
		~EllipseEditor() override;
	private:
		EllipseEditor(const EllipseEditor &);
	
		// attributes
	private:
		PartEllipse *part;
		StyleEditor *style_;
		QDoubleSpinBox *x, *y, *h, *v;
		bool m_locked;
	
		// methods
	public:
		bool setPart(CustomElementPart *) override;
		CustomElementPart *currentPart() const override;
        QList<CustomElementPart*> currentParts() const override;
	
	public slots:
		void editingFinished();
		void updateForm() override;
	
	private:
		void activeConnections(bool);
};
#endif
