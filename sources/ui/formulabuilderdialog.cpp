// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "formulabuilderdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

using namespace FormulaBuilder;

namespace FormulaBuilderDialogPrivate
{
	/**
		@brief The NodeEditor class
		One node of the formula: what it is (a field, a text, an
		operation...), its own value if it has one, and an editor for each of
		its inputs.
	*/
	class NodeEditor : public QWidget
	{
		Q_OBJECT

		public:
			NodeEditor(const QList<Field> &fields, const QVariantMap &node, QWidget *parent = nullptr);
			QVariantMap node() const;

		signals:
			void changed();

		private:
			void showNode(const QVariantMap &node);
			void addInput(const QVariantMap &child);
			QString inputLabel(const Function &f, int index) const;
			void updateButtons();

			QList<Field> m_fields;
			QComboBox *m_kind;
			QComboBox *m_field;
			QLineEdit *m_text;
			QDoubleSpinBox *m_number;
			QLineEdit *m_separator;
			QFormLayout *m_inputs;
			QList<NodeEditor *> m_children;
			QPushButton *m_add;
			QPushButton *m_remove;
	};

	NodeEditor::NodeEditor(const QList<Field> &fields, const QVariantMap &node, QWidget *parent) :
		QWidget(parent),
		m_fields(fields)
	{
		auto *layout = new QVBoxLayout(this);
		layout->setContentsMargins(0, 0, 0, 0);

		m_kind = new QComboBox;
		auto *model = new QStandardItemModel(m_kind);
		m_kind->setModel(model);
		QString category;
		for (const Function &f : functions()) {
			if (f.category != category) {   // a heading, which cannot be picked
				category = f.category;
				auto *heading = new QStandardItem(category);
				heading->setFlags(Qt::NoItemFlags);
				model->appendRow(heading);
			}
			auto *item = new QStandardItem(f.label);
			item->setData(f.id, Qt::UserRole);
			item->setToolTip(f.help);
			model->appendRow(item);
		}
		layout->addWidget(m_kind);

		m_field = new QComboBox;
		for (const Field &f : m_fields)
			m_field->addItem(f.label, f.expression);
		m_text = new QLineEdit;
		m_text->setPlaceholderText(tr("Enter the text"));
		m_number = new QDoubleSpinBox;
		m_number->setDecimals(6);
		m_number->setRange(-1e12, 1e12);
		m_separator = new QLineEdit;
		m_separator->setPlaceholderText(tr("Between the values, for example a space or a dash"));
		for (QWidget *w : QList<QWidget *>{m_field, m_text, m_number, m_separator})
			layout->addWidget(w);

		auto *inputs_frame = new QWidget;
		m_inputs = new QFormLayout(inputs_frame);
		m_inputs->setContentsMargins(18, 0, 0, 0);
		layout->addWidget(inputs_frame);

		auto *buttons = new QHBoxLayout;
		buttons->setContentsMargins(18, 0, 0, 0);
		m_add = new QPushButton(tr("Add an input"));
		m_remove = new QPushButton(tr("Remove the last input"));
		buttons->addWidget(m_add);
		buttons->addWidget(m_remove);
		buttons->addStretch();
		layout->addLayout(buttons);

		connect(m_kind, QOverload<int>::of(&QComboBox::activated), this, [this]() {
			showNode(defaultNode(m_kind->currentData(Qt::UserRole).toString(), m_fields));
			emit changed();
		});
		connect(m_field, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NodeEditor::changed);
		connect(m_text, &QLineEdit::textChanged, this, &NodeEditor::changed);
		connect(m_separator, &QLineEdit::textChanged, this, &NodeEditor::changed);
		connect(m_number, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &NodeEditor::changed);
		connect(m_add, &QPushButton::clicked, this, [this]() {
			addInput(defaultNode("field", m_fields));
			updateButtons();
			emit changed();
		});
		connect(m_remove, &QPushButton::clicked, this, [this]() {
			if (m_children.isEmpty()) return;
			m_inputs->removeRow(m_children.size() - 1);   // deletes the label and the editor
			m_children.removeLast();
			updateButtons();
			emit changed();
		});
		showNode(node);
	}

	/// The label of input @p index of @p f
	QString NodeEditor::inputLabel(const Function &f, int index) const
	{
		return f.arguments.value(index, tr("Input %1").arg(index + 1));
	}

