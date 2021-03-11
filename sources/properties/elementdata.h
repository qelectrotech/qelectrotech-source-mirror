/*
		Copyright 2006-2021 The QElectroTech Team
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
#ifndef ELEMENTDATA_H
#define ELEMENTDATA_H

#include "propertiesinterface.h"
#include "../diagramcontext.h"
#include "../NameList/nameslist.h"

/**
 * @brief The ElementData class
 * WARNING
 * This class inherit from PropertiesInterface but
 * only fromXml is actually reimplemented.
 */
class ElementData : public PropertiesInterface
{
		Q_GADGET

	public:
		enum Type {
			Simple         = 1,
			NextReport     = 2,
			PreviousReport = 4,
			AllReport      = 6,
			Master         = 8,
			Slave          = 16,
			Terminale      = 32};
		Q_ENUM(Type)

		enum MasterType {
			Coil,
			Protection,
			Commutator
		};
		Q_ENUM(MasterType)

		enum SlaveType {
			SSimple,
			Power,
			DelayOn,
			DelayOff,
			delayOnOff
		};
		Q_ENUM(SlaveType)

		enum SlaveState {
			NO,
			NC,
			SW
		};
		Q_ENUM(SlaveState)

		enum TerminalType {
			TTGeneric,
			Fuse,
			Sectional,
			Diode
		};
		Q_ENUM(TerminalType)

		enum TerminalFunction {
			TFGeneric,
			Phase,
			Neutral,
			PE
		};
		Q_ENUM(TerminalFunction)

		ElementData() {}
		~ElementData() override {}

		void toSettings(QSettings &settings, const QString prefix = QString()) const override;
		void fromSettings(const QSettings &settings,  const QString prefix = QString()) override;
		QDomElement toXml(QDomDocument &xml_element) const override;
		bool fromXml(const QDomElement &xml_element) override;
		QDomElement kindInfoToXml(QDomDocument &document);

		bool operator==(const ElementData &data) const;
		bool operator!=(const ElementData &data) const;

		static QString typeToString(ElementData::Type type);
		static ElementData::Type typeFromString(const QString &string);

		static QString masterTypeToString(ElementData::MasterType type);
		static ElementData::MasterType masterTypeFromString(const QString &string);

		static QString slaveTypeToString (ElementData::SlaveType type);
		static ElementData::SlaveType slaveTypeFromString(const QString &string);

		static QString slaveStateToString(ElementData::SlaveState type);
		static ElementData::SlaveState slaveStateFromString(const QString &string);

		static QString terminalTypeToString(ElementData::TerminalType type);
		static ElementData::TerminalType terminalTypeFromString(const QString &string);

		static QString terminalFunctionToString(ElementData::TerminalFunction function);
		static ElementData::TerminalFunction terminalFunctionFromString(const QString &string);

		// must be public, because this class is a private member
		// of Element/ element editor and they must access this data
		ElementData::Type       m_type = ElementData::Simple;

		ElementData::MasterType m_master_type = ElementData::Coil;

		ElementData::SlaveType  m_slave_type  = ElementData::SSimple;
		ElementData::SlaveState m_slave_state = ElementData::NO;

		ElementData::TerminalType     m_terminal_type     = ElementData::TTGeneric;
		ElementData::TerminalFunction m_terminal_function = ElementData::TFGeneric;

		int m_contact_count = 1;
		DiagramContext m_informations;
		NamesList m_names_list;
		QString m_drawing_information;

	private:
		void kindInfoFromXml(const QDomElement &xml_element);
};
#endif // ELEMENTDATA_H
