#include "machine_info.h"
#include <QScreen>
#include <QApplication>

/**
	@brief Machine_info::Machine_info
	@param parent
*/
Machine_info::Machine_info(QObject *parent) : QObject(parent)
{
	init_get_Screen_info();
}

/**
	@brief Machine_info::init_get_Screen_info
	Finds the largest screen and saves the values
*/
void Machine_info::init_get_Screen_info()
{
	const auto screens = qApp->screens();
	for (int ii = 0; ii < screens.count(); ++ii)
	{
		if(
				Max_screen_width
				<
				screens[ii]->geometry().width()
				*
				screens[ii]->devicePixelRatio()
				)
		{
			Max_screen_width =
					screens[ii]->geometry().width()
					*
					screens[ii]->devicePixelRatio();
		}
		if(
				Max_screen_height
				<
				screens[ii]->geometry().height()
				*
				screens[ii]->devicePixelRatio()
				)
		{
			Max_screen_height =
					screens[ii]->geometry().height()
					*
					screens[ii]->devicePixelRatio();
		}
	}
}

/**
	@brief Machine_info::get_max_screen_width
	@return max screen width
 */
int32_t Machine_info::get_max_screen_width()
{
	return Max_screen_width;
}

/**
	@brief Machine_info::get_max_screen_height
	@return max screen height
*/
int32_t Machine_info::get_max_screen_height()
{
	return Max_screen_height;
}
