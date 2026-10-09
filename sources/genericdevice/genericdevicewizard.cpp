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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "genericdevicewizard.h"

#include "../editor/terminalnamecheck.h"
#include "../factory/elementpicturefactory.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPicture>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

using namespace GenericDevice;

namespace {

	/// After terminals were added: renumber every pin if the names were
	/// still the automatic ones, otherwise only name the new, unnamed ones.
void nameNewTerminals(Spec &spec, bool were_numbered)
{
	if (were_numbered)
		numberPins(spec);
	else
		nameUnnamed(spec);
}

} // namespace

/**
	@brief The GenericDevicePreview class
	The box as it will be placed, drawn by ElementPictureFactory like any
	placed element, with its label and reference, size and warnings.
*/
class GenericDevicePreview : public QWidget
{
	public:
		explicit GenericDevicePreview(QWidget *parent = nullptr) :
			QWidget(parent)
		{
			auto *layout = new QVBoxLayout(this);
			layout->setContentsMargins(0, 0, 0, 0);
			m_canvas = new Canvas(this);
			m_size = new QLabel(this);
			m_size->setAlignment(Qt::AlignHCenter);
			m_warnings = new QLabel(this);
			m_warnings->setWordWrap(true);
			layout->addWidget(m_canvas, 1);
			layout->addWidget(m_size);
			layout->addWidget(m_warnings);
			setMinimumWidth(280);
		}

		void setSpec(const Spec &spec, const Fonts &fonts)
		{
			QDomDocument document;
			const QDomElement definition = toDefinition(spec, fonts, document);
			m_canvas->picture = ElementPictureFactory::instance()->pictureFromDefinition(definition);
			m_canvas->layout = GenericDevice::layout(spec, fonts);
			m_canvas->label = spec.label.trimmed();
			m_canvas->reference = spec.show_reference ? spec.reference.trimmed() : QString();
			m_canvas->fonts = fonts;
			m_canvas->update();

			const Layout &l = m_canvas->layout;
			m_size->setText(QCoreApplication::translate("GenericDeviceWizard", "Box: %1 × %2 grid squares")
					.arg(l.body_width / Grid).arg(l.body_height / Grid));
			m_warnings->setText(l.warnings.join(QLatin1Char('\n')));
			m_warnings->setVisible(!l.warnings.isEmpty());
		}

	private:
		class Canvas : public QWidget
		{
			public:
				using QWidget::QWidget;
				QPicture picture;
				Layout layout;
				Fonts fonts;
				QString label;
				QString reference;

			protected:
				void paintEvent(QPaintEvent *) override
				{
					QPainter painter(this);
					painter.fillRect(rect(), Qt::white);
					painter.setRenderHint(QPainter::Antialiasing);

					QRectF bounds(-Stub, -Stub, layout.body_width + 2 * Stub,
						      layout.body_height + 2 * Stub);
					bounds = bounds.united(layout.label_box).adjusted(-10, -10, 10, 10);
					const qreal scale = std::min({width() / bounds.width(),
								      height() / bounds.height(), 3.0});
					painter.translate(width() / 2.0, height() / 2.0);
					painter.scale(scale, scale);
					painter.translate(-bounds.center());

					painter.drawPicture(0, 0, picture);

						//Terminals are items on a folio, not drawing: mark the
						//point each wire will reach
					painter.setPen(QPen(QColor(0, 102, 204), 1.5 / scale));
					for (const auto &t : layout.terminals) {
						painter.drawLine(t.point + QPointF(-2, 0), t.point + QPointF(2, 0));
						painter.drawLine(t.point + QPointF(0, -2), t.point + QPointF(0, 2));
					}

					painter.setPen(Qt::black);
					const QRectF box = layout.label_box;
					painter.setFont(fonts.label);
					painter.drawText(QRectF(box.x(), box.y(), box.width(), box.height()),
							 Qt::AlignHCenter | Qt::AlignTop, label);
					painter.setFont(fonts.names);
					painter.drawText(QRectF(box.x(), box.bottom(), box.width(), box.height()),
							 Qt::AlignHCenter | Qt::AlignTop, reference);
				}
		};

