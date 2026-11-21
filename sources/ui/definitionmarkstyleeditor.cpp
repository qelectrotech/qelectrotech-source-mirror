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
#include "definitionmarkstyleeditor.h"
#include "ui_definitionmarkstyleeditor.h"

#include "autoNum/assignvariables.h"
#include "qetproject.h"

#include <QAction>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QGraphicsScene>
#include <QMenu>
#include <QPair>
#include <QPainter>
#include <QSettings>
#include <QTransform>
#include <QUndoCommand>

#include <functional>

namespace {
		/// Where the sample conductors of the preview stand, and where
		/// the stroke is anchored on each of them
	const QPointF horizontal_anchor(180, 150);
	const QPointF vertical_anchor(400, 150);

		/// What the preview's texts show when their formula says
		/// nothing: the wire values of a sample conductor
	autonum::FormulaContext sampleContext()
	{
		autonum::FormulaContext context;
		context.has_conductor = true;
		context.wire_color = QStringLiteral("BN");
		context.wire_section = QStringLiteral("2.5 mm²");
		context.wire_function = QStringLiteral("F1");
		context.wire_tension_protocol = QStringLiteral("24 V");
		context.wire_type = QStringLiteral("H07V-K");
		context.wire_cable = QStringLiteral("K1");
		return context;
	}

		/// What the shapes and texts of the preview take from their
		/// sample conductor
	DefinitionMarkStyle::RenderContext sampleRenderContext()
	{
		DefinitionMarkStyle::RenderContext context;
		context.conductor_color = Qt::black;
		context.conductor_width = 1.5;
		context.text_color = Qt::black;
		context.font = QFont(QStringLiteral("Sans Serif"), 8);
		return context;
	}

	QStringList sampleTexts(const DefinitionMarkStyle &style)
	{
		const autonum::FormulaContext context = sampleContext();
		autonum::sequentialNumbers numbers;
		QStringList texts;
		for (const DefinitionMarkStyle::TextItem &text : style.m_texts)
			texts << autonum::AssignVariables::formulaToLabel(
					text.m_formula, numbers, context);
		return texts;
	}

	/**
		@brief The ChangeDefinitionMarkStyle class
		One undo step for the way a project draws its definition marks
	*/
	class ChangeDefinitionMarkStyle : public QUndoCommand
	{
		public:
			ChangeDefinitionMarkStyle(QETProject *project,
						 const DefinitionMarkStyle &after) :
				m_project(project),
				m_after(after),
				m_before(project->definitionMarkStyle())
			{
				setText(QCoreApplication::translate(
					"DefinitionMarkStyleEditor",
					"Customize the stroke with colour and section"));
			}

			void redo() override { m_project->setDefinitionMarkStyle(m_after); }
			void undo() override { m_project->setDefinitionMarkStyle(m_before); }

		private:
			QETProject *m_project;
			DefinitionMarkStyle m_after;
			DefinitionMarkStyle m_before;
	};
}

/**
	@brief The PreviewItem class
	One shape or one text of the stroke, in the editor's preview: drawn
	from the style, dragged to change the place it stands at.
*/
class PreviewItem : public QGraphicsObject
{
	public:
		PreviewItem(DefinitionMarkStyle *style,
			    int index, bool text_item,
			    const DefinitionMarkStyle::RenderContext &context,
			    qreal turn, bool interactive,
			    const std::function<void()> &moved) :
			m_style(style),
			m_context(context),
			m_moved(moved),
			m_index(index),
			m_turn(turn),
			m_text_item(text_item)
		{
			if (interactive) {
				setFlags(ItemIsMovable | ItemIsSelectable
					 | ItemSendsGeometryChanges);
			} else {
				setAcceptedMouseButtons(Qt::NoButton);
			}
			setPos(place());
		}

		bool isText() const { return m_text_item; }
		int index() const { return m_index; }

		void setText(const QString &text)
		{
			m_text = text;
			update();
		}

		/// Take again the place the style says
		void refresh()
		{
			prepareGeometryChange();
			setPos(place());
			update();
		}

		QRectF boundingRect() const override
		{
			return rect().adjusted(-2, -2, 2, 2);
		}

