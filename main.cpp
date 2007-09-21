#include "qetapp.h"

/**
	Fonction principale du programme QElectroTech
	@param argc nombre de parametres
	@param argv parametres
*/
int main(int argc, char **argv) {
	// Creation de l'application
	QETApp app(argc, argv);
	
	// Creation et affichage d'un editeur de schema
	(new QETDiagramEditor()) -> show();
	
	// Execution de l'application
	return(app.exec());
}
