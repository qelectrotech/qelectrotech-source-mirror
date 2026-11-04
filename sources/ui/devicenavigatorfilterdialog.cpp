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
#include "devicenavigatorfilterdialog.h"
#include "ui_devicenavigatorfilterdialog.h"

#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>

/**
 * @brief DeviceNavigatorFilterScheme::matches
 */
bool DeviceNavigatorFilterScheme::matches(const QString &bmk,
										  const QString &structure,
										  bool isMaster,
										  bool isSlave,
										  bool isTerminal,
										  bool isCable,
										  bool isReport,
										  bool isConductorDef,
										  bool isThumbnail) const
{
	// 1. Element Type filtering
	if (isReport && !showReports) return false;
	if (isConductorDef && !showConductorDefs) return false;
	if (isMaster && !showMasters) return false;
	if (isSlave && !showSlaves) return false;
	if (isTerminal && !showTerminals) return false;
	if (isCable && !showCables) return false;
	if (isThumbnail && !showThumbnails) return false;
	if (!isMaster && !isSlave && !isTerminal && !isCable && !isReport && !isConductorDef && !isThumbnail && !showOthers) return false;

	// 2. Structure Mode filtering
	const QString noStruct = QObject::tr("No structure identifier");
	if (structureMode == 1) { // With structure only
		if (structure == noStruct) return false;
	} else if (structureMode == 2) { // Without structure only
		if (structure != noStruct) return false;
	}

	// 3. Structure text pattern
	if (!structurePattern.trimmed().isEmpty()) {
		if (!structure.contains(structurePattern.trimmed(), Qt::CaseInsensitive)) {
			return false;
		}
	}

	// 4. BMK Letters / Prefixes (e.g. "K, Q, F")
	QString cleanedBmk = bmk.trimmed();
	if (cleanedBmk.startsWith(QLatin1Char('-'))) {
		cleanedBmk.remove(0, 1);
	}

	QStringList prefixes = bmkLetters.split(QLatin1Char(','), Qt::SkipEmptyParts);
	if (!prefixes.isEmpty()) {
		bool prefixMatch = false;
		for (QString pref : prefixes) {
			pref = pref.trimmed();
			if (pref.startsWith(QLatin1Char('-'))) {
				pref.remove(0, 1);
			}
			if (!pref.isEmpty() && cleanedBmk.startsWith(pref, Qt::CaseInsensitive)) {
				prefixMatch = true;
				break;
			}
		}
		if (!prefixMatch) return false;
	}

	// 5. Custom BMK Wildcard / Pattern (e.g. *Pump* or -K[1-9])
	QString pat = bmkPattern.trimmed();
	if (!pat.isEmpty()) {
		QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pat),
								 QRegularExpression::CaseInsensitiveOption);
		if (!regex.match(bmk).hasMatch() && !regex.match(cleanedBmk).hasMatch()) {
			return false;
		}
	}

	return true;
}

/**
 * @brief DeviceNavigatorFilterScheme::defaultSchemes
 */
QList<DeviceNavigatorFilterScheme> DeviceNavigatorFilterScheme::defaultSchemes()
{
	QList<DeviceNavigatorFilterScheme> list;

	// 0: Not active (shows all real devices, cables, terminals; reports & conductor defs hidden)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("- Not active -");
		list.append(s);
	}
	// 1: Contactors / Relays (-K)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Contactors / Relays (-K)");
		s.bmkLetters = QStringLiteral("K");
		list.append(s);
	}
	// 2: Protection / Fuses (-F)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Protection / Fuses (-F)");
		s.bmkLetters = QStringLiteral("F");
		list.append(s);
	}
	// 3: Power Switches (-Q)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Power Switches (-Q)");
		s.bmkLetters = QStringLiteral("Q");
		list.append(s);
	}
	// 4: Terminals (-X)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Terminals (-X)");
		s.bmkLetters = QStringLiteral("X");
		s.showMasters = false;
		s.showSlaves = false;
		s.showCables = false;
		s.showTerminals = true;
		list.append(s);
	}
	// 5: Cables (-W)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Cables (-W)");
		s.bmkLetters = QStringLiteral("W");
		s.showMasters = false;
		s.showSlaves = false;
		s.showTerminals = false;
		s.showCables = true;
		list.append(s);
	}
	// 6: Motors / Drives (-M)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Motors / Drives (-M)");
		s.bmkLetters = QStringLiteral("M");
		list.append(s);
	}
	// 7: Sensors / Transducers (-B)
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("Sensors / Transducers (-B)");
		s.bmkLetters = QStringLiteral("B");
		list.append(s);
	}
	// 8: No structure identifier
	{
		DeviceNavigatorFilterScheme s;
		s.name = QObject::tr("No structure identifier");
		s.structureMode = 2; // Without structure only
		list.append(s);
	}

	return list;
}

