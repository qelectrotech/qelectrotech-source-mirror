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
#ifndef DEVICETYPECHOOSER_H
#define DEVICETYPECHOOSER_H

#include <QDialog>
#include <QPointer>
#include <QStringList>

class Element;
class QCheckBox;
class QETProject;
class QLineEdit;
class QListWidget;
class QUndoCommand;

/**
	@brief The DeviceTypeChooser class
	Which device type is this symbol's device? The types of the project
	and of the user's library whose head can be drawn with the symbol,
	the ones chosen lately first; a filter, and all the types on request.
	Asked when a master symbol is placed in a project following the
	device types (askFor()), and when a device changes its type.
*/
class DeviceTypeChooser : public QDialog
{
		Q_OBJECT

	public:
		DeviceTypeChooser(Element *symbol, QWidget *parent = nullptr,
						  const QString &current = QString());

		QString chosen() const;
		QStringList shown() const;
		void setFilter(const QString &text);
		void setShowAll(bool all);

		static QStringList candidates(Element *symbol, bool all);
			/// The undo command giving @p symbol the type @p name (copying a
			/// type of the library into the project); nullptr if nothing changes
		static QUndoCommand *give(Element *symbol, const QString &name);
			/// Ask @p symbol's device type when its project follows the
			/// device types and it is a master; push the answer
		static void askFor(Element *symbol, QWidget *parent = nullptr);
		static void remember(const QString &name);

	private:
		void fill();

		QPointer<Element> m_symbol;
		QString m_current;
		QLineEdit *m_filter = nullptr;
		QCheckBox *m_all = nullptr;
		QListWidget *m_list = nullptr;
};

#endif // DEVICETYPECHOOSER_H
