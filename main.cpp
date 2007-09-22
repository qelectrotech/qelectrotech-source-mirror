#include "qetapp.h"
/**
	Fonction principale du programme QElectroTech
	@param argc nombre de parametres
	@param argv parametres
*/
int main(int argc, char **argv) {
	// Creation et execution de l'application
	return(QETApp(argc, argv).exec());
}
