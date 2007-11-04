#ifndef NAMES_LIST_H
#define NAMES_LIST_H
#include <QtXml>
/**
	Cette classe represente une liste de noms, utilisee
	par les elements et categories pour embarquer un meme nom en plusieurs
	langues.
	Les langues sont representees par deux lettres (typiquement : les deux
	premieres de la locale du systeme) ; exemples : en pour l'anglais, fr
	pour le francais.
*/
class NamesList {
	// constructeurs, destructeur
	public:
	NamesList();
	NamesList(const NamesList &);
	virtual ~NamesList();
	
	// attributs
	private:
	QHash<QString, QString> hash_names;
	
	// methodes
	public:
	// methodes relatives a la gestion de la liste
	void addName(const QString &, const QString &);
	void removeName(const QString &);
	void clearNames();
	QList<QString> langs() const;
	bool isEmpty() const;
	int count() const;
	QString &operator[](const QString &);
	const QString operator[](const QString &) const;
	bool operator!=(const NamesList &) const;
	bool operator==(const NamesList &) const;
	QString name(const QString & = QString()) const;
	
	// methodes relatives a XML
	void fromXml(const QDomElement &);
	QDomElement toXml(QDomDocument &) const;
};
#endif
