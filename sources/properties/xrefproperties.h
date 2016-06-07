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
#ifndef XREFPROPERTIES_H
#define XREFPROPERTIES_H

#include "propertiesinterface.h"
#include <QStringList>

/**
 * @brief The XRefProperties class
 * this class store properties used by XrefItem
 */
class XRefProperties : public PropertiesInterface
{
	public:
	XRefProperties();

	enum DisplayHas {
		Cross,
		Contacts
	};

	enum SnapTo {
		Bottom,
		Label
	};

	virtual void toSettings	  (QSettings &settings, const QString = QString()) const;
	virtual void fromSettings (const QSettings &settings, const QString = QString());
	virtual void toXml		  (QDomElement &xml_element) const;
	virtual void fromXml	  (const QDomElement &xml_element);

	static QHash<QString, XRefProperties> defaultProperties();

	bool operator == (const XRefProperties &xrp) const;
	bool operator != (const XRefProperties &xrp) const;

	void setShowPowerContac (const bool a) {m_show_power_ctc = a;}
	bool showPowerContact	() const	   {return m_show_power_ctc;}

	void setDisplayHas	  (const DisplayHas dh) {m_display = dh;}
	DisplayHas displayHas () const				{return m_display;}

	void setSnapTo (const SnapTo st) {m_snap_to = st;}
	SnapTo snapTo  () const			 {return m_snap_to;}

	void setPrefix (const QString &key, const QString &value) {m_prefix.insert(key, value);}
	QString prefix (const QString &key) const {return m_prefix.value(key);}

	void setMasterLabel (const QString master) {m_master_label = master;}
	QString masterLabel () const			   {return m_master_label;}

	void setSlaveLabel(const QString slave) {m_slave_label = slave;}
	QString slaveLabel () const				{return m_slave_label;}

	void setOffset(const int offset) {m_offset = offset;}
	int offset() const				 {return m_offset;}

	private:
	bool m_show_power_ctc;
	DisplayHas m_display;
	SnapTo m_snap_to;
	QHash <QString, QString> m_prefix;
	QStringList m_prefix_keys;
	QString m_master_label;
	QString m_slave_label;
	int     m_offset;
};

#endif // XREFPROPERTIES_H
