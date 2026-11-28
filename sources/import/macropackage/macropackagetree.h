/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef MACROPACKAGETREE_H
#define MACROPACKAGETREE_H

#include "macropackage.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

/**
	@brief The macro package of a manufacturer as a tree in the elements
	panel: manufacturer > product family > type, read from the package's
	list of articles alone. Nothing is converted beforehand: a type is
	shown grey until it is asked for (double click); only then its macro
	becomes elements in the company collection, in manufacturer / family /
	type folders, and the type is an ordinary entry of the collection from
	then on.

	The widget has a model of its own, so the collection model does not
	have to know entries that are not files yet.
*/
class MacroPackageTree : public QWidget
{
	Q_OBJECT

	public:
		explicit MacroPackageTree(QWidget *parent = nullptr);

	signals:
		/// the type lies in @p folder of the company collection;
		/// @p fresh: it was converted just now
		void typeLoaded(const QString &folder, bool fresh);

	protected:
		void showEvent(QShowEvent *event) override;

	private:
		void fill();
		void filter(const QString &text);
		void activate(QTreeWidgetItem *item);
		void mark(QTreeWidgetItem *item, bool loaded);
		QString folderOf(const MacroPackageArticle &article) const;

		MacroPackage m_package;
		QString m_path;                 ///< the package shown
		QLabel *m_hint = nullptr;
		QLineEdit *m_filter = nullptr;
		QTreeWidget *m_tree = nullptr;
		bool m_terms_accepted = false;
};

#endif // MACROPACKAGETREE_H
