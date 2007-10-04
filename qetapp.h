#ifndef QET_APP_H
#define QET_APP_H
#include <QApplication>
#include <QTranslator>
#include <QtGui>
class QETDiagramEditor;
class QETElementEditor;
/**
	Cette classe represente l'application QElectroTech.
	
*/
class QETApp : public QApplication {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	QETApp(int &, char **);
	virtual ~QETApp();
	
	private:
	QETApp(const QETApp &);
	
	// methodes
	public:
	void setLanguage(const QString &);
	
	static QString commonElementsDir();
	static QString customElementsDir();
	static QString configDir();
	static QString languagesPath();
	static QString realPath(const QString &);
	static QString symbolicPath(const QString &);
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	static void overrideCommonElementsDir(const QString &);
	private:
	static QString common_elements_dir;
#endif
	
	// attributs
	private:
	QTranslator qtTranslator;
	QTranslator qetTranslator;
	QSystemTrayIcon *qsti;
	QMenu *menu_systray;
	QAction *quitter_qet;
	QAction *reduce_appli;
	QAction *restore_appli;
	QAction *reduce_diagrams;
	QAction *restore_diagrams;
	QAction *reduce_elements;
	QAction *restore_elements;
	QAction *new_diagram;
	QAction *new_element;
	QHash<QMainWindow *, QByteArray> window_geometries;
	QHash<QMainWindow *, QByteArray> window_states;
	bool every_editor_reduced;
	bool every_diagram_reduced;
	bool every_diagram_visible;
	bool every_element_reduced;
	bool every_element_visible;
	QSignalMapper signal_map;
	
	public slots:
	void systray(QSystemTrayIcon::ActivationReason);
	void reduceEveryEditor();
	void restoreEveryEditor();
	void reduceDiagramEditors();
	void restoreDiagramEditors();
	void reduceElementEditors();
	void restoreElementEditors();
	void newDiagramEditor();
	void newElementEditor();
	bool closeEveryEditor();
	void setMainWindowVisible(QMainWindow *, bool);
	void invertMainWindowVisibility(QWidget *);
	void quitQET();
	void checkRemainingWindows();
	
	// methodes privees
	private slots:
	void cleanup();
	
	private:
	QList<QETDiagramEditor *> diagramEditors() const;
	QList<QETElementEditor *> elementEditors() const;
	QList<QWidget *> floatingToolbarsAndDocksForMainWindow(QMainWindow *) const;
	void buildSystemTrayMenu();
	void fetchWindowStats(const QList<QETDiagramEditor *> &diagrams, const QList<QETElementEditor *> &elements);
};
#endif
