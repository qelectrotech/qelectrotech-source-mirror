#ifndef PANELAPPAREILS_H
#define PANELAPPAREILS_H
#include <QtGui>
#include "qetdiagrameditor.h"
/**
	Cette classe represente le panel d'appareils (en tant qu'element
	graphique) dans lequel l'utilisateur choisit les composants de
	son choix et les depose sur le schema par drag'n drop.
*/
class ElementsPanel : public QTreeWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ElementsPanel(QWidget * = 0);
	virtual ~ElementsPanel();
	
	private:
	ElementsPanel(const ElementsPanel &);
	
	// methodes
	public:
	bool selectedItemIsAnElement() const;
	bool selectedItemIsACategory() const;
	
	private:
	void addFile(QTreeWidgetItem *, QString);
	void addDir(QTreeWidgetItem *, QString, QString = QString());
	QFileInfo selectedFile() const;
	void launchElementEditor(const QString &);
	void launchCategoryEditor(const QString &);
	void saveCollapsedCategories();
	QStringList collapsed_directories;
	
	public slots:
	void slot_doubleClick(QTreeWidgetItem *, int);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void startDrag(Qt::DropActions);
	void reload();
	void editCategory();
	void editElement();
	void deleteCategory();
	void deleteElement();
};
#endif