		Canvas *m_canvas = nullptr;
		QLabel *m_size = nullptr;
		QLabel *m_warnings = nullptr;
};

/**
	@brief The GenericDeviceLayoutPage class
	Page 1: how many terminals on each side, how they are grouped, the
	pitch and any extra size.
*/
class GenericDeviceLayoutPage : public QWizardPage
{
	Q_OBJECT

	public:
		explicit GenericDeviceLayoutPage(GenericDeviceWizard *wizard) :
			QWizardPage(wizard),
			m_wizard(wizard)
		{
			setTitle(tr("Layout"));
				//Finish from here makes the device with what is set so far
			setFinalPage(true);
			setSubTitle(tr("How many terminals on each side, 0 for none. "
				       "They are numbered 1…n; name them on the next page."));

			auto *grid = new QGridLayout();
			grid->addWidget(new QLabel(tr("Terminals")), 0, 1);
			grid->addWidget(new QLabel(tr("Group every")), 0, 2);
			for (int side = 0 ; side < SideCount ; ++side)
			{
				grid->addWidget(new QLabel(sideName(Side(side))), side + 1, 0);
				m_count[side] = new QSpinBox();
				m_count[side]->setRange(0, MaxPatternCount);
				m_group[side] = new QSpinBox();
				m_group[side]->setRange(0, MaxPatternCount);
				m_group[side]->setSpecialValueText(tr("no groups"));
				m_group[side]->setToolTip(tr("Leave a gap after every this many terminals"));
				grid->addWidget(m_count[side], side + 1, 1);
				grid->addWidget(m_group[side], side + 1, 2);
				connect(m_count[side], qOverload<int>(&QSpinBox::valueChanged),
					this, [this, side](int value) { countChanged(Side(side), value); });
				connect(m_group[side], qOverload<int>(&QSpinBox::valueChanged),
					this, [this, side](int value) { groupChanged(Side(side), value); });
			}

			m_mark_gaps = new QCheckBox(tr("Mark gaps with a divider"));
			m_line_up = new QCheckBox(tr("Line up groups across opposite sides"));
			m_line_up->setToolTip(tr("Adds blank rows to the shorter group, so that "
						 "the gaps on facing sides fall on the same row"));

			m_pitch = new QSpinBox();
			m_pitch->setRange(1, 5);
			m_pitch->setSuffix(tr(" grid squares"));
			m_extra_width = new QSpinBox();
			m_extra_width->setRange(0, 100);
			m_extra_width->setSuffix(tr(" grid squares"));
			m_extra_height = new QSpinBox();
			m_extra_height->setRange(0, 100);
			m_extra_height->setSuffix(tr(" grid squares"));

			auto *form = new QFormLayout();
			form->addRow(tr("Space between terminals:"), m_pitch);
			form->addRow(tr("Extra width:"), m_extra_width);
			form->addRow(tr("Extra height:"), m_extra_height);

			auto *layout = new QVBoxLayout(this);
			layout->addLayout(grid);
			layout->addWidget(m_mark_gaps);
			layout->addWidget(m_line_up);
			layout->addLayout(form);
			layout->addStretch();

			connect(m_mark_gaps, &QCheckBox::toggled, this, [this](bool on) {
				m_wizard->editableSpec().mark_gaps = on; changed(); });
			connect(m_line_up, &QCheckBox::toggled, this, [this](bool on) {
				m_wizard->editableSpec().line_up = on; changed(); });
			connect(m_pitch, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
				m_wizard->editableSpec().pitch = v * Grid; changed(); });
			connect(m_extra_width, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
				m_wizard->editableSpec().extra_width = v; changed(); });
			connect(m_extra_height, qOverload<int>(&QSpinBox::valueChanged), this, [this](int v) {
				m_wizard->editableSpec().extra_height = v; changed(); });
		}

		void initializePage() override
		{
				//Back from page 2 shows what was done there
			const Spec &spec = m_wizard->editableSpec();
			for (int side = 0 ; side < SideCount ; ++side) {
				QSignalBlocker blocker(m_count[side]);
				int count = 0;
				for (const Slot &s : spec.sides[side])
					if (s.isTerminal()) ++count;
				m_count[side]->setValue(count);
			}
			QSignalBlocker b1(m_mark_gaps), b2(m_line_up), b3(m_pitch), b4(m_extra_width), b5(m_extra_height);
			m_mark_gaps->setChecked(spec.mark_gaps);
			m_line_up->setChecked(spec.line_up);
			m_pitch->setValue(spec.pitch / Grid);
			m_extra_width->setValue(spec.extra_width);
			m_extra_height->setValue(spec.extra_height);
		}

			/// Going Back must never change anything (scope §8)
		void cleanupPage() override {}

		bool isComplete() const override
		{
			return m_wizard->editableSpec().terminalCount() > 0;
		}

	private:
		void countChanged(Side side, int value)
		{
			Spec &spec = m_wizard->editableSpec();
			const bool numbered = hasNumberedPins(spec);
			spec.sides[side] = GenericDevice::resize(spec.sides[side], value, m_group[side]->value());
			nameNewTerminals(spec, numbered);
			changed();
		}

		void groupChanged(Side side, int value)
		{
			Spec &spec = m_wizard->editableSpec();
			const Slots regrouped = groupEvery(spec.sides[side], value);
			const Slots expected = groupEvery(spec.sides[side], m_group_value[side]);
			auto same = [](const Slots &a, const Slots &b) {
				if (a.size() != b.size()) return false;
				for (int i = 0 ; i < a.size() ; ++i)
					if (a[i].kind != b[i].kind) return false;
				return true;
			};
				//Gaps typed on page 2 would be replaced: say so first
			if (!same(spec.sides[side], expected)
			    && QMessageBox::question(this, tr("Group terminals"),
						     tr("Grouping replaces the gaps you made on the %1 side. Continue?")
						     .arg(sideName(side).toLower())) != QMessageBox::Yes)
			{
				QSignalBlocker blocker(m_group[side]);
				m_group[side]->setValue(m_group_value[side]);
				return;
			}
			m_group_value[side] = value;
			spec.sides[side] = regrouped;
			changed();
		}

		void changed()
		{
			m_wizard->specChanged();
			emit completeChanged();
		}

		GenericDeviceWizard *m_wizard;
		QSpinBox *m_count[SideCount];
		QSpinBox *m_group[SideCount];
		int m_group_value[SideCount] = {0, 0, 0, 0};
		QCheckBox *m_mark_gaps;
		QCheckBox *m_line_up;
		QSpinBox *m_pitch;
		QSpinBox *m_extra_width;
		QSpinBox *m_extra_height;
};

