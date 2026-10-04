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
#ifndef RENUMBERPREVIEWDIALOG_H
#define RENUMBERPREVIEWDIALOG_H

#include "../renumberelementscommand.h"

#include <QCoreApplication>
#include <QVector>

class Element;
class QWidget;

/**
	@brief The RenumberPreviewDialog class
	Shows what a numbering operation is about to do to the labels of
	elements -- each changed element with its label now and the one it
	would get -- and the elements with a frozen label it leaves as they are,
	and asks to go on or not. Nothing is changed by the dialog itself.
*/
class RenumberPreviewDialog
{
	Q_DECLARE_TR_FUNCTIONS(RenumberPreviewDialog)

	public:
			/// An element an operation leaves as it is, and why
		struct LeftAlone
		{
			Element *element = nullptr;
			QString note;
		};
		enum class Answer { Cancel, Go, GoAndReplace };

		static Answer ask(
				QWidget *parent,
				const QString &title,
				const QString &intro,
				const QVector<RenumberElementsCommand::ElementChange> &changes,
				const QVector<LeftAlone> &left_alone,
				const QString &replace_text = QString());

		static bool confirm(
				QWidget *parent,
				const QString &title,
				const QString &intro,
				const QVector<RenumberElementsCommand::ElementChange> &changes,
				const QVector<Element *> &frozen);

		static int changedLabelCount(
				const QVector<RenumberElementsCommand::ElementChange> &changes);
};

#endif // RENUMBERPREVIEWDIALOG_H