/**
 * @brief DeviceNavigatorFilterScheme::loadSchemes
 */
QList<DeviceNavigatorFilterScheme> DeviceNavigatorFilterScheme::loadSchemes()
{
	QSettings settings;
	int size = settings.beginReadArray(QStringLiteral("devicenavigator/filters"));
	if (size <= 0) {
		settings.endArray();
		return defaultSchemes();
	}

	QList<DeviceNavigatorFilterScheme> list;
	for (int i = 0; i < size; ++i) {
		settings.setArrayIndex(i);
		DeviceNavigatorFilterScheme s;
		s.name = settings.value(QStringLiteral("name")).toString();
		s.active = settings.value(QStringLiteral("active"), true).toBool();
		s.bmkLetters = settings.value(QStringLiteral("bmkLetters")).toString();
		s.bmkPattern = settings.value(QStringLiteral("bmkPattern")).toString();
		s.showMasters = settings.value(QStringLiteral("showMasters"), true).toBool();
		s.showSlaves = settings.value(QStringLiteral("showSlaves"), true).toBool();
		s.showTerminals = settings.value(QStringLiteral("showTerminals"), true).toBool();
		s.showCables = settings.value(QStringLiteral("showCables"), true).toBool();
		s.showReports = settings.value(QStringLiteral("showReports"), false).toBool();
		s.showConductorDefs = settings.value(QStringLiteral("showConductorDefs"), false).toBool();
		s.showThumbnails = settings.value(QStringLiteral("showThumbnails"), true).toBool();
		s.showOthers = settings.value(QStringLiteral("showOthers"), true).toBool();
		s.structureMode = settings.value(QStringLiteral("structureMode"), 0).toInt();
		s.structurePattern = settings.value(QStringLiteral("structurePattern")).toString();

		if (!s.name.isEmpty()) {
			list.append(s);
		}
	}
	settings.endArray();

	if (list.isEmpty()) {
		list = defaultSchemes();
	}
	return list;
}

/**
 * @brief DeviceNavigatorFilterScheme::saveSchemes
 */
void DeviceNavigatorFilterScheme::saveSchemes(const QList<DeviceNavigatorFilterScheme> &schemes)
{
	QSettings settings;
	settings.beginWriteArray(QStringLiteral("devicenavigator/filters"), schemes.size());
	for (int i = 0; i < schemes.size(); ++i) {
		settings.setArrayIndex(i);
		const DeviceNavigatorFilterScheme &s = schemes.at(i);
		settings.setValue(QStringLiteral("name"), s.name);
		settings.setValue(QStringLiteral("active"), s.active);
		settings.setValue(QStringLiteral("bmkLetters"), s.bmkLetters);
		settings.setValue(QStringLiteral("bmkPattern"), s.bmkPattern);
		settings.setValue(QStringLiteral("showMasters"), s.showMasters);
		settings.setValue(QStringLiteral("showSlaves"), s.showSlaves);
		settings.setValue(QStringLiteral("showTerminals"), s.showTerminals);
		settings.setValue(QStringLiteral("showCables"), s.showCables);
		settings.setValue(QStringLiteral("showReports"), s.showReports);
		settings.setValue(QStringLiteral("showConductorDefs"), s.showConductorDefs);
		settings.setValue(QStringLiteral("showThumbnails"), s.showThumbnails);
		settings.setValue(QStringLiteral("showOthers"), s.showOthers);
		settings.setValue(QStringLiteral("structureMode"), s.structureMode);
		settings.setValue(QStringLiteral("structurePattern"), s.structurePattern);
	}
	settings.endArray();
}

/**
 * @brief DeviceNavigatorFilterDialog::DeviceNavigatorFilterDialog
 */
