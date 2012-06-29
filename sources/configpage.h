#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H

#include <QWidget>

/**
	This abstract class specifies methods all derived classes should should
	implement.
*/
class ConfigPage : public QWidget {
	Q_OBJECT
	public:
	/**
		Constructor
		@param parent Parent QWidget
	*/
	ConfigPage(QWidget *parent) : QWidget(parent) {};
	/// Destructor
	virtual ~ConfigPage() {};
	/// Apply the configuration after user input
	virtual void applyConf() = 0;
	/// @return the configuration page title
	virtual QString title() const = 0;
	/// @return the configuration page icon
	virtual QIcon icon() const = 0;
};

#endif
