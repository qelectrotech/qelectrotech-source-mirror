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
#ifndef DEVICENAVIGATORFILTERDIALOG_H
#define DEVICENAVIGATORFILTERDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QList>

namespace Ui {
	class DeviceNavigatorFilterDialog;
}

/**
 * @brief The DeviceNavigatorFilterScheme struct
 * Represents a user-configurable filter scheme for the Device Navigator.
 */
struct DeviceNavigatorFilterScheme {
	QString name;
	bool active = true;
	QString bmkLetters;
	QString bmkPattern;
	bool showMasters = true;
	bool showSlaves = true;
	bool showTerminals = true;
	bool showCables = true;
	bool showReports = false;
	bool showConductorDefs = false;
	bool showThumbnails = true;
	bool showOthers = true;
	int structureMode = 0; // 0 = All, 1 = With structure, 2 = Without structure
	QString structurePattern;

	bool matches(const QString &bmk,
				 const QString &structure,
				 bool isMaster,
				 bool isSlave,
				 bool isTerminal,
				 bool isCable,
				 bool isReport,
				 bool isConductorDef,
				 bool isThumbnail) const;

	static QList<DeviceNavigatorFilterScheme> defaultSchemes();
	static QList<DeviceNavigatorFilterScheme> loadSchemes();
	static void saveSchemes(const QList<DeviceNavigatorFilterScheme> &schemes);
};

/**
 * @brief The DeviceNavigatorFilterDialog class
 * EPLAN-style filter scheme configuration dialog for the Device Navigator.
 */
class DeviceNavigatorFilterDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit DeviceNavigatorFilterDialog(QWidget *parent = nullptr);
		~DeviceNavigatorFilterDialog() override;

		void setSchemes(const QList<DeviceNavigatorFilterScheme> &schemes, int currentIndex = 0);
		QList<DeviceNavigatorFilterScheme> schemes() const;
		int currentSchemeIndex() const;

	private slots:
		void onSchemeChanged(int index);
		void onNewSchemeClicked();
		void onDeleteSchemeClicked();
		void onResetClicked();
		void onApplyClicked();
		void onPrefixCheckboxToggled();

	private:
		void loadSchemeToUi(const DeviceNavigatorFilterScheme &scheme);
		DeviceNavigatorFilterScheme saveSchemeFromUi() const;
		void updateBmkLettersFromCheckboxes();
		void updateCheckboxesFromBmkLetters();

	private:
		Ui::DeviceNavigatorFilterDialog *ui = nullptr;
		QList<DeviceNavigatorFilterScheme> m_schemes;
		int m_current_index = 0;
		bool m_updating_ui = false;
};

#endif // DEVICENAVIGATORFILTERDIALOG_H

