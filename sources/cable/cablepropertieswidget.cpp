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
#include "cablepropertieswidget.h"

#include "cabletextformatdialog.h"
#include "../qetapp.h"
#include "../qeticons.h"
#include "../utils/qetutils.h"

#include <QApplication>
#include <QCheckBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>

/**
	@brief CablePropertiesWidget::CablePropertiesWidget
	One row per field: its name, the tick saying whether the drawing
	shows it, the field itself, then the button setting how that one
	text is written -- one last row for the colours of the cores, which
	are written all together along the line, and one row per distance
	he may nudge the whole label and that row of colours with.
	@param parent
*/
CablePropertiesWidget::CablePropertiesWidget(QWidget *parent) :
	QWidget(parent)
{
	auto *grid = new QGridLayout(this);
	grid->setContentsMargins(0, 0, 0, 0);
	grid->setHorizontalSpacing(6);
	grid->setVerticalSpacing(3);

	auto *plan = new QLabel(tr("Drawing",
							   "column header: the drawing shows this field"), this);
	plan->setAlignment(Qt::AlignCenter);
	plan->setToolTip(tr("Tick to write this field on the drawing.",
						"tooltip of the tick before each field"));
	grid->addWidget(plan, 0, 1, Qt::AlignCenter);

	int row = 1;
	addField(grid, row++, QStringLiteral("designation"),
			 tr("Designation"), &m_designation, &m_show_designation);
	const int type_row = row;
	addField(grid, row++, QStringLiteral("type"),
			 tr("Type"), &m_type, &m_show_type);

		//A button beside the type field, opening the very window which
		//opens when a cable line has just been drawn: the type is
		//picked from the cable type file there -- with its search, its
		//filter and its own button for a type the file does not hold
		//yet -- rather than typed in as plain text.
	auto *pick_type = new QPushButton(this);
	pick_type->setFocusPolicy(Qt::NoFocus);
	pick_type->setIcon(QET::Icons::Cable);
	pick_type->setToolTip(tr("Choose the type of this cable from the list of "
							 "cable types…"));
	connect(pick_type, &QPushButton::clicked,
			this, &CablePropertiesWidget::typeChangeRequested);
	grid->addWidget(pick_type, type_row, 4, Qt::AlignRight | Qt::AlignVCenter);

	addField(grid, row++, QStringLiteral("installation"),
			 tr("Plant (=)"), &m_installation, &m_show_installation);
	addField(grid, row++, QStringLiteral("location"),
			 tr("Location (+)"), &m_location, &m_show_location);
	addField(grid, row++, QStringLiteral("length"),
			 tr("Length"), &m_length, &m_show_length);

		//The colours of the cores: one setting for every one of them at
		//once, since they are written in one row along the line and are
		//read as one row.
	auto *cores_label = new QLabel(tr("Core colors"), this);
	cores_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	cores_label->setToolTip(tr("The font of the colors of all the cores "
							   "of this cable, written along the line."));
	grid->addWidget(cores_label, row, 0, Qt::AlignRight | Qt::AlignVCenter);
	addFormatButton(grid, row, QStringLiteral("cores"));

		//Where that whole label stands: one distance along the line and
		//one across it, in pixels, moving the label as one thing --
		//installation, place and designation over the line, type and
		//length under it -- and the line not at all.
	addOffsetRow(grid, row + 1, tr("Text position"),
				 tr("Shifts all the cable text: plant, location "
					"and designation on one side, type and length "
					"on the other. The line, the slashes and the "
					"wiring stay in place. 10 px make one grid cell."),
				 &m_text_offset_x, &m_text_offset_y);

		//And where the row of colours stands from the slashes it names.
	addOffsetRow(grid, row + 2, tr("Core colors position"),
				 tr("Shifts the core colors relative to their slashes. "
					"The slashes, the entries and the wiring stay in place. "
					"10 px make one grid cell."),
				 &m_core_offset_x, &m_core_offset_y);

	grid->setColumnStretch(2, 1);
}

/**
	@brief CablePropertiesWidget::addField
	Build one row of the form.
	@param grid the form being built
	@param row where to put it
	@param key the name this text goes by in the file
	@param name what the field holds
	@param edit receives the line edit
	@param tick receives the tick before it
*/
void CablePropertiesWidget::addField(QGridLayout *grid,
									 int row,
									 const QString &key,
									 const QString &name,
									 QLineEdit **edit,
									 QCheckBox **tick)
{
	auto *label = new QLabel(name, this);
	auto *check = new QCheckBox(this);
	check->setToolTip(tr("Tick to write this field on the drawing.",
						"tooltip of the tick before each field"));
	auto *field = new QLineEdit(this);
	field->setClearButtonEnabled(true);

	connect(field, &QLineEdit::editingFinished,
			this, &CablePropertiesWidget::propertiesEdited);
		//Every character too: what is typed is written onto the cable
		//as it stands, so the drawing follows while he is still typing
		//rather than only once he leaves the field.
	connect(field, &QLineEdit::textChanged,
			this, &CablePropertiesWidget::propertiesTyped);
	connect(check, &QCheckBox::toggled,
			this, &CablePropertiesWidget::propertiesEdited);

	grid->addWidget(label, row, 0, Qt::AlignRight | Qt::AlignVCenter);
	grid->addWidget(check, row, 1, Qt::AlignCenter);
	grid->addWidget(field, row, 2);
	addFormatButton(grid, row, key);

	*edit = field;
	*tick = check;
}