		void paint(QPainter *painter,
			   const QStyleOptionGraphicsItem *,
			   QWidget *) override
		{
				//The item already stands at its place: the
				//shape or text is drawn at its centre
			if (m_text_item) {
				auto text = m_style->m_texts.at(m_index);
				text.m_pos = QPointF();
				DefinitionMarkStyle::paintItem(painter, text,
							       m_context, m_text,
							       m_turn);
			} else {
				auto shape = m_style->m_shapes.at(m_index);
				shape.m_pos = QPointF();
				DefinitionMarkStyle::paintItem(painter, shape,
							       m_context, m_turn);
			}
		}

	protected:
		QVariant itemChange(GraphicsItemChange change,
				    const QVariant &value) override
		{
			if (change == ItemPositionHasChanged
			    && (flags() & ItemIsMovable)) {
				if (m_text_item)
					m_style->m_texts[m_index].m_pos = value.toPointF();
				else
					m_style->m_shapes[m_index].m_pos = value.toPointF();
				if (m_moved)
					m_moved();
			}
			return QGraphicsObject::itemChange(change, value);
		}

	private:
		QPointF place() const
		{
			const QPointF pos = m_text_item
				? m_style->m_texts.at(m_index).m_pos
				: m_style->m_shapes.at(m_index).m_pos;
			return QTransform().rotate(m_turn).map(pos);
		}

		QRectF rect() const
		{
			if (m_text_item) {
				auto text = m_style->m_texts.at(m_index);
				text.m_pos = QPointF();
				return DefinitionMarkStyle::itemRect(text, m_context,
								     m_text, m_turn);
			}
			auto shape = m_style->m_shapes.at(m_index);
			shape.m_pos = QPointF();
			return DefinitionMarkStyle::itemRect(shape, m_context, m_turn);
		}

		DefinitionMarkStyle *m_style;
		DefinitionMarkStyle::RenderContext m_context;
		std::function<void()> m_moved;
		QString m_text;
		int m_index;
		qreal m_turn;
		bool m_text_item;
};

