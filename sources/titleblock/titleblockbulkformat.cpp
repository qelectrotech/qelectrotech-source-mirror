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
#include "titleblockbulkformat.h"

#include <titleblockcell.h>
#include <titleblocktemplate.h>
#include "templatecommands.h"
#include "templateview.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>

/**
	@brief TitleBlockBulkFormat::mergedFont
	@param current : QFont description the text has now, may be empty
	@param fallback : font to start from when \a current is empty
	@return the QFont description of the text once the checked
	font properties are applied
*/
QString TitleBlockBulkFormat::mergedFont(const QString &current,
					 const QFont &fallback) const
{
	const int font_properties = Family | Size | Bold | Italic;
	if (!(properties & font_properties)) {
		return current;
	}

	QFont result = fallback;
	if (!current.isEmpty()) {
		result.fromString(current);
	}
	if (properties & Family) result.setFamily(font.family());
	if (properties & Size) {
		if (font.pointSizeF() > 0) {
			result.setPointSizeF(font.pointSizeF());
		} else if (font.pixelSize() > 0) {
			result.setPixelSize(font.pixelSize());
		}
	}
	if (properties & Bold) result.setBold(font.bold());
	if (properties & Italic) result.setItalic(font.italic());
	return result.toString();
}

/**
	@brief TitleBlockBulkFormat::mergedColor
	@param current : colour the text has now, invalid for the default colour
	@return the colour of the text once the checked colour properties are
	applied. Changing only the colour keeps the opacity, changing only the
	opacity keeps the colour.
*/
QColor TitleBlockBulkFormat::mergedColor(const QColor &current) const
{
	if (!(properties & (Color | Opacity))) {
		return current;
	}

	QColor result = current.isValid() ? current : QColor(Qt::black);
	if (properties & Color) {
		const int alpha = result.alpha();
		result = color;
		result.setAlpha(alpha);
	}
	if (properties & Opacity) {
		result.setAlphaF(opacity / 100.0);
	}
	return result;
}

/**
	@brief TitleBlockBulkFormat::createCommand
	@param cells : cells to format. Cells that are not text cells and cells
	covered by another cell are ignored.
	@param view : view to refresh when the command is done or undone
	@return a command applying this format to \a cells, or nullptr when
	it would change nothing. The caller takes ownership.
*/
QUndoCommand *TitleBlockBulkFormat::createCommand(
		const QList<TitleBlockCell *> &cells,
		TitleBlockTemplateView *view) const
{
	if (!properties) return nullptr;

	auto *command = new QUndoCommand(
		QObject::tr("Format title block cells"));
	QSet<TitleBlockCell *> done;
	for (TitleBlockCell *cell : cells) {
		if (!cell || done.contains(cell)) continue;
		done.insert(cell);
		if (cell->spanner_cell
		    || cell->cell_type != TitleBlockCell::TextCell) continue;

		const QFont cell_font = TitleBlockTemplate::fontForCell(*cell);
		QHash<QString, QVariant> changes;
		if (part != ValueOnly) {
			changes["label_font"] = mergedFont(cell->label_font, cell_font);
			changes["label_color"] = mergedColor(cell->label_color);
		}
		if (part != LabelOnly) {
			changes["value_font"] = mergedFont(cell->value_font, cell_font);
			changes["value_color"] = mergedColor(cell->value_color);
		}
		if (properties & Alignment) {
			changes["alignment"] = (cell->alignment & ~int(Qt::AlignHorizontal_Mask))
				| alignment;
		}

		// only what really changes goes in the undo command
		QUndoCommand *change = nullptr;
		for (auto it = changes.cbegin(); it != changes.cend(); ++it) {
			if (cell->attribute(it.key()) == it.value()) continue;
			if (!change) {
				auto *modify = new ModifyTitleBlockCellCommand(cell, command);
				modify->setView(view);
				change = modify;
			}
			static_cast<ModifyTitleBlockCellCommand *>(change)
				->addModification(it.key(), it.value());
		}
	}

	if (!command->childCount()) {
		delete command;
		return nullptr;
	}
	return command;
}