/**
	@brief CablePropertiesWidget::addFormatButton
	The small button standing at the right of a text: it says which
	size that text is written with, and opens the window where that
	size -- and the rest of how the text is written -- is set.
	@param grid the form being built
	@param row where to put it
	@param key the name of the text
	@return the button, so the caller may reach it again
*/
QPushButton *CablePropertiesWidget::addFormatButton(QGridLayout *grid,
													int row,
													const QString &key)
{
	auto *button = new QPushButton(this);
	button->setFocusPolicy(Qt::NoFocus);
	connect(button, &QPushButton::clicked, this,
			[this, key]() {editFormat(key);});

	grid->addWidget(button, row, 3, Qt::AlignRight | Qt::AlignVCenter);
	m_format_buttons.insert(key, button);
	refreshFormatButton(key);
	return button;
}

/**
	@brief CablePropertiesWidget::addOffsetRow
	Build one of the two rows nudging what is drawn: a name at the left
	of the form, then two distances in pixels -- one measured along the
	line, one across it -- the way the cross references of a slave offer
	theirs.

	Both boxes speak of the drawing and nothing else: what he types here
	moves texts, and the line, the slashes and the wires behind them keep
	standing exactly where they were.
	@param grid the form being built
	@param row where to put it
	@param name what that distance moves
	@param tip what it does and does not touch
	@param dx receives the spin box for the distance along the line
	@param dy receives the spin box for the distance across it
*/
void CablePropertiesWidget::addOffsetRow(QGridLayout *grid,
										 int row,
										 const QString &name,
										 const QString &tip,
										 QSpinBox **dx,
										 QSpinBox **dy)
{
	auto *label = new QLabel(name, this);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setToolTip(tip);

	const auto spin = [this, tip]() {
		auto *box = new QSpinBox(this);
		box->setRange(-200, 200);
		box->setSingleStep(1);
		box->setSuffix(QStringLiteral(" px"));
		box->setAlignment(Qt::AlignRight);
		box->setToolTip(tip);
		connect(box, &QSpinBox::valueChanged,
				this, &CablePropertiesWidget::propertiesEdited);
		return box;
	};
	QSpinBox *x_box = spin();
	QSpinBox *y_box = spin();

	auto *along = new QLabel(tr("X :"), this);
	along->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	auto *across = new QLabel(tr("Y :"), this);
	across->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	auto *values = new QHBoxLayout;
	values->setContentsMargins(0, 0, 0, 0);
	values->setSpacing(4);
	values->addWidget(along);
	values->addWidget(x_box);
	values->addWidget(across);
	values->addWidget(y_box);
	values->addStretch(1);

	grid->addWidget(label, row, 0, Qt::AlignRight | Qt::AlignVCenter);
	grid->addLayout(values, row, 2);

	*dx = x_box;
	*dy = y_box;
}

/**
	@brief CablePropertiesWidget::editFormat
	Open the window where one text of the cable is set, and -- when he
	came out of it having really changed something -- write the answer
	back into the form. One window is one change, so one step of undo.
	@param key the name of the text
*/
void CablePropertiesWidget::editFormat(const QString &key)
{
	CableTextFormatDialog dialog(m_formats.value(key), coresKey(key), this);
	if (dialog.exec() != QDialog::Accepted) return;

	const CableTextFormat answer = dialog.format();
	if (m_formats.value(key) == answer) return;

	if (answer == CableTextFormat()) m_formats.remove(key);
	else m_formats.insert(key, answer);

	refreshFormatButton(key);
	emit propertiesEdited();
}

/**
	@brief CablePropertiesWidget::refreshFormatButton
	Let the button of one text say which size that text is written with,
	so that the size is read off the form rather than only in the
	drawing.
	@param key the name of the text
*/
void CablePropertiesWidget::refreshFormatButton(const QString &key)
{
	QPushButton *button = m_format_buttons.value(key);
	if (!button) return;

	const QFont font = effectiveFont(key);
	button->setText(tr("%1 pt").arg(QString::number(font.pointSizeF(), 'g', 3)));

	const QString tip = coresKey(key)
			? tr("Font, size and style of the core colors.")
			: tr("Font, size, style and alignment of this text.");
	const CableTextFormat format = m_formats.value(key);
	const bool its_own = !format.font.isEmpty() || format.alignment != 0;
	button->setToolTip(tip + QLatin1Char(' ') + (its_own
			? tr("Set for this cable.")
			: tr("Value defined in the preferences.")));
}