/**
	@brief DefinitionMarkStyleEditor::DefinitionMarkStyleEditor
	Edit the way @p project draws its definition marks, with a sample
	conductor showing the result.
	@param project the project whose style is edited
	@param parent
*/
DefinitionMarkStyleEditor::DefinitionMarkStyleEditor(QETProject *project,
						     QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DefinitionMarkStyleEditor),
	m_project(project),
	m_scene(new QGraphicsScene(this))
{
	ui->setupUi(this);
		//Room for the preview beside the properties
	ui->m_main_gl->setColumnStretch(0, 2);
	ui->m_main_gl->setColumnStretch(1, 3);
	m_style = project ? project->definitionMarkStyle()
			  : DefinitionMarkStyle::standardStyle();
		//An editor for a project that customized nothing opens on the
		//drawing the mark has always had, so there is something to see
		//and to change: the slanted stroke with its two texts
	if (m_style.m_shapes.isEmpty() && m_style.m_texts.isEmpty()) {
		const bool enabled = m_style.m_enabled;
		m_style = DefinitionMarkStyle::standardStyle();
		m_style.m_enabled = enabled;
	}

		//The two sample conductors of the preview
	m_scene->setSceneRect(10, 25, 415, 255);
	const QList<QLineF> samples {
		QLineF(QPointF(30, horizontal_anchor.y()),
		       QPointF(330, horizontal_anchor.y())),
		QLineF(QPointF(vertical_anchor.x(), 40),
		       QPointF(vertical_anchor.x(), 260))};
	for (const QLineF &sample : samples) {
		auto *conductor = m_scene->addLine(sample, QPen(Qt::black, 1.5));
		conductor->setZValue(-1);
	}
	ui->m_preview_view->setScene(m_scene);
	ui->m_preview_view->setRenderHint(QPainter::Antialiasing);
	connect(m_scene, &QGraphicsScene::selectionChanged,
		this, &DefinitionMarkStyleEditor::previewSelectionChanged);

		//The shapes and texts take their kind from a menu
	auto *shape_menu = new QMenu(ui->m_add_shape_pb);
	const QList<QPair<QString, int>> kinds {
		{tr("Line"), int(DefinitionMarkStyle::ShapeKind::Line)},
		{tr("Circle"), int(DefinitionMarkStyle::ShapeKind::Circle)},
		{tr("Rectangle"), int(DefinitionMarkStyle::ShapeKind::Rectangle)}};
	for (const auto &kind : kinds) {
		QAction *action = shape_menu->addAction(kind.first);
		action->setData(kind.second);
		connect(action, &QAction::triggered,
			this, &DefinitionMarkStyleEditor::addShape);
	}
	ui->m_add_shape_pb->setMenu(shape_menu);

		//The variables of a conductor's formula, to write in a text
	auto *variable_menu = new QMenu(ui->m_insert_variable_tb);
	const QList<QPair<QString, QString>> variables {
		{tr("Colour of the wire (%wc)"), QStringLiteral("%wc")},
		{tr("Section of the wire (%ws)"), QStringLiteral("%ws")},
		{tr("Function (%wf)"), QStringLiteral("%wf")},
		{tr("Voltage / protocol (%wv)"), QStringLiteral("%wv")},
		{tr("Conductor type (%wt)"), QStringLiteral("%wt")},
		{tr("Cable (%wb)"), QStringLiteral("%wb")}};
	for (const auto &variable : variables) {
		connect(variable_menu->addAction(variable.first), &QAction::triggered,
			this, [this, variable]() {
				ui->m_text_formula_pte->textCursor().insertText(variable.second);
				applyTextForm();
			});
	}
	ui->m_insert_variable_tb->setMenu(variable_menu);
		//The variables and what they give, in the open: their names
		//alone say nothing
	ui->m_variable_help_label->setText([&variables]() {
		QStringList described;
		for (const auto &variable : variables)
			described << variable.first;
		return described.join(QStringLiteral(", "));
	}());

		//The properties write the style as they are edited
	for (QDoubleSpinBox *sb : {ui->m_shape_x_sb, ui->m_shape_y_sb,
				   ui->m_shape_rotation_sb, ui->m_shape_width_sb,
				   ui->m_shape_height_sb, ui->m_shape_pen_width_sb})
		connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			this, &DefinitionMarkStyleEditor::applyShapeForm);
	connect(ui->m_shape_kind_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &DefinitionMarkStyleEditor::applyShapeForm);
	connect(ui->m_shape_filled_cb, &QCheckBox::toggled,
		this, &DefinitionMarkStyleEditor::applyShapeForm);
	connect(ui->m_shape_auto_color_cb, &QCheckBox::toggled,
		this, &DefinitionMarkStyleEditor::applyShapeForm);
	connect(ui->m_shape_color_kpb, &KColorButton::changed,
		this, &DefinitionMarkStyleEditor::applyShapeForm);

	for (QDoubleSpinBox *sb : {ui->m_text_x_sb, ui->m_text_y_sb,
				   ui->m_text_rotation_sb, ui->m_text_size_sb})
		connect(sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
			this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_formula_pte, &QPlainTextEdit::textChanged,
		this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_upright_cb, &QCheckBox::toggled,
		this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_auto_color_cb, &QCheckBox::toggled,
		this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_color_kpb, &KColorButton::changed,
		this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_halign_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &DefinitionMarkStyleEditor::applyTextForm);
	connect(ui->m_text_valign_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &DefinitionMarkStyleEditor::applyTextForm);

		//The turn the drawing takes where the conductor runs up and
		//down: the combo's rows are its four quarters
	for (int i = 0 ; i < ui->m_vertical_turn_cb->count() ; ++i)
		ui->m_vertical_turn_cb->setItemData(i, i * 90.0);
	ui->m_vertical_turn_cb->setCurrentIndex(qRound(m_style.m_vertical_turn / 90.0) % 4);
	connect(ui->m_vertical_turn_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &DefinitionMarkStyleEditor::applyVerticalTurn);

	connect(ui->m_enabled_cb, &QCheckBox::toggled,
		this, &DefinitionMarkStyleEditor::toggleEnabled);
	connect(ui->m_add_text_pb, &QPushButton::clicked,
		this, &DefinitionMarkStyleEditor::addText);
	connect(ui->m_remove_pb, &QPushButton::clicked,
		this, &DefinitionMarkStyleEditor::removeItem);
	connect(ui->m_standard_pb, &QPushButton::clicked,
		this, &DefinitionMarkStyleEditor::resetToStandard);
	connect(ui->m_items_lw, &QListWidget::currentRowChanged,
		this, &DefinitionMarkStyleEditor::currentRowChanged);
	connect(ui->m_button_box, &QDialogButtonBox::accepted,
		this, &DefinitionMarkStyleEditor::accept);
	connect(ui->m_button_box, &QDialogButtonBox::rejected,
		this, &DefinitionMarkStyleEditor::reject);

		//Saving the drawing as the default for the projects created
		//from now on is a choice the user makes, not one made for
		//them: the box starts unchecked
	ui->m_as_default_cb->setChecked(false);

	m_filling = true;
	ui->m_enabled_cb->setChecked(m_style.m_enabled);
	m_filling = false;
	toggleEnabled(m_style.m_enabled);
	updateList();
	rebuildItems();
	fillForm();
	ui->m_preview_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

