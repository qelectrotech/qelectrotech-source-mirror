#ifndef ABOUTQET_H
#define ABOUTQET_H
#include <QtGui>
/**
	Cette classe represente la boite de dialogue
	« A propos de QElectroTech »
*/
class AboutQET : public QDialog {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	AboutQET(QWidget * = 0);
	virtual ~AboutQET();
	
	private:
	AboutQET(AboutQET &);
	
	// methodes
	private:
	QWidget *titre() const;
	QWidget *ongletAPropos() const;
	QWidget *ongletAuteurs() const;
	QWidget *ongletLicence() const;
};
#endif
