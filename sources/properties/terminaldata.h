#ifndef TERMINALDATA_H
#define TERMINALDATA_H

#include "propertiesinterface.h"
#include "qet.h"

#include <QUuid>
#include <QPointF>

class QGraphicsObject;

/**
	@brief The TerminalData class
	Data of the terminal.
	Stored in extra class so it can be used by PartTerminal
	and Terminal without defining everything again.
	@note tis class needs to be work on
*/
class TerminalData : public PropertiesInterface
{
	public:
		TerminalData();
		TerminalData(QGraphicsObject* parent);
		~TerminalData();

		void init();

		void setParent(QGraphicsObject* parent);
		void toSettings(QSettings &settings,
				const QString prefix = QString()) const override;
		void fromSettings(const QSettings &settings,
				  const QString prefix = QString()) override;
		QDomElement toXml(QDomDocument &xml_element) const override;
		bool fromXml(const QDomElement &xml_element) override;

	// must be public, because this class is a private member
	// of PartTerminal/Terminal and they must access this data
	public:
		/**
			@brief m_orientation
			Orientation of the terminal
		*/
		Qet::Orientation m_orientation;
		/**
			@brief second_point
			Position of the second point of the terminal
			in scene coordinates
		*/
		QPointF second_point;
		/**
			@brief m_uuid
			Uuid of the terminal.

			In elementscene.cpp an element gets a new uuid when
			saving the element. In the current state
			each connection is made by using the local position
			of the terminal and a dynamic id. In the new
			case, each terminal should have it's own uuid to
			identify it uniquely. When changing each time this
			uuid, the conductor after updating the part is anymore
			valid. So if in the loaded document a uuid exists,
			use this one and don't create a new one.
		*/
		QUuid m_uuid;
		/**
			@brief m_name
			Name of the element.
			It can be used to create wiring harness tables
		*/
		QString m_name;

		/**
			@brief m_pos
			Position of the terminal. The second point is calculated
			from this position and the orientation
			@note
			Important: this variable is only updated during read
			from xml and not during mouse move!
			It is used to store the initial position so that
			PartTerminal and Terminal have access to it.
		*/
		QPointF m_pos;
	private:
		QGraphicsObject* q{nullptr};
};

#endif // TERMINALDATA_H
