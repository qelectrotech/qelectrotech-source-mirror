#ifndef MULTIPASTEDIALOG_H
#define MULTIPASTEDIALOG_H

#include <QDialog>
#include "diagramcontent.h"
#include "QDomDocument"

class Diagram;

namespace Ui {
	class MultiPasteDialog;
}

class MultiPasteDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit MultiPasteDialog(Diagram *diagram, QWidget *parent = 0);
		~MultiPasteDialog();
		void updatePreview();
	
		private slots:
		void on_m_button_box_accepted();
		
		private:
		Ui::MultiPasteDialog *ui;
		Diagram *m_diagram = nullptr;
		DiagramContent m_pasted_content;
		QPointF m_origin;
		QDomDocument m_document;
		bool m_accept = false;
};

#endif // MULTIPASTEDIALOG_H