/**
	@brief CablePropertiesWidget::effectiveFont
	@param key the name of a text of the form
	@return the font that text really is written with on the drawing:
	the one this cable asks for, or -- when it asks for none -- the one
	the preferences offer
*/
QFont CablePropertiesWidget::effectiveFont(const QString &key) const
{
	const CableTextFormat format = m_formats.value(key);
	if (!format.font.isEmpty())
	{
		QFont font;
		if (QETUtils::fontFromString(font, format.font)) {
			return font;
		}
	}
	return coresKey(key) ? QETApp::cableCoreFont() : QETApp::cableTextsFont();
}

/**
	@brief CablePropertiesWidget::properties
	@return everything the form holds right now
*/
CableProperties CablePropertiesWidget::properties() const
{
	CableProperties properties;
	properties.designation = m_designation->text().trimmed();
		//A name which is not the one read from the cable can only have
		//come from him typing it in: from that moment on the cable
		//carries a name of his own rather than one the numbering rule
		//handed out. Putting the very same name back by hand keeps the
		//cable as it stood, which is what the field says.
	properties.designation_by_hand = properties.designation == m_designation_text
			? m_designation_by_hand : true;
	properties.type = m_type->text().trimmed();
	properties.installation = m_installation->text().trimmed();
	properties.location = m_location->text().trimmed();
	properties.length = m_length->text().trimmed();
	properties.show_designation = m_show_designation->isChecked();
	properties.show_type = m_show_type->isChecked();
	properties.show_installation = m_show_installation->isChecked();
	properties.show_location = m_show_location->isChecked();
	properties.show_length = m_show_length->isChecked();
	properties.texts = m_formats;
	properties.text_offset = QPointF(m_text_offset_x->value(),
									 m_text_offset_y->value());
	properties.core_offset = QPointF(m_core_offset_x->value(),
									 m_core_offset_y->value());
	return properties;
}

/**
	@brief CablePropertiesWidget::setProperties
	Fill the form in. The ticks say what is drawn, so a field which is
	switched off still keeps the text standing in it -- hiding it on the
	drawing never throws away what the user typed.
	@param properties
*/
void CablePropertiesWidget::setProperties(const CableProperties &properties)
{
	m_designation->setText(properties.designation);
		//What the field holds now is what the cable said: anything he
		//types from here on is a name of his own
	m_designation_text = properties.designation;
	m_designation_by_hand = properties.designation_by_hand;
	m_type->setText(properties.type);
	m_installation->setText(properties.installation);
	m_location->setText(properties.location);
	m_length->setText(properties.length);
	m_show_designation->setChecked(properties.show_designation);
	m_show_type->setChecked(properties.show_type);
	m_show_installation->setChecked(properties.show_installation);
	m_show_location->setChecked(properties.show_location);
	m_show_length->setChecked(properties.show_length);

		//Filling the form in is not the user nudging anything: those
		//two rows are muted while they are set, so that loading a cable
		//can never look like an edit which then has to be undone.
	const QSignalBlocker block_text_x(m_text_offset_x);
	const QSignalBlocker block_text_y(m_text_offset_y);
	const QSignalBlocker block_core_x(m_core_offset_x);
	const QSignalBlocker block_core_y(m_core_offset_y);
	m_text_offset_x->setValue(qRound(properties.text_offset.x()));
	m_text_offset_y->setValue(qRound(properties.text_offset.y()));
	m_core_offset_x->setValue(qRound(properties.core_offset.x()));
	m_core_offset_y->setValue(qRound(properties.core_offset.y()));

		//The texts this cable writes its own way. Only what really has
		//an answer of its own is kept, so that a stray empty entry can
		//never make the form claim a change it never made.
	m_formats.clear();
	for (auto it = properties.texts.cbegin(); it != properties.texts.cend(); ++it)
	{
		if (it.value() == CableTextFormat()) continue;
		m_formats.insert(it.key(), it.value());
	}
	for (auto it = m_format_buttons.cbegin(); it != m_format_buttons.cend(); ++it) {
		refreshFormatButton(it.key());
	}
}

/**
	@brief CablePropertiesWidget::editing
	@return true when the keyboard stands in one of the fields of this
	form. The panel uses it to keep its hands off what is being typed:
	writing the cable back into the form while he is typing would take
	the cursor away from him with every key.
*/
bool CablePropertiesWidget::editing() const
{
	QWidget *focus = QApplication::focusWidget();
	return focus && (focus == this || isAncestorOf(focus));
}
