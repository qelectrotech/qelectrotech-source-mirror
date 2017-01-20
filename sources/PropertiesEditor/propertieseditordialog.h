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
#ifndef PROPERTIESEDITORDIALOG_H
#define PROPERTIESEDITORDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QAbstractButton>

/**
 * @brief The PropertiesEditorDialog class
 * Create a dialog to edit some properties of a thing.
 * Only create a instance of this class and call exec, all is done for you in this class.
 * The first argument (a template) must be a subclass of QWidget and provide the 3 methods bellow :
 * QString::title()
 * void::apply()
 * void::reset()
 * You can subclass the interface PropertiesEditorWidget who provide all this methods.
 * This dialog take ownership of the editor, so the editor will be deleted by this dialog
 */
class PropertiesEditorDialog : public QDialog
{
		Q_OBJECT
	public:
		template<typename T>
		PropertiesEditorDialog(T editor, QWidget *parent = 0) :
		QDialog (parent)
		{
				//Set dialog title
			setWindowTitle(editor->title());
				//Reparent the editor, to be deleted at the same time of this dialog
			editor->setParent(this);

				//Build the dialog
			QVBoxLayout *vlayout = new QVBoxLayout(this);
			vlayout->addWidget(editor);
			QDialogButtonBox *button_box = new QDialogButtonBox (QDialogButtonBox::Apply | QDialogButtonBox::Cancel | QDialogButtonBox::Reset, this);
			vlayout->addWidget(button_box);

				//Setup connection between button box and the editor
			connect(button_box, &QDialogButtonBox::clicked, [editor, button_box, this](QAbstractButton *button)
			{
				switch(button_box->buttonRole(button))
				{
					case QDialogButtonBox::RejectRole:
						editor->reset();
						this->reject();
						break;
					case QDialogButtonBox::ResetRole:
						editor->reset();
						break;
					case QDialogButtonBox::ApplyRole:
						editor->apply();
						this->accept();
						break;
					default:
						editor->reset();
						this->reject();
				}
			});
		}
};

#endif // PROPERTIESEDITORDIALOG_H