DeviceNavigatorFilterDialog::DeviceNavigatorFilterDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DeviceNavigatorFilterDialog)
{
	ui->setupUi(this);

	ui->m_struct_mode_combo->addItem(tr("All structures"));
	ui->m_struct_mode_combo->addItem(tr("With structure identifier only"));
	ui->m_struct_mode_combo->addItem(tr("Without structure identifier only"));

	connect(ui->m_scheme_combo, qOverload<int>(&QComboBox::currentIndexChanged),
			this, &DeviceNavigatorFilterDialog::onSchemeChanged);
	connect(ui->m_new_scheme_btn, &QPushButton::clicked,
			this, &DeviceNavigatorFilterDialog::onNewSchemeClicked);
	connect(ui->m_delete_scheme_btn, &QPushButton::clicked,
			this, &DeviceNavigatorFilterDialog::onDeleteSchemeClicked);

	QPushButton *resetBtn = ui->buttonBox->button(QDialogButtonBox::Reset);
	if (resetBtn) {
		connect(resetBtn, &QPushButton::clicked,
				this, &DeviceNavigatorFilterDialog::onResetClicked);
	}
	QPushButton *applyBtn = ui->buttonBox->button(QDialogButtonBox::Apply);
	if (applyBtn) {
		connect(applyBtn, &QPushButton::clicked,
				this, &DeviceNavigatorFilterDialog::onApplyClicked);
	}

	// Quick select checkboxes
	connect(ui->m_cb_prefix_k, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_f, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_q, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_x, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_w, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_m, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_b, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
	connect(ui->m_cb_prefix_p, &QCheckBox::toggled, this, &DeviceNavigatorFilterDialog::onPrefixCheckboxToggled);
}

DeviceNavigatorFilterDialog::~DeviceNavigatorFilterDialog()
{
	delete ui;
}

void DeviceNavigatorFilterDialog::setSchemes(const QList<DeviceNavigatorFilterScheme> &schemes, int currentIndex)
{
	m_schemes = schemes.isEmpty() ? DeviceNavigatorFilterScheme::defaultSchemes() : schemes;
	m_current_index = qBound(0, currentIndex, m_schemes.size() - 1);

	ui->m_scheme_combo->blockSignals(true);
	ui->m_scheme_combo->clear();
	for (const auto &s : m_schemes) {
		ui->m_scheme_combo->addItem(s.name);
	}
	ui->m_scheme_combo->setCurrentIndex(m_current_index);
	ui->m_scheme_combo->blockSignals(false);

	loadSchemeToUi(m_schemes.at(m_current_index));
}

QList<DeviceNavigatorFilterScheme> DeviceNavigatorFilterDialog::schemes() const
{
	return m_schemes;
}

int DeviceNavigatorFilterDialog::currentSchemeIndex() const
{
	return m_current_index;
}

void DeviceNavigatorFilterDialog::onSchemeChanged(int index)
{
	if (index < 0 || index >= m_schemes.size()) return;

	// Save previous scheme
	if (m_current_index >= 0 && m_current_index < m_schemes.size()) {
		m_schemes[m_current_index] = saveSchemeFromUi();
	}

	m_current_index = index;
	loadSchemeToUi(m_schemes.at(m_current_index));
}

void DeviceNavigatorFilterDialog::onNewSchemeClicked()
{
	bool ok = false;
	QString name = QInputDialog::getText(this, tr("New Scheme"),
										 tr("Enter name for new filter scheme:"),
										 QLineEdit::Normal, QString(), &ok);
	if (!ok || name.trimmed().isEmpty()) return;

	DeviceNavigatorFilterScheme s = saveSchemeFromUi();
	s.name = name.trimmed();
	m_schemes.append(s);

	ui->m_scheme_combo->addItem(s.name);
	ui->m_scheme_combo->setCurrentIndex(m_schemes.size() - 1);
}

void DeviceNavigatorFilterDialog::onDeleteSchemeClicked()
{
	if (m_current_index <= 0) {
		QMessageBox::information(this, tr("Cannot Delete"),
								 tr("The default scheme cannot be deleted."));
		return;
	}

	auto reply = QMessageBox::question(this, tr("Delete Scheme"),
									   tr("Are you sure you want to delete scheme '%1'?")
									   .arg(m_schemes.at(m_current_index).name),
									   QMessageBox::Yes | QMessageBox::No);
	if (reply != QMessageBox::Yes) return;

	m_schemes.removeAt(m_current_index);
	ui->m_scheme_combo->removeItem(m_current_index);
}

void DeviceNavigatorFilterDialog::onResetClicked()
{
	auto reply = QMessageBox::question(this, tr("Reset Schemes"),
									   tr("Reset all filter schemes to defaults?"),
									   QMessageBox::Yes | QMessageBox::No);
	if (reply != QMessageBox::Yes) return;

	setSchemes(DeviceNavigatorFilterScheme::defaultSchemes(), 0);
}

void DeviceNavigatorFilterDialog::onApplyClicked()
{
	if (m_current_index >= 0 && m_current_index < m_schemes.size()) {
		m_schemes[m_current_index] = saveSchemeFromUi();
	}
	DeviceNavigatorFilterScheme::saveSchemes(m_schemes);
}

void DeviceNavigatorFilterDialog::loadSchemeToUi(const DeviceNavigatorFilterScheme &s)
{
	m_updating_ui = true;

	ui->m_bmk_letters_edit->setText(s.bmkLetters);
	ui->m_bmk_pattern_edit->setText(s.bmkPattern);

	ui->m_cb_master->setChecked(s.showMasters);
	ui->m_cb_slave->setChecked(s.showSlaves);
	ui->m_cb_terminal->setChecked(s.showTerminals);
	ui->m_cb_cable->setChecked(s.showCables);
	ui->m_cb_report->setChecked(s.showReports);
	ui->m_cb_conductor_def->setChecked(s.showConductorDefs);
	ui->m_cb_thumbnail->setChecked(s.showThumbnails);
	ui->m_cb_other->setChecked(s.showOthers);

	ui->m_struct_mode_combo->setCurrentIndex(qBound(0, s.structureMode, 2));
	ui->m_struct_pattern_edit->setText(s.structurePattern);

	updateCheckboxesFromBmkLetters();

	m_updating_ui = false;
}

DeviceNavigatorFilterScheme DeviceNavigatorFilterDialog::saveSchemeFromUi() const
{
	DeviceNavigatorFilterScheme s;
	if (m_current_index >= 0 && m_current_index < m_schemes.size()) {
		s.name = m_schemes.at(m_current_index).name;
	} else {
		s.name = tr("Custom");
	}

	s.bmkLetters = ui->m_bmk_letters_edit->text().trimmed();
	s.bmkPattern = ui->m_bmk_pattern_edit->text().trimmed();

	s.showMasters = ui->m_cb_master->isChecked();
	s.showSlaves = ui->m_cb_slave->isChecked();
	s.showTerminals = ui->m_cb_terminal->isChecked();
	s.showCables = ui->m_cb_cable->isChecked();
	s.showReports = ui->m_cb_report->isChecked();
	s.showConductorDefs = ui->m_cb_conductor_def->isChecked();
	s.showThumbnails = ui->m_cb_thumbnail->isChecked();
	s.showOthers = ui->m_cb_other->isChecked();

	s.structureMode = ui->m_struct_mode_combo->currentIndex();
	s.structurePattern = ui->m_struct_pattern_edit->text().trimmed();

	return s;
}

void DeviceNavigatorFilterDialog::onPrefixCheckboxToggled()
{
	if (m_updating_ui) return;
	updateBmkLettersFromCheckboxes();
}

void DeviceNavigatorFilterDialog::updateBmkLettersFromCheckboxes()
{
	QStringList letters;
	if (ui->m_cb_prefix_k->isChecked()) letters << QStringLiteral("K");
	if (ui->m_cb_prefix_f->isChecked()) letters << QStringLiteral("F");
	if (ui->m_cb_prefix_q->isChecked()) letters << QStringLiteral("Q");
	if (ui->m_cb_prefix_x->isChecked()) letters << QStringLiteral("X");
	if (ui->m_cb_prefix_w->isChecked()) letters << QStringLiteral("W");
	if (ui->m_cb_prefix_m->isChecked()) letters << QStringLiteral("M");
	if (ui->m_cb_prefix_b->isChecked()) letters << QStringLiteral("B");
	if (ui->m_cb_prefix_p->isChecked()) letters << QStringLiteral("P, H");

	ui->m_bmk_letters_edit->setText(letters.join(QStringLiteral(", ")));
}

void DeviceNavigatorFilterDialog::updateCheckboxesFromBmkLetters()
{
	const QString text = ui->m_bmk_letters_edit->text().toUpper();
	QStringList tokens = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
	QSet<QString> set;
	for (auto t : tokens) {
		t = t.trimmed();
		if (t.startsWith(QLatin1Char('-'))) t.remove(0, 1);
		if (!t.isEmpty()) set.insert(t);
	}

	ui->m_cb_prefix_k->setChecked(set.contains(QStringLiteral("K")));
	ui->m_cb_prefix_f->setChecked(set.contains(QStringLiteral("F")));
	ui->m_cb_prefix_q->setChecked(set.contains(QStringLiteral("Q")));
	ui->m_cb_prefix_x->setChecked(set.contains(QStringLiteral("X")));
	ui->m_cb_prefix_w->setChecked(set.contains(QStringLiteral("W")));
	ui->m_cb_prefix_m->setChecked(set.contains(QStringLiteral("M")));
	ui->m_cb_prefix_b->setChecked(set.contains(QStringLiteral("B")));
	ui->m_cb_prefix_p->setChecked(set.contains(QStringLiteral("P")) || set.contains(QStringLiteral("H")));
}