/**
	@brief The GenericDeviceTerminalsPage class
	Page 2: one table per side, and the same list as a pattern line above
	it. Editing either rewrites the other.
*/
class GenericDeviceTerminalsPage : public QWizardPage
{
	Q_OBJECT

	public:
		explicit GenericDeviceTerminalsPage(GenericDeviceWizard *wizard) :
			QWizardPage(wizard),
			m_wizard(wizard)
		{
			setTitle(tr("Terminals"));
				//Finish from here makes the device with what is set so far
			setFinalPage(true);
			setSubTitle(tr("Name the terminals. A side can be typed as one line: "
				       "In{4} gives In1 to In4, Q{z4} Q0 to Q3, X{3-6} X3 to X6, "
				       "and an empty entry is a gap."));

			m_tabs = new QTabWidget();
			for (int side = 0 ; side < SideCount ; ++side)
			{
				auto *tab = new QWidget();
				auto *layout = new QVBoxLayout(tab);
				auto *line = new QHBoxLayout();
				line->addWidget(new QLabel(tr("As a line:")));
				m_pattern[side] = new QLineEdit();
				m_pattern[side]->setPlaceholderText(tr("e.g. L1,L2,L3,,PE"));
				line->addWidget(m_pattern[side], 1);
				layout->addLayout(line);
				m_error[side] = new QLabel();
				m_error[side]->setWordWrap(true);
				m_error[side]->setStyleSheet(QStringLiteral("color: #c0392b"));
				m_error[side]->hide();
				layout->addWidget(m_error[side]);

				m_table[side] = new QTableWidget(0, 3);
				m_table[side]->setHorizontalHeaderLabels({tr("Name"), tr("Function"), tr("Type")});
				m_table[side]->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
				m_table[side]->setSelectionBehavior(QAbstractItemView::SelectRows);
				layout->addWidget(m_table[side], 1);
				m_tabs->addTab(tab, sideName(Side(side)));

				connect(m_pattern[side], &QLineEdit::textEdited, this,
					[this, side](const QString &text) { patternEdited(Side(side), text); });
				connect(m_table[side], &QTableWidget::itemChanged, this,
					[this, side](QTableWidgetItem *item) { itemChanged(Side(side), item); });
			}

			auto *add = new QPushButton(tr("Add terminal"));
			auto *gap = new QPushButton(tr("Add gap"));
			auto *remove = new QPushButton(tr("Delete"));
			auto *up = new QPushButton(tr("Up"));
			auto *down = new QPushButton(tr("Down"));
			auto *buttons = new QHBoxLayout();
			for (QPushButton *b : {add, gap, remove, up, down})
				buttons->addWidget(b);
			buttons->addStretch();

			auto *number = new QPushButton(tr("Number pins 1…n"));
			number->setToolTip(tr("Rename every terminal by its pin number, "
					      "counter-clockwise from the top of the left side"));
			auto *paste = new QPushButton(tr("Paste table"));
			paste->setToolTip(tr("Rows of side, name, function and type, copied from "
					     "a spreadsheet or a CSV file. Side is left, right, top, "
					     "bottom or L, R, T, B; an empty name is a gap."));
			auto *global = new QHBoxLayout();
			global->addWidget(number);
			global->addWidget(paste);
			global->addStretch();

			m_paste_report = new QLabel();
			m_paste_report->setWordWrap(true);
			m_paste_report->hide();

			auto *layout = new QVBoxLayout(this);
			layout->addWidget(m_tabs, 1);
			layout->addLayout(buttons);
			layout->addLayout(global);
			layout->addWidget(m_paste_report);

			connect(add, &QPushButton::clicked, this, [this] { insert(Slot::terminal(QString())); });
			connect(gap, &QPushButton::clicked, this, [this] { insert(Slot::gap()); });
			connect(remove, &QPushButton::clicked, this, &GenericDeviceTerminalsPage::removeRows);
			connect(up, &QPushButton::clicked, this, [this] { move(-1); });
			connect(down, &QPushButton::clicked, this, [this] { move(1); });
			connect(number, &QPushButton::clicked, this, [this] {
				numberPins(m_wizard->editableSpec());
				rebuildAll();
				changed();
			});
			connect(paste, &QPushButton::clicked, this, &GenericDeviceTerminalsPage::pasteTable);
		}