DefinitionMarkStyleEditor::~DefinitionMarkStyleEditor()
{
		//The preview's items go with the scene, whose selection must
		//not reach the dialog as it is being destroyed
	disconnect(m_scene, nullptr, this, nullptr);
	delete ui;
}

/**
	The drawing is one undo step: it goes back and forth as a whole. A
	drawing that is not turned on saves as none: the marks are drawn the
	way they always were, and nothing is written into the project.
*/
void DefinitionMarkStyleEditor::accept()
{
	const DefinitionMarkStyle style = m_style.m_enabled
			? m_style : DefinitionMarkStyle();
	if (m_project && style != m_project->definitionMarkStyle()) {
		m_project->undoStack()->push(
			new ChangeDefinitionMarkStyle(m_project, style));
	}
	if (ui->m_as_default_cb->isChecked()) {
		QSettings settings;
		style.toSettings(settings, QStringLiteral("diagrameditor/default"));
	}
	QDialog::accept();
}

/**
	@brief DefinitionMarkStyleEditor::resizeEvent
	Keep both sample conductors of the preview in view
	@param event
*/
void DefinitionMarkStyleEditor::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);
		//Only when shown: during setupUi() the preview is not there yet
	if (isVisible() && m_scene)
		ui->m_preview_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

/**
	@brief DefinitionMarkStyleEditor::showEvent
	Fit the preview to its final size
	@param event
*/
void DefinitionMarkStyleEditor::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	ui->m_preview_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

/**
	@brief DefinitionMarkStyleEditor::toggleEnabled
	Turn the customized drawing on or off; the shapes and texts are only
	edited while it is on.
	@param on
*/
void DefinitionMarkStyleEditor::toggleEnabled(bool on)
{
	m_style.m_enabled = on;
	ui->m_items_gb->setEnabled(on);
	ui->m_properties_stack->setEnabled(on);
}

/**
	@brief DefinitionMarkStyleEditor::applyVerticalTurn
	Keep the turn the drawing takes where the conductor runs up and
	down, and draw the vertical sample again with it
	@param index
*/
void DefinitionMarkStyleEditor::applyVerticalTurn(int index)
{
	if (m_filling)
		return;
	m_style.m_vertical_turn = ui->m_vertical_turn_cb->itemData(index).toDouble();
	rebuildItems();
}

void DefinitionMarkStyleEditor::addShape()
{
	auto *action = qobject_cast<QAction *>(sender());
	DefinitionMarkStyle::ShapeItem shape;
	shape.m_kind = action
		? DefinitionMarkStyle::ShapeKind(action->data().toInt())
		: DefinitionMarkStyle::ShapeKind::Line;
	const int index = m_style.m_shapes.size();
	m_style.m_shapes << shape;
	updateList();
	rebuildItems();
	selectRow(rowOf(false, index));
}

void DefinitionMarkStyleEditor::addText()
{
	DefinitionMarkStyle::TextItem text;
	text.m_formula = QStringLiteral("%wc");
	text.m_pos = QPointF(0, -12);
	const int index = m_style.m_texts.size();
	m_style.m_texts << text;
	updateList();
	rebuildItems();
	selectRow(rowOf(true, index));
}

