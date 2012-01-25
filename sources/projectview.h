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
#ifndef PROJECT_VIEW_H
#define PROJECT_VIEW_H
#include <QtGui>
#include "templatelocation.h"
class QETProject;
class DiagramView;
class Diagram;
class ElementsLocation;
class QETTabWidget;
/**
	Cette classe affiche les schemas d'un projet dans des onglets.
*/
class ProjectView : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ProjectView(QETProject *, QWidget * = 0);
	virtual ~ProjectView();
	private:
	ProjectView(const ProjectView &);
	
	// methodes
	public:
	QETProject *project();
	void setProject(QETProject *);
	QList<DiagramView *> diagrams() const;
	DiagramView *currentDiagram() const;
	void closeEvent(QCloseEvent *);
	
	public slots:
	void addNewDiagram();
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
	void printProject();
	void exportProject();
	bool save();
	bool saveAs();
	bool saveAll();
	int cleanProject();
	void updateWindowTitle();
	void updateTabTitle(DiagramView *, const QString &);
	void tabMoved(int, int);
	void editTitleBlockTemplateRequired(const QString &, bool);
	
	signals:
	void diagramAdded(DiagramView *);
	void diagramAboutToBeRemoved(DiagramView *);
	void diagramRemoved(DiagramView *);
	void diagramActivated(DiagramView *);
	void diagramOrderChanged(ProjectView *, int, int);
	void projectClosed(ProjectView *);
	// relayed signals
	void findElementRequired(const ElementsLocation &);
	void editElementRequired(const ElementsLocation &);
	void editTitleBlockTemplate(const TitleBlockTemplateLocation &, bool);
	
	private:
	void loadDiagrams();
	DiagramView *findDiagram(Diagram *);
	void rebuildDiagramsMap();
	bool tryClosing();
	bool tryClosingElementEditors();
	bool tryClosingDiagrams();
	
	private slots:
	void tabChanged(int);
	void tabDoubleClicked(int);
	void firstTabInserted();
	void lastTabRemoved();
	void setDisplayFallbackWidget(bool);
	void adjustReadOnlyState();
	
	// attributs
	private:
	QETProject *project_;
	QVBoxLayout *layout_;
	QWidget *fallback_widget_;
	QETTabWidget *tabs_;
	QMap<int, DiagramView *> diagram_ids_;
	QList<DiagramView *> diagrams_;
};
#endif