	/// Show @p node: the kind, its value and its inputs
	void NodeEditor::showNode(const QVariantMap &node)
	{
		const QString op = node.value("op", "field").toString();
		const Function f = function(op);
		const QSignalBlocker blockers[] = {QSignalBlocker(m_kind), QSignalBlocker(m_field), QSignalBlocker(m_text),
										   QSignalBlocker(m_separator), QSignalBlocker(m_number)};
		m_kind->setCurrentIndex(m_kind->findData(op, Qt::UserRole));
		m_field->setVisible(op == "field");
		m_text->setVisible(op == "text");
		m_number->setVisible(op == "number");
		m_separator->setVisible(op == "join");
		if (op == "field") m_field->setCurrentIndex(qMax(0, m_field->findData(node.value("value"))));
		if (op == "text") m_text->setText(node.value("value").toString());
		if (op == "number") m_number->setValue(node.value("value", 1).toDouble());
		if (op == "join") m_separator->setText(node.value("separator", " ").toString());

		while (!m_children.isEmpty()) {
			m_inputs->removeRow(m_children.size() - 1);
			m_children.removeLast();
		}
		const QVariantList args = node.value("args").toList();
		for (int i = 0 ; i < args.size() ; ++i)
			addInput(args.at(i).toMap());
		updateButtons();
	}

	void NodeEditor::addInput(const QVariantMap &child)
	{
		const Function f = function(m_kind->currentData(Qt::UserRole).toString());
		auto *editor = new NodeEditor(m_fields, child);
		connect(editor, &NodeEditor::changed, this, &NodeEditor::changed);
		m_inputs->addRow(inputLabel(f, m_children.size()), editor);
		m_children << editor;
	}

	void NodeEditor::updateButtons()
	{
		const Function f = function(m_kind->currentData(Qt::UserRole).toString());
		m_add->setVisible(f.maximum > f.minimum);
		m_remove->setVisible(f.maximum > f.minimum);
		m_add->setEnabled(m_children.size() < f.maximum);
		m_remove->setEnabled(m_children.size() > f.minimum);
	}

	QVariantMap NodeEditor::node() const
	{
		const QString op = m_kind->currentData(Qt::UserRole).toString();
		QVariantMap result;
		result["op"] = op;
		if (op == "field") result["value"] = m_field->currentData();
		if (op == "text") result["value"] = m_text->text();
		if (op == "number") result["value"] = m_number->value();
		if (op == "join") result["separator"] = m_separator->text();
		QVariantList args;
		for (const NodeEditor *child : m_children)
			args << child->node();
		result["args"] = args;
		return result;
	}
}

using FormulaBuilderDialogPrivate::NodeEditor;

/**
	@brief FormulaBuilderDialog::FormulaBuilderDialog
	@param fields the columns the formula may read
	@param parent
*/
FormulaBuilderDialog::FormulaBuilderDialog(const QList<Field> &fields, QWidget *parent) :
	QDialog(parent),
	m_fields(fields)
{
	setWindowTitle(tr("Formula column"));
	resize(560, 520);
	auto *layout = new QVBoxLayout(this);

	auto *note = new QLabel(tr("Build the value of a new column. Pick a field, a value or an operation; "
							   "an operation's inputs can be operations too. Quotes and brackets are written for you."));
	note->setWordWrap(true);
	layout->addWidget(note);

	auto *form = new QFormLayout;
	m_title = new QLineEdit;
	m_title->setPlaceholderText(tr("Title of the column"));
	form->addRow(tr("Column title"), m_title);
	layout->addLayout(form);

	m_editor = new NodeEditor(m_fields, defaultNode("field", m_fields));
	layout->addWidget(m_editor);
	layout->addStretch();

	m_error = new QLabel;
	m_error->setWordWrap(true);
	m_error->setStyleSheet(QStringLiteral("color:#b00020"));
	layout->addWidget(m_error);

	m_sql = new QPlainTextEdit;
	m_sql->setReadOnly(true);
	m_sql->setMaximumHeight(70);
	layout->addWidget(new QLabel(tr("The SQL this makes (read only)")));
	layout->addWidget(m_sql);

	m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	m_buttons->button(QDialogButtonBox::Ok)->setText(tr("Add the column"));
	layout->addWidget(m_buttons);
	connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_title, &QLineEdit::textChanged, this, &FormulaBuilderDialog::update);
	connect(m_editor, &NodeEditor::changed, this, &FormulaBuilderDialog::update);
	update();
}

/// @return the title of the column, with no quote or line break in it
QString FormulaBuilderDialog::title() const
{
	QString title = m_title->text().simplified();
	title.remove('"');
	return title;
}

/**
	@brief FormulaBuilderDialog::update
	Compile the formula as it stands and say whether it can be used.
*/
void FormulaBuilderDialog::update()
{
	QString error;
	m_expression = compile(m_editor->node(), m_fields, &error);
	m_error->setText(error);
	m_sql->setPlainText(m_expression);
	m_buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_expression.isEmpty() && !title().isEmpty());
}

#include "formulabuilderdialog.moc"