void DefinitionMarkStyleEditor::removeItem()
{
	const int row = ui->m_items_lw->currentRow();
	if (row < 0)
		return;
	if (rowIsText(row))
		m_style.m_texts.removeAt(rowIndexOf(row));
	else
		m_style.m_shapes.removeAt(rowIndexOf(row));
	updateList();
	rebuildItems();
	fillForm();
}

/**
	@brief DefinitionMarkStyleEditor::currentRowChanged
	Show the properties of the item the list has selected
*/
void DefinitionMarkStyleEditor::currentRowChanged()
{
	if (m_filling)
		return;
	const int row = ui->m_items_lw->currentRow();
	m_filling = true;
	for (PreviewItem *item : m_preview_items)
		item->setSelected(row >= 0
				  && item->isText() == rowIsText(row)
				  && item->index() == rowIndexOf(row));
	m_filling = false;
	fillForm();
}

/**
	@brief DefinitionMarkStyleEditor::fillForm
	Show in the properties the item the list has selected
*/
void DefinitionMarkStyleEditor::fillForm()
{
	m_filling = true;
	const int row = ui->m_items_lw->currentRow();
	ui->m_properties_stack->setCurrentIndex(row < 0
			? 0
			: (rowIsText(row) ? 2 : 1));
	if (row >= 0 && rowIsText(row)) {
		const DefinitionMarkStyle::TextItem &text =
			m_style.m_texts.at(rowIndexOf(row));
		ui->m_text_formula_pte->setPlainText(text.m_formula);
		ui->m_text_x_sb->setValue(text.m_pos.x());
		ui->m_text_y_sb->setValue(text.m_pos.y());
		ui->m_text_rotation_sb->setValue(text.m_rotation);
		ui->m_text_size_sb->setValue(text.m_size);
		ui->m_text_auto_color_cb->setChecked(!text.m_color.isValid());
		ui->m_text_color_kpb->setColor(text.m_color.isValid()
				? text.m_color : QColor(Qt::black));
		ui->m_text_color_kpb->setEnabled(text.m_color.isValid());
		ui->m_text_upright_cb->setChecked(text.m_upright);
		ui->m_text_halign_cb->setCurrentIndex(text.m_alignment & Qt::AlignLeft ? 0
				: text.m_alignment & Qt::AlignRight ? 2 : 1);
		ui->m_text_valign_cb->setCurrentIndex(text.m_alignment & Qt::AlignTop ? 0
				: text.m_alignment & Qt::AlignBottom ? 2 : 1);
	} else if (row >= 0) {
		const DefinitionMarkStyle::ShapeItem &shape =
			m_style.m_shapes.at(rowIndexOf(row));
		ui->m_shape_kind_cb->setCurrentIndex(
			shape.m_kind == DefinitionMarkStyle::ShapeKind::Circle ? 1
			: shape.m_kind == DefinitionMarkStyle::ShapeKind::Rectangle ? 2 : 0);
		ui->m_shape_x_sb->setValue(shape.m_pos.x());
		ui->m_shape_y_sb->setValue(shape.m_pos.y());
		ui->m_shape_rotation_sb->setValue(shape.m_rotation);
		ui->m_shape_width_sb->setValue(shape.m_width);
		ui->m_shape_height_sb->setValue(shape.m_height);
		ui->m_shape_pen_width_sb->setValue(shape.m_pen_width);
		ui->m_shape_auto_color_cb->setChecked(!shape.m_color.isValid());
		ui->m_shape_color_kpb->setColor(shape.m_color.isValid()
				? shape.m_color : QColor(Qt::black));
		ui->m_shape_color_kpb->setEnabled(shape.m_color.isValid());
		ui->m_shape_filled_cb->setChecked(shape.m_filled);
		ui->m_shape_filled_cb->setEnabled(
			shape.m_kind != DefinitionMarkStyle::ShapeKind::Line);
	}
	m_filling = false;
}

