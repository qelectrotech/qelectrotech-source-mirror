#ifndef MACHINE_INFO_H
#define MACHINE_INFO_H

#include <QObject>

/**
	@brief The Machine_info class
	This class hold information from your PC.
*/
class Machine_info : public QObject
{
	Q_OBJECT
public:
	explicit Machine_info(QObject *parent = nullptr);
	int32_t get_max_screen_width();
	int32_t get_max_screen_height();

signals:

private:
	void init_get_Screen_info();
	int32_t Max_screen_width;
	int32_t Max_screen_height;


};

#endif // MACHINE_INFO_H
