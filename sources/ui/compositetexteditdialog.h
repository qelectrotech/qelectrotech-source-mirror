#ifndef COMPOSITETEXTEDITDIALOG_H
#define COMPOSITETEXTEDITDIALOG_H

#include <QDialog>

namespace Ui {
	class CompositeTextEditDialog;
}

class DynamicElementTextItem;

/**
 * @brief The CompositeTextEditDialog class
 * CompositeTextEditDialog display a dialog use to write the complex text
 * of a dynamic element text item.
 * This dialog provide a QComboBox to quickly add an information of the element
 * in the composite text.
 */
class CompositeTextEditDialog : public QDialog
{
	Q_OBJECT
	
	public:
		explicit CompositeTextEditDialog(DynamicElementTextItem *text, QWidget *parent = nullptr);
		~CompositeTextEditDialog() override;
		
		QString plainText() const;
	
	private slots:
		void on_m_info_cb_activated(const QString &arg1);
	
	private :
		void setUpComboBox();
		QString infoToVar(const QString& info) const;
	
	private:
		Ui::CompositeTextEditDialog *ui;
		QString m_default_text;
		DynamicElementTextItem *m_text = nullptr;
};

#endif // COMPOSITETEXTEDITDIALOG_H
