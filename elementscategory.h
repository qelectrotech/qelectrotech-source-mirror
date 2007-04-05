#ifndef ELEMENTS_CATEGORY_H
#define ELEMENTS_CATEGORY_H
#include <QtCore>
class ElementsCategory : public QDir {
	// Constructeur, destructeur
	public:
	ElementsCategory(const QString & = QString());
	~ElementsCategory();
	
	// attributs
	private:
	QHash<QString, QString> category_names;
	
	// methodes
	private:
	bool rmdir(const QString &) const;
	void loadNames();
	
	public:
	QString name() const;
	QHash<QString, QString> categoryNames() const;
	void clearNames();
	void addName(const QString &, const QString &);
	bool write() const;
	bool remove() const;
	//bool move(const QString &new_parent);
};
#endif