		void initializePage() override
		{
			rebuildAll();
				//Open on the first side that has terminals
			const Spec &spec = m_wizard->editableSpec();
			for (int side = 0 ; side < SideCount ; ++side)
				if (!spec.sides[side].isEmpty()) { m_tabs->setCurrentIndex(side); break; }
		}

		void cleanupPage() override {}

		bool isComplete() const override
		{
			return m_wizard->editableSpec().terminalCount() > 0;
		}

	private:
		Side currentSide() const { return Side(m_tabs->currentIndex()); }

		void rebuildAll()
		{
			for (int side = 0 ; side < SideCount ; ++side) {
				rebuildTable(Side(side));
				updatePattern(Side(side));
			}
		}

		void rebuildTable(Side side)
		{
			QTableWidget *table = m_table[side];
			const Slots &list = m_wizard->editableSpec().sides[side];
			QSignalBlocker blocker(table);
			table->clearSpans();
			table->setRowCount(int(list.size()));
			for (int row = 0 ; row < list.size() ; ++row)
			{
				const Slot &slot = list.at(row);
				table->removeCellWidget(row, 2);
				if (!slot.isTerminal())
				{
					auto *item = new QTableWidgetItem(tr("gap"));
					item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
					item->setForeground(palette().color(QPalette::PlaceholderText));
					item->setTextAlignment(Qt::AlignCenter);
					table->setItem(row, 0, item);
					delete table->takeItem(row, 1);
					table->setSpan(row, 0, 1, 3);
					continue;
				}
				table->setItem(row, 0, new QTableWidgetItem(slot.name));
				table->setItem(row, 1, new QTableWidgetItem(slot.function));
				auto *type = new QComboBox();
				type->addItem(tr("Generic"), QStringLiteral("Generic"));
				type->addItem(tr("Inner"), QStringLiteral("Inner"));
				type->addItem(tr("Outer"), QStringLiteral("Outer"));
				type->setCurrentIndex(std::max(0, type->findData(slot.type)));
				connect(type, qOverload<int>(&QComboBox::currentIndexChanged), this,
					[this, side, type, row](int) {
					Slots &l = m_wizard->editableSpec().sides[side];
					if (row < l.size()) l[row].type = type->currentData().toString();
					changed();
				});
				table->setCellWidget(row, 2, type);
			}
			markRepeatedNames();
		}

