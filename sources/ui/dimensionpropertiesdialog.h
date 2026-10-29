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
#ifndef DIMENSIONPROPERTIESDIALOG_H
#define DIMENSIONPROPERTIESDIALOG_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QPointer>
#include "../qetgraphicsitem/dimensionitem.h"

namespace Ui {
	class DimensionPropertiesDialog;
}

class DimensionPropertiesDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DimensionPropertiesDialog(DimensionItem *item, QWidget *parent = nullptr);
	~DimensionPropertiesDialog() override;

	void apply();
	QList<DimensionItem *> selectedSegments() const;

public slots:
	void accept() override;
	void reject() override;

private slots:
	void onSelectionChanged();
	void onSelectAllToggled(bool checked);
	void updateValueDisplay();
	void onApplyClicked();

private:
	void setupPropertyConnections();
	void loadItemProperties(DimensionItem *item);

	Ui::DimensionPropertiesDialog *ui;
	QPointer<DimensionItem> m_primary_item;
	QList<DimensionItem *> m_segments;
	DimensionItem *m_active_item = nullptr;
	QMap<DimensionItem *, DimensionProperties> m_original_props;
	QMap<DimensionItem *, DimensionProperties> m_current_props;
	bool m_updating = false;
	bool m_updating_selection = false;
	bool m_applied = false;
};

#endif // DIMENSIONPROPERTIESDIALOG_H
