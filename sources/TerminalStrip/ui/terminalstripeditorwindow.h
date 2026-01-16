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
#ifndef TERMINALSTRIPEDITORWINDOW_H
#define TERMINALSTRIPEDITORWINDOW_H

#include <QMainWindow>
#include <QMutex>
#include <QPointer>

class QETProject;
class TerminalStripTreeDockWidget;
class TerminalStrip;
class FreeTerminalEditor;
class TerminalStripEditor;
class QAbstractButton;

namespace Ui {
	class TerminalStripEditorWindow;
}

class TerminalStripEditorWindow : public QMainWindow
{
		Q_OBJECT

	private:
			//We need to use a QPointer instead of a raw pointer because when window_
			//have got a parent widget, the parent widget can delete the window_
			//instance in her destrucor and then window_ become a dangling pointer.
		static QPointer<TerminalStripEditorWindow> window_;

    public:
        static TerminalStripEditorWindow* instance(QETProject *project, QWidget *parent = nullptr) {
            static QMutex mutex_;
            if (!window_) {
                mutex_.lock();
                if (!window_)
                    window_ = new TerminalStripEditorWindow{project, parent};
                mutex_.unlock();
            } else {
                window_->setProject(project);
            }
            return window_;
        }

		static void dropInstance () {
			static QMutex mutex;
			if (window_) {
				mutex.lock();
				window_->deleteLater();
				window_.clear();
				mutex.unlock();
			}
		}

		static void edit(TerminalStrip *strip);

	public:
		explicit TerminalStripEditorWindow(QETProject *project, QWidget *parent = nullptr);
		~TerminalStripEditorWindow();

        void setProject(QETProject *project);
        void setCurrentStrip(TerminalStrip *strip);

	private slots:
		void on_m_add_terminal_strip_triggered();
		void on_m_remove_terminal_triggered();
		void on_m_reload_triggered();
		void on_m_button_box_clicked(QAbstractButton *button);
		void on_m_stacked_widget_currentChanged(int arg1);

	private:
		void addTreeDockWidget();
		void currentStripChanged(TerminalStrip *strip);
		void updateUi();

	private:
		Ui::TerminalStripEditorWindow *ui{nullptr};
        QPointer <QETProject> m_project;
		TerminalStripTreeDockWidget *m_tree_dock{nullptr};
		FreeTerminalEditor *m_free_terminal_editor {nullptr};
		TerminalStripEditor *m_terminal_strip_editor {nullptr};
};

#endif // TERMINALSTRIPEDITORWINDOW_H
