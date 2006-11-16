#include <QApplication>
#include <QTranslator>
#include "qetapp.h"

/**
	Fonction principale du programme QElectroTech
	@param argc nombre de parametres
	@param argv parametres
*/
int main(int argc, char **argv) {
	// Creation de l'application
	QApplication app(argc, argv);
	
	// determine la langue a utiliser pour l'application
	QTranslator trad;
	QString system_language = QLocale::system().name().left(2);
	if (system_language != "fr") {
		// utilisation de la version anglaise par defaut
		if (!trad.load("qet_" + system_language, QETApp::languagesPath())) trad.load("qet_en", QETApp::languagesPath());
		app.installTranslator(&trad);
	}
	
	// Creation et affichage du QETApp : QElectroTechApplication
	(new QETApp()) -> show();
	// Execution de l'application
	return(app.exec());
}
