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
	static QString realPath(QString &);
	static QString symbolicPath(QString &);
	
	// attributs
	private:
	QTranslator qtTranslator;
	QTranslator qetTranslator;
	QSystemTrayIcon *qsti;
	/// Elements de menus pour l'icone du systray
	QMenu *menu_systray;
	QAction *systray_masquer;
	QAction *config_fullscreen;
	/// Geometrie de la fenetre principale
	QByteArray wg;
	QAction *reduce_appli;
	QAction *restore_appli;
	QAction *menu_systray_masquer_restaurer;
	QAction *quitter_qet;
	QHash<QMainWindow *, QByteArray> window_geometries;
	QHash<QMainWindow *, QByteArray> window_states;
	bool every_editor_reduced;
	
	public slots:
	void systray(QSystemTrayIcon::ActivationReason);
	void systrayReduce();
	void systrayRestore();
	void closeEveryEditor();
	
	// methodes privees
	private slots:
	void cleanup();
	
	private:
	QList<QETDiagramEditor *> diagramEditors() const;
	QList<QETElementEditor *> elementEditors() const;
	void setMainWindowVisible(QMainWindow *, bool);
};
#endif
