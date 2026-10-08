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
#ifndef SELECTAUTONUMW_H
#define SELECTAUTONUMW_H

#include "../numerotationcontext.h"
#include "formulaautonumberingw.h"

#include <QWidget>

class NumPartEditorW;
class QAbstractButton;
class FormulaAutonumberingW;
class QComboBox;
class QPushButton;

namespace Ui {
	class SelectAutonumW;
}

/**
	@brief The SelectAutonumW class
*/
class SelectAutonumW : public QWidget
{
	Q_OBJECT
	
	//METHODS
	public:
		explicit SelectAutonumW(int type, QWidget *parent = nullptr);
		explicit SelectAutonumW(const NumerotationContext &context,
					int type,
					QWidget *parent = nullptr);
		~SelectAutonumW() override;

		void setContext (const NumerotationContext &context);
		NumerotationContext toNumContext() const;
		void contextToFormula ();
		QString formula();
		QComboBox *contextComboBox() const;
		/**
			Show this editor as the only numbering rule there is: the
			row which lets several named numberings be kept and chosen
			between is hidden, so the rule being edited is the one and
			only. Everything else -- the definition itself, the buttons
			around it -- stays exactly as it is for the other kinds.
		*/
		void setSingleRuleMode(bool single);
		bool isSingleRuleMode() const { return m_single_rule; }
		/**
			True while everything the editor holds is filled in well
			enough to be a rule -- no variable left with nothing in it.
			The Apply button says the same thing; this is so that a page
			which saves on its own way knows it too, rather than writing
			a rule nobody has finished filling in.
		*/
		bool isValid();
		/**
			True while the rule on show has been touched since it was
			read or since it was last applied: a page which saves on
			its own way (the cable rule does) writes it only when there
			is something to write, so that confirming a window with
			nothing changed in it leaves the project alone.
		*/
		bool isModified() const { return m_dirty; }
		/**
			Show the button which takes the rule itself away, which is
			the only way to stop numbering cables automatically again:
			the row the other numberings delete from is hidden here, so
			a single rule would have no way out at all. The page behind
			the editor says whether there is a rule to take away at the
			moment -- without one the button has nothing to do and is
			kept out of sight.
			@param can_remove true when a rule is really there
		*/
		void setRuleRemovable(bool can_remove);

	signals:
		void applyPressed();
		void removeClicked();
			/**
				Sent whenever what the rule holds is edited -- a
				field of a part changed, a part added, a part taken
				away -- so that a window holding this editor can let
				its own buttons say whether there is something to
				write now.
			*/
		void contextEdited();

		//SLOT
	private slots:
		void on_add_button_clicked();
		void on_remove_button_clicked();
		void on_buttonBox_clicked(QAbstractButton *);
		void applyEnable (bool = true);
		void on_m_next_pb_clicked();
		void on_m_previous_pb_clicked();
		void on_m_comboBox_currentTextChanged(const QString &arg1);
		
		//ATTRIBUTES
		void on_m_remove_pb_clicked();
		
	private:
		Ui::SelectAutonumW *ui;
		QList <NumPartEditorW *> num_part_list_;
		NumerotationContext m_context;
		FormulaAutonumberingW *m_feaw;
		FormulaAutonumberingW *m_fcaw;
		int m_edited_type = -1; ///<0 == element : 1 == conductor : 2 == folio : 3 == cable
			///< when true, the row of available numberings is hidden and
			///< this editor holds the one rule there is (see setSingleRuleMode)
		bool m_single_rule = false;
			///< true from the moment the rule on show is touched until
			///< it is read again or applied (see isModified)
		bool m_dirty = false;
			///< the button which takes the whole rule away, built for
			///< the one kind of numbering which has no list to delete
			///< it from (see setRuleRemovable)
		QPushButton *m_rule_remove_pb = nullptr;
			///< whether such a rule is there to be taken away
		bool m_rule_removable = false;
};

#endif // SELECTAUTONUMW_H
