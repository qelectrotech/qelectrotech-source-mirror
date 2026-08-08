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
#ifndef CUSTOMELEMENTINFOPARTWIDGET_H
#define CUSTOMELEMENTINFOPARTWIDGET_H

#include <QWidget>

class QLineEdit;
class QToolButton;

/**
	@brief The CustomElementInfoPartWidget class
	A single row letting the user define their own element information
	key/value pair, unlike ElementInfoPartWidget which is bound to one
	predefined key. The key is validated against
	DiagramContext::isKeyAcceptable() as the user types.
*/
class CustomElementInfoPartWidget : public QWidget
{
		Q_OBJECT

	public:
		explicit CustomElementInfoPartWidget(
				const QString &key = QString(),
				const QString &value = QString(),
				QWidget *parent = nullptr);
		~CustomElementInfoPartWidget() override;

		QString key() const;
		QString value() const;
		bool hasValidKey() const;

	signals:
		void changed();
		void removeRequested(CustomElementInfoPartWidget *self);

	private slots:
		void validateKey();

	private:
		QLineEdit   *m_key_edit;
		QLineEdit   *m_value_edit;
		QToolButton *m_remove_button;
};

#endif // CUSTOMELEMENTINFOPARTWIDGET_H
