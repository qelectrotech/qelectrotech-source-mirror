#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H
#include <QtGui>
class Diagram;
/**
	Classe representant graphiquement un schema electrique
*/
class DiagramView : public QGraphicsView {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	DiagramView();
	DiagramView(QWidget * = 0);
	virtual ~DiagramView();
	
	private:
	DiagramView(const DiagramView &);
	
	// attributs
	private:
	Diagram *scene;
	QList<QGraphicsItem *> garbage;
	
	// methodes
	public:
	bool ouvrir(QString, int * = NULL);
	void closeEvent(QCloseEvent *);
	QString nom_fichier;
	bool enregistrer();
	bool enregistrer_sous();
	void dialogExport();
	void dialogEditInfos();
	void dialogPrint();
	void addColumn();
	void removeColumn();
	void adjustHeight(qreal);
	void adjustSceneRect();
	void expand();
	void shrink();
	Diagram *diagram() { return(scene); }
	bool hasSelectedItems();
	
	private:
	bool private_enregistrer(QString &);
	void initialise();
	void mousePressEvent(QMouseEvent *);
	void dragEnterEvent(QDragEnterEvent *);
	void dragLeaveEvent(QDragLeaveEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	QRectF viewedSceneRect() const;
	
	signals:
	void selectionChanged();
	void antialiasingChanged();
	void modeChanged();
	
	public slots:
	void selectNothing();
	void selectAll();
	void selectInvert();
	void supprimer();
	void pivoter();
	void setVisualisationMode();
	void setSelectionMode();
	void zoomPlus();
	void zoomMoins();
	void zoomFit();
	void zoomReset();
	void couper();
	void copier();
	void coller();
	
	private slots:
	void slot_selectionChanged();
	void adjustGridToZoom();
};
#endif
