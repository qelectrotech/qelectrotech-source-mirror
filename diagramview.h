#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H
#include <QtGui>
class Diagram;
class QETDiagramEditor;
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
	QMenu *context_menu;
	QAction *paste_here;
	QPoint paste_here_pos;
	bool is_adding_text;
	
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
	QETDiagramEditor *diagramEditor() const;
	bool hasSelectedItems();
	void addText();
	
	protected:
	virtual void contextMenuEvent(QContextMenuEvent *);
	virtual void wheelEvent(QWheelEvent *);
	virtual bool event(QEvent *);
	
	private:
	bool saveDiagramToFile(QString &);
	void mousePressEvent(QMouseEvent *);
	void dragEnterEvent(QDragEnterEvent *);
	void dragLeaveEvent(QDragLeaveEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	QRectF viewedSceneRect() const;
	
	signals:
	/// Signal emis lorsque la selection change
	void selectionChanged();
	/// Signal emis lorsque le mode de selection change
	void modeChanged();
	/// Signal emis lorsqu'un texte a ete pose
	void textAdded(bool);
	
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
	void paste(const QPointF & = QPointF(), QClipboard::Mode = QClipboard::Clipboard);
	void pasteHere();
	void adjustSceneRect();
	void updateWindowTitle();
	void editConductor();
	void resetConductors();
	void editDefaultConductorProperties();
	
	private slots:
	void slot_selectionChanged();
	void adjustGridToZoom();
};
#endif
