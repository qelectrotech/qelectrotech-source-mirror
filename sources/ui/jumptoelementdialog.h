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
#ifndef JUMPTOELEMENTDIALOG_H
#define JUMPTOELEMENTDIALOG_H

#include <QDialog>
#include <QPointer>

class Diagram;
class Element;
class QLineEdit;
class QListWidget;

/**
	@brief The JumpToElementDialog class
	A lightweight, transient "quick open" popup: type part of an element's
	label or other information to live-filter the elements on a diagram,
	then Enter to select the chosen element on the diagram and scroll it
	into view. Up/Down move through the filtered list, Escape cancels
	without changing the current selection.
*/
class JumpToElementDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit JumpToElementDialog(Diagram *diagram, QWidget *parent = nullptr);
		~JumpToElementDialog() override;

	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	private slots:
		void updateFilteredList(const QString &filter_text);
		void activateCurrentItem();

	private:
		void buildCandidates();

		struct Candidate {
			QPointer<Element> element;
			QString display_text;
			QString search_text;
		};

		QPointer<Diagram> m_diagram;
		QList<Candidate> m_candidates;
		QLineEdit *m_filter_edit;
		QListWidget *m_result_list;
};

#endif // JUMPTOELEMENTDIALOG_H
