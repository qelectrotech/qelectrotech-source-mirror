/*
        Copyright 2006-2023 The QElectroTech Team
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
#ifndef TERMINALSTRIPLAYOUTEDITOR_H
#define TERMINALSTRIPLAYOUTEDITOR_H

#include <QWidget>

class TerminalStripLayoutPattern;

namespace Ui {
    class TerminalStripLayoutEditor;
}

/**
 * @brief The TerminalStripLayoutEditor class
 * Widget used to edit the layout of a terminal strip item
 */
class TerminalStripLayoutEditor : public QWidget
{
        Q_OBJECT

    public:
        explicit TerminalStripLayoutEditor(QSharedPointer<TerminalStripLayoutPattern> layout,
                                           QWidget *parent = nullptr);
        ~TerminalStripLayoutEditor();

    private slots:
        void valueEdited();

    private:
        void updateUi();

    private:
        Ui::TerminalStripLayoutEditor *ui;
        QSharedPointer<TerminalStripLayoutPattern> m_layout;
        bool m_ui_updating { false } ;
};

#endif // TERMINALSTRIPLAYOUTEDITOR_H
