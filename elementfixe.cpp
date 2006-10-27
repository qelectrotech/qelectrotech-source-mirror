#include "elementfixe.h"
/**
	Constructeur
*/
ElementFixe::ElementFixe(QGraphicsItem *parent, Schema *scene) : Element(parent, scene) {
}

/**
	@return Le nombre minimal de bornes que l'element peut avoir
*/
int ElementFixe::nbBornesMin() const {
	return(nbBornes());
}

/**
	@return Le nombre maximal de bornes que l'element peut avoir
*/
int ElementFixe::nbBornesMax() const {
	return(nbBornes());
}

/**
	Methode d'import XML. Cette methode est appelee lors de l'import de contenu XML (coller, import, ouverture de fichier...) afin que l'element puisse gerer lui-meme l'importation de ses bornes. Ici, comme cette classe est caracterisee par un nombre fixe de bornes, l'implementation exige de retrouver exactement ses bornes dans le fichier XML.
	@param e L'element XML a analyser.
	@param table_id_adr Reference vers la table de correspondance entre les IDs du fichier XML et les adresses en memoire. Si l'import reussit, il faut y ajouter les bons couples (id, adresse).
	@return true si l'import a reussi, false sinon
	
*/
bool ElementFixe::fromXml(QDomElement &e, QHash<int, Borne *> &table_id_adr) {
	/*
		les bornes vont maintenant etre recensees pour associer leurs id à leur adresse reelle
		ce recensement servira lors de la mise en place des fils
	*/
	
	QList<QDomElement> liste_bornes;
	// parcours des enfants de l'element
	for (QDomNode enfant = e.firstChild() ; !enfant.isNull() ; enfant = enfant.nextSibling()) {
		// on s'interesse a l'element XML "bornes"
		QDomElement bornes = enfant.toElement();
		if (bornes.isNull() || bornes.tagName() != "bornes") continue;
		// parcours des enfants de l'element XML "bornes"
		for (QDomNode node_borne = bornes.firstChild() ; !node_borne.isNull() ; node_borne = node_borne.nextSibling()) {
			// on s'interesse a l'element XML "borne"
			QDomElement borne = node_borne.toElement();
			if (!borne.isNull() && Borne::valideXml(borne)) liste_bornes.append(borne);
		}
	}
	
	QHash<int, Borne *> priv_id_adr;
	int bornes_non_trouvees = 0;
	foreach(QGraphicsItem *qgi, children()) {
		if (Borne *p = qgraphicsitem_cast<Borne *>(qgi)) {
			bool borne_trouvee = false;
			foreach(QDomElement qde, liste_bornes) {
				if (p -> fromXml(qde)) {
					priv_id_adr.insert(qde.attribute("id").toInt(), p);
					borne_trouvee = true;
					break;
				}
			}
			if (!borne_trouvee) ++ bornes_non_trouvees;
		}
	}
	
	if (bornes_non_trouvees > 0) {
		return(false);
	} else {
		// verifie que les associations id / adr n'entrent pas en conflit avec table_id_adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			if (table_id_adr.contains(id_trouve)) {
				// cet element possede un id qui est deja reference (= conflit)
				return(false);
			}
		}
		// copie des associations id / adr
		foreach(int id_trouve, priv_id_adr.keys()) {
			table_id_adr.insert(id_trouve, priv_id_adr.value(id_trouve));
		}
	}
	
	return(true);
}
