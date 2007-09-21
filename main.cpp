#include <QApplication>
#include <QTranslator>
#include "qetdiagrameditor.h"

/**
	Fonction principale du programme QElectroTech
	@param argc nombre de parametres
	@param argv parametres
*/
int main(int argc, char **argv) {
	// Creation de l'application
	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	
	QString system_language = QLocale::system().name().left(2);
	// charge les eventuelles traductions pour la lib Qt
	QTranslator qtTranslator;
	qtTranslator.load("qt_" + system_language, QETDiagramEditor::languagesPath());
	app.installTranslator(&qtTranslator);
	
	// determine la langue a utiliser pour l'application
	QTranslator trad;
	if (system_language != "fr") {
		// utilisation de la version anglaise par defaut
		if (!trad.load("qet_" + system_language, QETDiagramEditor::languagesPath())) trad.load("qet_en", QETDiagramEditor::languagesPath());
		app.installTranslator(&trad);
	}
	
	// Creation et affichage d'un editeur de schema
	(new QETDiagramEditor()) -> show();
	// Execution de l'application
	return(app.exec());
}
