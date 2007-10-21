#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H
#include <QtGui>
#include "diagram.h"
class QSvgGenerator;
/**
	Cette classe represente le dialogue permettant d'exporter un schema
	sous forme d'image selon les desirs de l'utilisateur
*/
class ExportDialog : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	ExportDialog(Diagram *, QWidget * = 0);
	virtual ~ExportDialog();
	
	private:
	ExportDialog(const ExportDialog &);
	
	// attributs
	private:
	// elements graphiques
	QLineEdit *filename;
	QPushButton *button_browse;
	QComboBox *format;
	QSpinBox *width;
	QSpinBox *height;
	QCheckBox *keep_aspect_ratio;
	QCheckBox *draw_grid;
	QCheckBox *draw_border;
	QCheckBox *draw_inset;
	QCheckBox *draw_columns;
	QCheckBox *draw_terminals;
	QRadioButton *export_elements;
	QRadioButton *export_border;
	QDialogButtonBox *buttons;
	QGraphicsScene *preview_scene;
	QGraphicsView *preview_view;
	
	// booleens pour ne pas avoir de boucle lors de l'edition des dimensions de l'image
	bool dontchangewidth;
	bool dontchangeheight;
	
	// elements relatifs au traitement effectue par le dialogue
	Diagram *diagram;
	QSize diagram_size;
	qreal diagram_ratio;
	QVector<QRgb> ColorTab;
	
	// methodes
	private:
	QWidget *leftPart();
	QWidget *rightPart();
	QGroupBox *setupDimensionsGroupBox();
	QGroupBox *setupOptionsGroupBox();
	void saveReloadDiagramParameters(bool = true);
	void generateSvg(QFile &file);
	QImage generateImage();
	
	public slots:
	void slot_correctWidth();
	void slot_correctHeight();
	void slot_chooseAFile();
	void slot_check();
	void slot_changeUseBorder();
	void slot_refreshPreview();
};
#endif
