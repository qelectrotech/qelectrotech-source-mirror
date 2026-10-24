/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "connectionsymbols.h"

#include <QCoreApplication>
#include <QSettings>

namespace {
	QString key(ConnectionSymbols::Kind kind)
	{
		static const char *names[] = {"corner", "tee", "interruption",
					      "report_outgoing", "report_incoming"};
		return QStringLiteral("connectionsymbols/") + QLatin1String(names[kind]);
	}
}

QList<ConnectionSymbols::Kind> ConnectionSymbols::kinds()
{
	return {Corner, Tee, Interruption, ReportOutgoing, ReportIncoming};
}

QString ConnectionSymbols::commandId(Kind kind)
{
	return QStringLiteral("diagrameditor.place_") + key(kind).section('/', 1);
}

QString ConnectionSymbols::title(Kind kind)
{
	switch (kind) {
		case Corner:
			return QCoreApplication::translate("ConnectionSymbols", "Corner");
		case Tee:
			return QCoreApplication::translate("ConnectionSymbols", "T-junction");
		case Interruption:
			return QCoreApplication::translate("ConnectionSymbols", "Interruption point");
		case ReportOutgoing:
			return QCoreApplication::translate("ConnectionSymbols", "Outgoing cross-reference");
		case ReportIncoming:
			return QCoreApplication::translate("ConnectionSymbols", "Incoming cross-reference");
	}
	return QString();
}

QString ConnectionSymbols::defaultPath(Kind kind)
{
	switch (kind) {
		case Corner:
			return QStringLiteral("common://10_electric/10_allpole/114_connections/corner.elmt");
		case Tee:
			return QStringLiteral("common://10_electric/10_allpole/114_connections/thruleft.elmt");
		case Interruption:
			return QString();
		case ReportOutgoing:
			return QStringLiteral("common://10_electric/10_allpole/100_folio_referencing/02going_arrow.elmt");
		case ReportIncoming:
			return QStringLiteral("common://10_electric/10_allpole/100_folio_referencing/01coming_arrow.elmt");
	}
	return QString();
}

QString ConnectionSymbols::path(Kind kind)
{
	return QSettings().value(key(kind), defaultPath(kind)).toString();
}

ElementsLocation ConnectionSymbols::location(Kind kind)
{
	const QString p = path(kind);
	return p.isEmpty() ? ElementsLocation() : ElementsLocation(p);
}

void ConnectionSymbols::setPath(Kind kind, const QString &path)
{
	QSettings().setValue(key(kind), path);
}

void ConnectionSymbols::resetPath(Kind kind)
{
	QSettings().remove(key(kind));
}
