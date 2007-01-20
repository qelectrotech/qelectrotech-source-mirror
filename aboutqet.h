#ifndef ABOUTQET_H
	#define ABOUTQET_H
	#include <QtGui>
	/**
		Cette classe represente la boite de dialogue
		« A propos de QElectroTech »
	*/
	class AboutQET : public QDialog {
		Q_OBJECT
		public:
		AboutQET(QWidget * = 0);
		private:
		QWidget *titre() const;
		QWidget *ongletAPropos() const;
		QWidget *ongletAuteurs() const;
		QWidget *ongletLicence() const;
	};
#endif
