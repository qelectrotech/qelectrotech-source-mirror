/*
	Copyright 2006-2016 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef ELEMENTFACTORY_H
#define ELEMENTFACTORY_H

#include <QMutex>

class Element;
class ElementsLocation;
class QGraphicsItem;

/**
 * @brief The ElementFactory class
 *this class is a pattern factory and also a singleton factory.
 *this class create new instance of herited class element like
 *simple element or report element.
 *
 */
class ElementFactory
{
		//methods for singleton pattern
	public:
			// return instance of factory
		static ElementFactory* Instance() {
			static QMutex mutex;
			if (!factory_) {
				mutex.lock();
				if (!factory_) factory_ = new ElementFactory();
				mutex.unlock();
			}
			return factory_;
		}

			// delete the instance of factory
		static void dropInstance () {
			static QMutex mutex;
			if (factory_) {
				mutex.lock();
				delete factory_;
				factory_ = 0;
				mutex.unlock();
			}
		}

		//attributes
	private:
		static ElementFactory* factory_;

		//methods for the class factory himself
	private:
		ElementFactory() {}
		ElementFactory (const ElementFactory &);
		ElementFactory operator= (const ElementFactory &);
		~ElementFactory() {}

	public:
		Element *createElement (const ElementsLocation &, QGraphicsItem * = 0, int * = 0);
};
//ElementFactory ElementFactory::factory_ = 0;
#endif // ELEMENTFACTORY_H