/**
	@brief DefinitionMarkStyleEditor::applyShapeForm
	Write the shape properties into the style, as they are edited
*/
void DefinitionMarkStyleEditor::applyShapeForm()
{
	if (m_filling)
		return;
	const int row = ui->m_items_lw->currentRow();
	if (row < 0 || rowIsText(row))
		return;
	DefinitionMarkStyle::ShapeItem &shape =
		m_style.m_shapes[rowIndexOf(row)];
	const int kind = ui->m_shape_kind_cb->currentIndex();
	shape.m_kind = kind == 1 ? DefinitionMarkStyle::ShapeKind::Circle
		: kind == 2 ? DefinitionMarkStyle::ShapeKind::Rectangle
		: DefinitionMarkStyle::ShapeKind::Line;
	shape.m_pos = QPointF(ui->m_shape_x_sb->value(),
			      ui->m_shape_y_sb->value());
	shape.m_rotation = ui->m_shape_rotation_sb->value();
	shape.m_width = ui->m_shape_width_sb->value();
	shape.m_height = ui->m_shape_height_sb->value();
	shape.m_pen_width = ui->m_shape_pen_width_sb->value();
	shape.m_color = ui->m_shape_auto_color_cb->isChecked()
			? QColor() : ui->m_shape_color_kpb->color();
	shape.m_filled = ui->m_shape_filled_cb->isChecked();
	ui->m_shape_filled_cb->setEnabled(shape.m_kind != DefinitionMarkStyle::ShapeKind::Line);
	ui->m_shape_color_kpb->setEnabled(!ui->m_shape_auto_color_cb->isChecked());
	updatePreviews();
}

/**
	@brief DefinitionMarkStyleEditor::applyTextForm
	Write the text properties into the style, as they are edited
*/
void DefinitionMarkStyleEditor::applyTextForm()
{
	if (m_filling)
		return;
	const int row = ui->m_items_lw->currentRow();
	if (row < 0 || !rowIsText(row))
		return;
	DefinitionMarkStyle::TextItem &text =
		m_style.m_texts[rowIndexOf(row)];
	text.m_formula = ui->m_text_formula_pte->toPlainText();
	text.m_pos = QPointF(ui->m_text_x_sb->value(),
			     ui->m_text_y_sb->value());
	text.m_rotation = ui->m_text_rotation_sb->value();
	text.m_size = ui->m_text_size_sb->value();
	text.m_color = ui->m_text_auto_color_cb->isChecked()
			? QColor() : ui->m_text_color_kpb->color();
	text.m_upright = ui->m_text_upright_cb->isChecked();
	const int horizontal = ui->m_text_halign_cb->currentIndex();
	const int vertical = ui->m_text_valign_cb->currentIndex();
	text.m_alignment = Qt::Alignment(horizontal == 0 ? Qt::AlignLeft
			: horizontal == 2 ? Qt::AlignRight : Qt::AlignHCenter)
		| (vertical == 0 ? Qt::AlignTop
		   : vertical == 2 ? Qt::AlignBottom : Qt::AlignVCenter);
	ui->m_text_color_kpb->setEnabled(!ui->m_text_auto_color_cb->isChecked());

	if (QListWidgetItem *item = ui->m_items_lw->item(row))
		item->setText(tr("Text: %1").arg(text.m_formula));
	updatePreviews();
}

/**
	@brief DefinitionMarkStyleEditor::resetToStandard
	Go back to the standard drawing: one slanted stroke with the colour
	above and the section below, as the mark is drawn when nothing was
	customized.
*/
void DefinitionMarkStyleEditor::resetToStandard()
{
	m_style = DefinitionMarkStyle::standardStyle();
	m_style.m_enabled = false;
	m_filling = true;
	ui->m_enabled_cb->setChecked(false);
	ui->m_vertical_turn_cb->setCurrentIndex(0);
	m_filling = false;
	toggleEnabled(false);
	updateList();
	rebuildItems();
	fillForm();
}

/**
	@brief DefinitionMarkStyleEditor::previewSelectionChanged
	Show the properties of the item dragged or clicked in the preview
*/
void DefinitionMarkStyleEditor::previewSelectionChanged()
{
	if (m_filling)
		return;
	for (QGraphicsItem *item : m_scene->selectedItems()) {
		auto *preview = dynamic_cast<PreviewItem *>(item);
		if (preview) {
			selectRow(rowOf(preview->isText(), preview->index()));
			return;
		}
	}
}

