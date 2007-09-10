#ifndef NAMES_LIST_H
#define NAMES_LIST_H
#include <QtXml>
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
	const QString &name(const QString & = QString()) const;
	
	// methodes relatives a XML
	void fromXml(const QDomElement &);
	QDomElement toXml(QDomDocument &) const;
};
#endif
