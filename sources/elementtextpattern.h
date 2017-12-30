/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef ELEMENTTEXTPATTERN_H
#define ELEMENTTEXTPATTERN_H

#include <QString>
#include <QDomDocument>

class Element;
class QWidget;

class ExportElementTextPattern
{
	public:
		ExportElementTextPattern(Element *elmt);
		
	private:
		QString getName(bool &ok) const;
		QWidget *parentWidget() const;
		QDomDocument xmlConf() const;
		
	private:
		Element *m_element = nullptr;
		QString m_name;
};

class ImportElementTextPattern
{
	public:
		ImportElementTextPattern(Element *elmt);
		
	private:
		QString getName(QStringList list, bool &ok) const;
		QWidget *parentWidget() const;
		void apply(QString name) const;
		
	private:
		Element *m_element = nullptr;
};

#endif // ELEMENTTEXTCONFIGURATION_H
