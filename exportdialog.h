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
		ExportDialog(Diagram &, QWidget * = 0);
		
		private:
		// elements graphiques
		QLineEdit *filename;
		QPushButton *button_browse;
		QComboBox *format;
		QSpinBox *width;
		QSpinBox *height;
		QCheckBox *keep_aspect_ratio;
		QCheckBox *export_grid;
		QCheckBox *keep_colors;
		QDialogButtonBox *buttons;
		
		// booleens pour ne pas avoir de boucle lors de l'edition des dimensions de l'image
		bool dontchangewidth;
		bool dontchangeheight;
		
		// elements relatifs au traitement effectue par le dialogue
		Diagram *diagram;
		QSize diagram_size;
		QString diagram_path;
		qreal diagram_ratio;
		
		QGroupBox *setupDimensionsGroupBox();
		QGroupBox *setupOptionsGroupBox();
		
		public slots:
		void slot_correctWidth();
		void slot_correctHeight();
		void slot_chooseAFile();
		void slot_check();
	};
#endif
