#ifndef QGI_MANAGER_H
#define QGI_MANAGER_H
#include <QtCore>
#include <QGraphicsScene>
#include <QGraphicsItem>
class QGIManager {
	// constructeurs, destructeurs
	public:
	QGIManager(QGraphicsScene *);
	virtual ~QGIManager();
	private:
	QGIManager(const QGIManager &);
	
	// attributs
	private:
	QGraphicsScene *scene;
	QHash<QGraphicsItem *, int> qgi_manager;
	bool destroy_qgi_on_delete;
	
	//methodes
	public:
	void manage(QGraphicsItem *);
	void release(QGraphicsItem *);
	void setDestroyQGIOnDelete(bool);
};
#endif