/**
	@brief TitleBlockBulkFormat::getFromUser
	Ask the user what to change.
	@param format : filled with the answer when the dialog is accepted
	@param has_selection : true if some cells are selected; the dialog then
	offers them as the default target
	@param parent : parent widget of the dialog
	@return true if the user accepted with at least one property checked
*/
bool TitleBlockBulkFormat::getFromUser(TitleBlockBulkFormat &format,
				       bool has_selection,
				       QWidget *parent)
{
	QDialog dialog(parent);
	dialog.setWindowTitle(QObject::tr("Format title block cells"));
	auto *layout = new QFormLayout(&dialog);

	auto *note = new QLabel(QObject::tr(
		"Only the checked properties are changed. "
		"Alignment applies to the whole cell."));
	note->setWordWrap(true);
	layout->addRow(note);

	auto *scope = new QComboBox;
	scope->addItem(QObject::tr("Selected cells"));
	scope->addItem(QObject::tr("All text cells"));
	if (!has_selection) scope->setCurrentIndex(1);
	layout->addRow(QObject::tr("Apply to"), scope);

	auto *part = new QComboBox;
	part->addItem(QObject::tr("Captions and values"), LabelAndValue);
	part->addItem(QObject::tr("Captions only"), LabelOnly);
	part->addItem(QObject::tr("Values only"), ValueOnly);
	layout->addRow(QObject::tr("Text"), part);

	QHash<int, QCheckBox *> checks;
	auto add_check = [&](Property property, const QString &label,
			     QWidget *control = nullptr) {
		auto *check = new QCheckBox(label);
		checks.insert(property, check);
		if (control) {
			layout->addRow(check, control);
		} else {
			layout->addRow(check);
		}
	};
	auto add_style_choice = [&](const QString &label) {
		auto *choice = new QComboBox;
		choice->addItem(QObject::tr("Unchanged"), -1);
		choice->addItem(QObject::tr("On"), 1);
		choice->addItem(QObject::tr("Off"), 0);
		layout->addRow(label, choice);
		return choice;
	};

	// choosing a font or a colour also checks what it provides
	QFont font = QApplication::font();
	auto *font_button = new QPushButton(QObject::tr("Choose font..."));
	layout->addRow(font_button);
	add_check(Family, QObject::tr("Font family"));
	add_check(Size, QObject::tr("Font size"));
	auto *bold = add_style_choice(QObject::tr("Bold"));
	auto *italic = add_style_choice(QObject::tr("Italic"));
	QObject::connect(font_button, &QPushButton::clicked, &dialog, [&]() {
		bool ok = false;
		const QFont chosen = QFontDialog::getFont(&ok, font, &dialog);
		if (!ok) return;
		font = chosen;
		font_button->setText(QStringLiteral("%1 %2")
			.arg(font.family()).arg(font.pointSizeF()));
		checks[Family]->setChecked(true);
		checks[Size]->setChecked(true);
		bold->setCurrentIndex(bold->findData(font.bold() ? 1 : 0));
		italic->setCurrentIndex(italic->findData(font.italic() ? 1 : 0));
	});

	QColor color = Qt::black;
	auto *color_button = new QPushButton(QObject::tr("Choose colour..."));
	layout->addRow(color_button);
	add_check(Color, QObject::tr("Text colour"));
	QObject::connect(color_button, &QPushButton::clicked, &dialog, [&]() {
		const QColor chosen = QColorDialog::getColor(color, &dialog);
		if (!chosen.isValid()) return;
		color = chosen;
		color_button->setText(color.name());
		checks[Color]->setChecked(true);
	});

	auto *opacity = new QSpinBox;
	opacity->setRange(0, 100);
	opacity->setValue(100);
	opacity->setSuffix(QStringLiteral(" %"));
	add_check(Opacity, QObject::tr("Opacity"), opacity);

	auto *alignment = new QComboBox;
	alignment->addItem(QObject::tr("Left"), int(Qt::AlignLeft));
	alignment->addItem(QObject::tr("Centre"), int(Qt::AlignHCenter));
	alignment->addItem(QObject::tr("Right"), int(Qt::AlignRight));
	add_check(Alignment, QObject::tr("Horizontal alignment"), alignment);

	auto *buttons = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	layout->addRow(buttons);
	QObject::connect(buttons, &QDialogButtonBox::accepted,
			 &dialog, &QDialog::accept);
	QObject::connect(buttons, &QDialogButtonBox::rejected,
			 &dialog, &QDialog::reject);

	if (dialog.exec() != QDialog::Accepted) return false;

	format.properties = 0;
	for (auto it = checks.cbegin(); it != checks.cend(); ++it) {
		if (it.value()->isChecked()) format.properties |= it.key();
	}
	if (bold->currentData().toInt() >= 0) {
		format.properties |= Bold;
		font.setBold(bold->currentData().toInt() == 1);
	}
	if (italic->currentData().toInt() >= 0) {
		format.properties |= Italic;
		font.setItalic(italic->currentData().toInt() == 1);
	}
	if (!format.properties) return false;
	format.all_cells = scope->currentIndex() == 1;
	format.part = Part(part->currentData().toInt());
	format.font = font;
	format.color = color;
	format.opacity = opacity->value();
	format.alignment = alignment->currentData().toInt();
	return true;
}