			/// A repeated name gets a warning icon on its row, never a block
		void markRepeatedNames()
		{
			const Spec &spec = m_wizard->editableSpec();
			QStringList all;
			for (const Slots &side : spec.sides)
				for (const Slot &s : side)
					if (s.isTerminal()) all << s.name.trimmed();
			QSet<QString> repeated;
			for (const auto &pair : TerminalNameCheck::repeatedNames(all))
				repeated << pair.first;

			for (int side = 0 ; side < SideCount ; ++side)
			{
				QSignalBlocker blocker(m_table[side]);
				const Slots &list = spec.sides[side];
				for (int row = 0 ; row < list.size() ; ++row)
				{
					QTableWidgetItem *item = m_table[side]->item(row, 0);
					if (!item || !list.at(row).isTerminal()) continue;
					const bool twice = repeated.contains(list.at(row).name.trimmed());
					item->setIcon(twice ? QIcon::fromTheme(QStringLiteral("dialog-warning")) : QIcon());
					item->setToolTip(twice ? tr("This name is used more than once. "
								    "Fine if the device repeats it, as N or PE often are.")
							       : QString());
				}
			}
		}

		void updatePattern(Side side)
		{
			bool expressible = true;
			const QString text = toPattern(m_wizard->editableSpec().sides[side], &expressible);
			if (!m_pattern[side]->hasFocus())
				m_pattern[side]->setText(text);
			m_pattern[side]->setReadOnly(!expressible);
			m_pattern[side]->setToolTip(expressible ? QString()
								: tr("A name holds a comma or a brace, which a line "
								     "cannot say: edit this side in the table."));
			m_error[side]->hide();
		}

		void patternEdited(Side side, const QString &text)
		{
			const PatternResult result = expandPattern(text);
			if (!result.ok()) {
				m_error[side]->setText(result.error);
				m_error[side]->show();
				return;
			}
			m_error[side]->hide();
			Slots &list = m_wizard->editableSpec().sides[side];
			list = applyPattern(list, result.list);
			rebuildTable(side);
			changed();
		}

