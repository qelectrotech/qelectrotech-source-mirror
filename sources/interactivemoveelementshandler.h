/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef INTERACTIVE_MOVE_ELEMENTS_HANDLER_H
#define INTERACTIVE_MOVE_ELEMENTS_HANDLER_H
#include "basicmoveelementshandler.h"
class QDialog;
class QDialogButtonBox;
class QAbstractButton;
class QPushButton;
class QFileNameEdit;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
/**
	This class implements the MoveElementsHandler Strategy class by asking
	users how to handle the various expected problems through interactive
	dialogs.
*/
class InteractiveMoveElementsHandler : public BasicMoveElementsHandler {
	Q_OBJECT
	
	// constructors, destructor
	public:
	InteractiveMoveElementsHandler(QWidget * = 0);
	virtual ~InteractiveMoveElementsHandler();
	private:
	InteractiveMoveElementsHandler(const InteractiveMoveElementsHandler &);
	
	// methods
	public:
	virtual QET::Action categoryAlreadyExists(ElementsCategory *, ElementsCategory  *);
	virtual QET::Action elementAlreadyExists(ElementDefinition *, ElementDefinition *);
	virtual QET::Action categoryIsNotReadable(ElementsCategory *);
	virtual QET::Action elementIsNotReadable(ElementDefinition *);
	virtual QET::Action categoryIsNotWritable(ElementsCategory *);
	virtual QET::Action elementIsNotWritable(ElementDefinition *);
	virtual QET::Action errorWithACategory(ElementsCategory *, const QString &);
	virtual QET::Action errorWithAnElement(ElementDefinition *, const QString &);
	virtual QString nameForRenamingOperation();
	
	private slots:
	void conflictDialogFileNameFieldChanged();
	void conflictDialogButtonClicked(QAbstractButton *);
	
	private:
	void initConflictDialog();
	void setConflictDialogTitle(const QString &);
	QString conflictDialogTitle() const;
	void setConflictDialogMainWidget(QWidget *);
	QWidget *conflictDialogMainWidget() const;
	QET::Action retryErrorMessage(const QString &) const;
	void simpleErrorMessage(const QString &) const;
	
	
	// attributes
	private:
	QWidget *parent_widget_;      ///< Widget to be used as parent when displaying dialogs
	QString rename_;              ///< Name to be used when renaming an item
	bool always_erase_;           ///< Whether to systematically erase conflicting targets without bothering users
	bool always_skip_;            ///< Whether to systematically ignore conflicting targets without bothering users
	bool aborted_;                ///< Whether the movement has been cancelled
	
	// attributes related to the dialog displayed for already existing elements and
	// categories (= i.e. conflict dialog)
	QET::Action conflict_result_;
	QDialog *conflict_dialog_;
	QVBoxLayout *conflict_layout0_;
	QHBoxLayout *conflict_layout1_;
	QLabel *rename_label_;
	QFileNameEdit *rename_textfield_;
	
	/// Buttons for the conflict dialog
	QDialogButtonBox *conflict_buttons_;
	QPushButton *rename_button_;
	QPushButton *erase_button_;
	QPushButton *erase_all_button_;
	QPushButton *ignore_button_;
	QPushButton *ignore_all_button_;
	QPushButton *abort_button_;
};
#endif
