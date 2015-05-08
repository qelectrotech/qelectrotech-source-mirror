#ifndef IMAGEPROPERTIESDIALOG_H
#define IMAGEPROPERTIESDIALOG_H

#include <QDialog>

class ImagePropertiesWidget;
class DiagramImageItem;

namespace Ui {
	class ImagePropertiesDialog;
}

class ImagePropertiesDialog : public QDialog
{
		Q_OBJECT

	public:
		explicit ImagePropertiesDialog(DiagramImageItem *image = nullptr, QWidget *parent = 0);
		~ImagePropertiesDialog();
		void setImageItem (DiagramImageItem *image);

	private slots:
		void on_buttonBox_accepted();
		void on_buttonBox_rejected();

	private:
		Ui::ImagePropertiesDialog *ui;
		ImagePropertiesWidget *m_editor;
};

#endif // IMAGEPROPERTIESDIALOG_H