		void itemChanged(Side side, QTableWidgetItem *item)
		{
			Slots &list = m_wizard->editableSpec().sides[side];
			if (item->row() >= list.size() || !list[item->row()].isTerminal())
				return;
			if (item->column() == 0)
				list[item->row()].name = item->text().trimmed();
			else if (item->column() == 1)
				list[item->row()].function = item->text().trimmed();
			updatePattern(side);
			markRepeatedNames();
			changed();
		}

		void insert(const Slot &slot)
		{
			const Side side = currentSide();
			Spec &spec = m_wizard->editableSpec();
			const bool numbered = hasNumberedPins(spec);
			const int current = m_table[side]->currentRow();
			const int row = current < 0 ? int(spec.sides[side].size()) : current + 1;
			spec.sides[side].insert(row, slot);
			if (slot.isTerminal())
				nameNewTerminals(spec, numbered);
			rebuildAll();
			m_table[side]->setCurrentCell(row, 0);
			changed();
		}

		void removeRows()
		{
			const Side side = currentSide();
			QList<int> rows;
			for (const QModelIndex &index : m_table[side]->selectionModel()->selectedRows())
				rows << index.row();
			std::sort(rows.rbegin(), rows.rend());
			Spec &spec = m_wizard->editableSpec();
			const bool numbered = hasNumberedPins(spec);
			for (int row : rows)
				spec.sides[side].removeAt(row);
			if (numbered) numberPins(spec);
			rebuildAll();
			changed();
		}

		void move(int step)
		{
			const Side side = currentSide();
			Slots &list = m_wizard->editableSpec().sides[side];
			const int row = m_table[side]->currentRow();
			const int to = row + step;
			if (row < 0 || to < 0 || to >= list.size())
				return;
			list.swapItemsAt(row, to);
			rebuildTable(side);
			updatePattern(side);
			m_table[side]->setCurrentCell(to, 0);
			changed();
		}

		void pasteTable()
		{
			const PasteResult result = parsePastedTable(QApplication::clipboard()->text());
			if (!result.rows)
			{
				m_paste_report->setText(tr("Nothing to paste: copy rows of side, name, function "
							   "from a spreadsheet or a CSV file first."));
				m_paste_report->show();
				return;
			}
			Spec &spec = m_wizard->editableSpec();
			for (int side = 0 ; side < SideCount ; ++side)
				if (result.has_side[side]) spec.sides[side] = result.sides[side];
			rebuildAll();
			changed();
			if (result.bad_rows.isEmpty()) {
				m_paste_report->setText(tr("Pasted %n row(s).", "", result.rows));
			} else {
				m_paste_report->setText(tr("Pasted %1 rows; skipped these, which could not be read: %2")
							.arg(result.rows)
							.arg(result.bad_rows.join(QStringLiteral(" · "))));
			}
			m_paste_report->show();
		}

		void changed()
		{
			m_wizard->specChanged();
			emit completeChanged();
		}

		GenericDeviceWizard *m_wizard;
		QTabWidget *m_tabs;
		QLineEdit *m_pattern[SideCount];
		QLabel *m_error[SideCount];
		QTableWidget *m_table[SideCount];
		QLabel *m_paste_report;
};

/**
	@brief The GenericDeviceInformationPage class
	Page 3: the name of the symbol and the element information.
*/
class GenericDeviceInformationPage : public QWizardPage
{
	Q_OBJECT

