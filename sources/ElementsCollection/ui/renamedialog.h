/*
    Copyright 2006-2016 The QElectroTech Team
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
#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <QDialog>
#include "qet.h"

namespace Ui {
    class RenameDialog;
}

class RenameDialog : public QDialog
{
    Q_OBJECT

    public:
        explicit RenameDialog(QString path, QWidget *parent = 0);
        ~RenameDialog();

		QString newName() const {return m_new_name;}
		QET::Action selectedAction() const {return m_action;}

    private slots:
        void on_lineEdit_textEdited(const QString &arg1);
		void on_m_erase_pb_clicked();
		void on_m_rename_pb_clicked();
		void on_m_cancel_pb_clicked();

	private:
        Ui::RenameDialog *ui;
        QString m_path;
        QString m_name;
		QString m_new_name;
		QET::Action m_action;
};

#endif // RENAMEDIALOG_H
