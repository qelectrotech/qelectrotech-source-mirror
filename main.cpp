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
	// Traducteur
	QTranslator trad;
	//trad.load("qet_en");
	app.installTranslator(&trad);
	// Creation et affichage du QETApp : QElectroTechApplication
	(new QETApp()) -> show();
	// Execution de l'application
	return(app.exec());
}