/**
	@brief DefinitionMarkStyleEditor::rebuildItems
	Draw the shapes and texts of the style again, on both sample
	conductors: the horizontal one moves them, the vertical one only
	shows how the drawing follows the conductor's direction.
*/
void DefinitionMarkStyleEditor::rebuildItems()
{
	qDeleteAll(m_preview_items);
	m_preview_items.clear();
	qDeleteAll(m_anchors);
	m_anchors.clear();

	const QList<QPair<QPointF, qreal>> anchors {
		{horizontal_anchor, 0},
		{vertical_anchor, m_style.m_vertical_turn}};
	for (const auto &anchor : anchors)
	{
		auto *holder = m_scene->addRect(QRectF(-1, -1, 2, 2),
						Qt::NoPen, Qt::NoBrush);
		holder->setPos(anchor.first);
		holder->setZValue(-1);
		m_anchors << holder;
		const bool interactive = anchor.second == 0;
		for (int i = 0 ; i < m_style.m_shapes.size() ; ++i) {
			auto *item = new PreviewItem(&m_style, i, false,
					sampleRenderContext(), anchor.second,
					interactive,
					[this]() { fillForm(); });
			item->setParentItem(holder);
			m_preview_items << item;
		}
		for (int i = 0 ; i < m_style.m_texts.size() ; ++i) {
			auto *item = new PreviewItem(&m_style, i, true,
					sampleRenderContext(), anchor.second,
					interactive,
					[this]() { fillForm(); });
			item->setParentItem(holder);
			m_preview_items << item;
		}
	}
	updatePreviews();
}

/**
	@brief DefinitionMarkStyleEditor::updatePreviews
	Draw the shapes and texts again, with the words the preview's sample
	conductor gives to their formulas
*/
void DefinitionMarkStyleEditor::updatePreviews()
{
	const QStringList texts = sampleTexts(m_style);
	for (PreviewItem *item : m_preview_items) {
		item->setText(item->isText() ? texts.value(item->index()) : QString());
		item->refresh();
	}
}

/**
	@brief DefinitionMarkStyleEditor::updateList
	List the shapes and texts of the style
*/
void DefinitionMarkStyleEditor::updateList()
{
	const int row = ui->m_items_lw->currentRow();
	m_filling = true;
	ui->m_items_lw->clear();
	const QList<QString> kinds {tr("Line"), tr("Circle"), tr("Rectangle")};
	for (int i = 0 ; i < m_style.m_shapes.size() ; ++i) {
		const DefinitionMarkStyle::ShapeItem &shape = m_style.m_shapes.at(i);
		auto *item = new QListWidgetItem(
			kinds.at(shape.m_kind == DefinitionMarkStyle::ShapeKind::Circle ? 1
				 : shape.m_kind == DefinitionMarkStyle::ShapeKind::Rectangle ? 2 : 0),
			ui->m_items_lw);
		item->setData(Qt::UserRole, false);
		item->setData(Qt::UserRole + 1, i);
	}
	for (int i = 0 ; i < m_style.m_texts.size() ; ++i) {
		auto *item = new QListWidgetItem(
			tr("Text: %1").arg(m_style.m_texts.at(i).m_formula),
			ui->m_items_lw);
		item->setData(Qt::UserRole, true);
		item->setData(Qt::UserRole + 1, i);
	}
	ui->m_items_lw->setCurrentRow(qMin(row, ui->m_items_lw->count() - 1));
	m_filling = false;
}

/**
	@brief DefinitionMarkStyleEditor::selectRow
	Show the properties of the item at @p row in the list, and pick it
	in the preview
	@param row
*/
void DefinitionMarkStyleEditor::selectRow(int row)
{
	m_filling = true;
	ui->m_items_lw->setCurrentRow(row);
	for (PreviewItem *item : m_preview_items)
		item->setSelected(row >= 0
				  && item->isText() == rowIsText(row)
				  && item->index() == rowIndexOf(row));
	m_filling = false;
	fillForm();
}

int DefinitionMarkStyleEditor::rowOf(bool text_item, int index) const
{
	return text_item ? m_style.m_shapes.size() + index : index;
}

bool DefinitionMarkStyleEditor::rowIsText(int row) const
{
	return row >= m_style.m_shapes.size();
}

int DefinitionMarkStyleEditor::rowIndexOf(int row) const
{
	return rowIsText(row) ? row - m_style.m_shapes.size() : row;
}
