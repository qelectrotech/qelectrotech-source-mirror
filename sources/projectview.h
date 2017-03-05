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
#ifndef PROJECT_VIEW_H
#define PROJECT_VIEW_H

#include <QWidget>

#include "templatelocation.h"
#include "qetresult.h"

class QETProject;
class DiagramView;
class Diagram;
class ElementsLocation;
class QTabWidget;
class QLabel;
class QVBoxLayout;

/**
	This class provides a widget displaying the diagrams of a particular
	project using tabs.
*/
class ProjectView : public QWidget
{
	Q_OBJECT
		
		// constructors, destructor
	public:
		ProjectView(QETProject *, QWidget * = 0);
		virtual ~ProjectView();
	private:
		ProjectView(const ProjectView &);

		// methods
	public:
		QETProject *project();
		void setProject(QETProject *);
		QList<DiagramView *> diagram_views() const;
		DiagramView *currentDiagram() const;
		void closeEvent(QCloseEvent *);
		void changeTabUp();
		void changeTabDown();
		void changeFirstTab();
		void changeLastTab();

	public slots:
		void addNewDiagram();
		void addNewDiagramFolioList();
		void addDiagram(DiagramView *);
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
		void printProject();
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
		void editElementRequired(const ElementsLocation &);

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

		// attributes
	private:
		QAction *add_new_diagram_;
		QETProject *m_project;
		QVBoxLayout *layout_;
		QWidget *fallback_widget_;
		QLabel *fallback_label_;
		QTabWidget *m_tab;
		QMap<int, DiagramView *> m_diagram_ids;
		QList<DiagramView *> m_diagram_view_list;
};

#endif
