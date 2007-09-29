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
	DiagramView(QWidget * = 0);
	virtual ~DiagramView();
	
	private:
	DiagramView(const DiagramView &);
	
	// attributs
	public:
	QString file_name;
	
	private:
	Diagram *scene;
	
	// methodes
	public:
	bool open(QString, int * = NULL);
	void closeEvent(QCloseEvent *);
	bool save();
	bool saveAs();
	void dialogExport();
	void dialogEditInfos();
	void dialogPrint();
	void addColumn();
	void removeColumn();
	void expand();
	void shrink();
	Diagram *diagram() { return(scene); }
	bool hasSelectedItems();
	
	private:
	bool saveDiagramToFile(QString &);
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
	void deleteSelection();
	void rotateSelection();
	void setVisualisationMode();
	void setSelectionMode();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoomReset();
	void cut();
	void copy();
	void paste();
	void adjustSceneRect();
	void updateWindowTitle();
	
	private slots:
	void slot_selectionChanged();
	void adjustGridToZoom();
};
#endif
