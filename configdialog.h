#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H
#include <QDialog>
#include "configpages.h"
class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QDialogButtonBox;
/**
	Cette classe represente le dialogue de configuration de QElectroTech.
	Il s'agit d'un dialogue affichant des "pages de configuration".
	Chaque page de configuration doit fournir une icone et un titre.
*/
class ConfigDialog : public QDialog {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	ConfigDialog(QWidget * = 0);
	virtual ~ConfigDialog();
	private:
	ConfigDialog(const ConfigDialog &);
	
	// methodes
	public slots:
    void changePage(QListWidgetItem *, QListWidgetItem *);
	void applyConf();
	void addPage(ConfigPage *);
	
	private:
	void buildPagesList();
	
	// attributs
	private:
	QListWidget *pages_list;
	QStackedWidget *pages_widget;
	QDialogButtonBox *buttons;
	QList<ConfigPage *> pages;
};
#endif
