#ifndef EXPORTDIALOG_H
	#define EXPORTDIALOG_H
	#include <QtGui>
	#include "diagram.h"
	/**
		Cette classe represente le dialogue permettant d'exporter un schema
		sous forme d'image selon les desirs de l'utilisateur
	*/
	class ExportDialog : public QDialog {
		Q_OBJECT
		public:
		ExportDialog(Diagram *, QWidget * = 0);
		~ExportDialog();
		
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
		QCheckBox *keep_colors;
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
		QString diagram_path;
		qreal diagram_ratio;
		
		QWidget *leftPart();
		QWidget *rightPart();
		QGroupBox *setupDimensionsGroupBox();
		QGroupBox *setupOptionsGroupBox();
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