	public:
		explicit GenericDeviceInformationPage(GenericDeviceWizard *wizard) :
			QWizardPage(wizard),
			m_wizard(wizard)
		{
			setTitle(tr("Information"));
			setSubTitle(tr("The name is how the symbol is listed in the project's "
				       "collection; the rest can be changed later on the placed device."));

			m_name = new QLineEdit();
			m_name->setPlaceholderText(tr("Generic device"));
			m_label = new QLineEdit();
			m_label->setPlaceholderText(tr("e.g. -U1"));
			m_manufacturer = new QLineEdit();
			m_reference = new QLineEdit();
			m_description = new QLineEdit();
			m_show_reference = new QCheckBox(tr("Show the manufacturer reference in the box"));

			auto *form = new QFormLayout(this);
			form->addRow(tr("Name:"), m_name);
			form->addRow(tr("Label:"), m_label);
			form->addRow(tr("Manufacturer:"), m_manufacturer);
			form->addRow(tr("Manufacturer reference:"), m_reference);
			form->addRow(tr("Description:"), m_description);
			form->addRow(m_show_reference);

			auto bind = [this](QLineEdit *edit, QString Spec::*field) {
				connect(edit, &QLineEdit::textChanged, this, [this, field](const QString &text) {
					m_wizard->editableSpec().*field = text;
					m_wizard->specChanged();
				});
			};
			bind(m_name, &Spec::name);
			bind(m_label, &Spec::label);
			bind(m_manufacturer, &Spec::manufacturer);
			bind(m_reference, &Spec::reference);
			bind(m_description, &Spec::description);
			connect(m_show_reference, &QCheckBox::toggled, this, [this](bool on) {
				m_wizard->editableSpec().show_reference = on;
				m_wizard->specChanged();
			});
		}

		void initializePage() override
		{
			const Spec &spec = m_wizard->editableSpec();
			for (auto pair : {qMakePair(m_name, spec.name), qMakePair(m_label, spec.label),
					  qMakePair(m_manufacturer, spec.manufacturer),
					  qMakePair(m_reference, spec.reference),
					  qMakePair(m_description, spec.description)}) {
				QSignalBlocker blocker(pair.first);
				pair.first->setText(pair.second);
			}
			QSignalBlocker blocker(m_show_reference);
			m_show_reference->setChecked(spec.show_reference);
		}

		void cleanupPage() override {}

		bool isComplete() const override
		{
			return m_wizard->editableSpec().terminalCount() > 0;
		}

	private:
		GenericDeviceWizard *m_wizard;
		QLineEdit *m_name;
		QLineEdit *m_label;
		QLineEdit *m_manufacturer;
		QLineEdit *m_reference;
		QLineEdit *m_description;
		QCheckBox *m_show_reference;
};

/**
	@brief GenericDeviceWizard::GenericDeviceWizard
	@param fonts : the fonts the folio draws names and labels with, so the
	box is sized for them
*/
GenericDeviceWizard::GenericDeviceWizard(const Fonts &fonts, QWidget *parent) :
	QWizard(parent),
	m_fonts(fonts)
{
	setWindowTitle(tr("Generic device"));
	setOption(QWizard::HaveFinishButtonOnEarlyPages);
	setOption(QWizard::NoBackButtonOnStartPage);

		//Something to see on opening: two numbered terminals left and right
	m_spec.sides[Left] = GenericDevice::resize({}, 2, 0);
	m_spec.sides[Right] = GenericDevice::resize({}, 2, 0);
	numberPins(m_spec);

	m_preview = new GenericDevicePreview(this);
	setSideWidget(m_preview);

	setPage(LayoutPageId, new GenericDeviceLayoutPage(this));
	setPage(TerminalsPageId, new GenericDeviceTerminalsPage(this));
	setPage(InformationPageId, new GenericDeviceInformationPage(this));
	resize(960, 600);
	specChanged();
}

/**
	@return the device as filled in, with a name even if none was typed
*/
Spec GenericDeviceWizard::spec() const
{
	Spec spec = m_spec;
	spec.name = spec.name.trimmed();
	if (spec.name.isEmpty())
		spec.name = tr("Generic device");
	return spec;
}

void GenericDeviceWizard::specChanged()
{
	m_preview->setSpec(m_spec, m_fonts);
	emit specUpdated();
}

#include "genericdevicewizard.moc"
