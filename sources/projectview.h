/*
	Copyright 2006-2020 The QElectroTech Team
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
#ifndef PROJECT_VIEW_H
#define PROJECT_VIEW_H

#include <QWidget>
#include <QtWidgets>
#include <QTabBar>

#include "templatelocation.h"
#include "qetresult.h"

#ifdef Q_OS_MACOS

class WheelEnabledTabBar : public QTabWidget
{
public:
	WheelEnabledTabBar(QWidget *parent = nullptr)
		: QTabWidget(parent)
	{}

	double temp_index = 0;

	void wheelEvent(QWheelEvent *event) override
	{
		int index = currentIndex();
		double delta = 0;
		double scale_factor = 0.005; // Decrease or increase speed of mouse wheel (0.04 = decrease)
			if (event->modifiers() & Qt::ControlModifier) {
				if (index != -1) {
					delta = event->delta() * scale_factor; // Read and scale the scroll value
					if (delta > 0 && (temp_index > -1)) temp_index = temp_index - abs(delta);
					if (delta < 0 && (temp_index < count())) temp_index = temp_index + abs(delta);

					index = int (temp_index);
					qDebug() << "index" << index << "temp_index" << temp_index << "  " << event->delta() << delta;

					if (index >= 0 && index < count())
						setCurrentIndex(index);

					//                qDebug() << currentIndex();
				}
			}
	}
};
#endif


class QETProject;
class DiagramView;
class Diagram;
class ElementsLocation;
class QTabWidget;
class QLabel;
class QVBoxLayout;


/**
	@brief The ProjectView class
	This class provides a widget displaying the diagrams of a particular
	project using tabs.
*/
class ProjectView : public QWidget
{
	Q_OBJECT
		
		// constructors, destructor
	public:
		ProjectView(QETProject *, QWidget * = nullptr);
		~ProjectView() override;
	private:
		ProjectView(const ProjectView &);

		// methods
	public:
		QETProject *project();
		void setProject(QETProject *);
		QList<DiagramView *> diagram_views() const;
		DiagramView *currentDiagram() const;
		void closeEvent(QCloseEvent *) override;
		void changeTabUp();
		void changeTabDown();
		void changeFirstTab();
		void changeLastTab();

	public slots:
		void removeDiagram(DiagramView *);
		void removeDiagram(Diagram *);
		void showDiagram(DiagramView *);
		void showDiagram(Diagram *);
		void editProjectProperties();
		void editCurrentDiagramProperties();
		void editDiagramProperties(DiagramView *);
		void editDiagramProperties(Diagram *);
		void moveDiagramUp(DiagramView *);
		void moveDiagramUp(Diagram *);
		void moveDiagramDown(DiagramView *);
		void moveDiagramDown(Diagram *);
		void moveDiagramUpTop(DiagramView *);
		void moveDiagramUpTop(Diagram *);
		void moveDiagramUpx10(DiagramView *);
		void moveDiagramUpx10(Diagram *);
		void moveDiagramDownx10(DiagramView *);
		void moveDiagramDownx10(Diagram *);
		void exportProject();
		QETResult save();
		QETResult saveAs();
		QETResult doSave();
		int cleanProject();
		void updateWindowTitle();
		void updateTabTitle(DiagramView *);
		void updateAllTabsTitle();
		void tabMoved(int, int);

	signals:
		void diagramAdded(DiagramView *);
		void diagramRemoved(DiagramView *);
		void diagramActivated(DiagramView *);
		void projectClosed(ProjectView *);
		void errorEncountered(const QString &);
			// relayed signals
		void findElementRequired(const ElementsLocation &);

	private:
		void initActions();
		void initWidgets();
		void initLayout();
		void loadDiagrams();
		DiagramView *findDiagram(Diagram *);
		DiagramView *nextDiagram();
		DiagramView *previousDiagram();
		DiagramView *firstDiagram();
		DiagramView *lastDiagram();
		void rebuildDiagramsMap();
		bool tryClosing();
		bool tryClosingElementEditors();
		int tryClosingDiagrams();
		QString askUserForFilePath(bool = true);
		QETResult noProjectResult() const;

	private slots:
		void tabChanged(int);
		void tabDoubleClicked(int);
		void setDisplayFallbackWidget(bool);
		void adjustReadOnlyState();
		void diagramAdded(Diagram *diagram);

		// attributes
	private:
		QAction *m_add_new_diagram,
		*m_first_view,
		*m_end_view;
		QETProject *m_project;
		QVBoxLayout *layout_;
		QWidget *fallback_widget_;
		QLabel *fallback_label_;

#ifdef Q_OS_MACOS
		WheelEnabledTabBar *m_tab; 
#else
		QTabWidget *m_tab;
#endif

		QMap<int, DiagramView *> m_diagram_ids;
		int m_previous_tab_index = -1;
		QList<DiagramView *> m_diagram_view_list;
};


#endif
