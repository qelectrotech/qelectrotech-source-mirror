/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef ELEMENTPICTUREFACTORY_H
#define ELEMENTPICTUREFACTORY_H

#include <QMutex>
#include <QSharedPointer>
#include <QHash>

class ElementsLocation;
class QPicture;
class QUuid;
class QDomElement;
class QPainter;

/**
 * @brief The ElementPictureFactory class
 * This class is singleton factory, use
 * to create and get the picture use by elements
 */
class ElementPictureFactory
{
	public :
		struct primitives
		{
			QList<QLineF> m_lines;
			QList<QRectF> m_rectangles;
			QList<QRectF> m_circles;
			QList<QVector<QPointF>> m_polygons;
			QList<QVector<qreal>> m_arcs;
		};
		
		
		/**
		 * @brief instance
		 * @return The instance of the factory
		 */
		static ElementPictureFactory* instance()
		{
			static QMutex mutex;
			if (!m_factory)
			{
				mutex.lock();
				if (!m_factory) {
					m_factory = new ElementPictureFactory();	
				}
				mutex.unlock();
			}
			return m_factory;
		}
		
		/**
		 * @brief dropInstance
		 * Drop the instance of factory
		 */
		static void dropInstance()
		{
			static QMutex mutex;
			if (m_factory)
			{
				mutex.lock();
				delete m_factory;
				m_factory = nullptr;
				mutex.unlock();
			}
		}

		void getPictures(const ElementsLocation &location, QPicture &picture, QPicture &low_picture);
		QPixmap pixmap(const ElementsLocation &location);
		ElementPictureFactory::primitives getPrimitives(const ElementsLocation &location);
		
	private:
		ElementPictureFactory() {}
		ElementPictureFactory (const ElementPictureFactory &);
		ElementPictureFactory operator= (const ElementPictureFactory &);
		~ElementPictureFactory() {}
		
		bool build(const ElementsLocation &location);
		void parseElement(const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseLine   (const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseRect   (const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseEllipse(const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseCircle (const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseArc    (const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parsePolygon(const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void parseText   (const QDomElement &dom, QPainter &painter, primitives &prim) const;
		void setPainterStyle(const QDomElement &dom, QPainter &painter) const;
		
		QHash<QUuid, QPicture> m_pictures_H;
		QHash<QUuid, QPicture> m_low_pictures_H;
		QHash<QUuid, QPixmap> m_pixmap_H;
		QHash<QUuid, primitives> m_primitives_H;
		static ElementPictureFactory* m_factory;
};

#endif // ELEMENTPICTUREFACTORY_H
