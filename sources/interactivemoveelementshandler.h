/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe implemente la classe strategie MoveElementsHandler.
	Via une interface graphique, elle demande a l'utilisateur comment il faut
	traiter tel ou tel probleme puis transmet la reponse via l'API de la classe
	MoveElementsHandler.
*/
class InteractiveMoveElementsHandler : public BasicMoveElementsHandler {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	InteractiveMoveElementsHandler(QWidget * = 0);
	virtual ~InteractiveMoveElementsHandler();
	private:
	InteractiveMoveElementsHandler(const InteractiveMoveElementsHandler &);
	
	// methodes
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
	
	
	// attributs
	private:
	QWidget *parent_widget_;      ///< Widget a utiliser comme parent pour l'affichage des dialogues
	QString rename_;              ///< Nom a utiliser lors d'une operation de renommage
	bool always_erase_;           ///< Booleen indiquant qu'il faut toujours ecraser les cibles en conflit sans poser de question
	bool always_skip_;            ///< Booleen indiquant qu'il faut toujours ignorer les cibles en conflit sans poser de question
	bool aborted_;                /// Booleen indiquant que le mouvement a ete annule
	
	// attributs relatifs au dialogue affiche pour les elements et categories deja existants (= dialogue de conflit)
	QET::Action conflict_result_;
	QDialog *conflict_dialog_;
	QVBoxLayout *conflict_layout0_;
	QHBoxLayout *conflict_layout1_;
	QLabel *rename_label_;
	QFileNameEdit *rename_textfield_;
	
	/// Boutons pour le dialogue de conflit
	QDialogButtonBox *conflict_buttons_;
	QPushButton *rename_button_;
	QPushButton *erase_button_;
	QPushButton *erase_all_button_;
	QPushButton *ignore_button_;
	QPushButton *ignore_all_button_;
	QPushButton *abort_button_;
};
#endif
