#include "guidespropertieswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QColorDialog>

GuidesPropertiesWidget::GuidesPropertiesWidget(QWidget *parent)
: QWidget(parent)
{
	setupUi();
}

GuidesPropertiesWidget::~GuidesPropertiesWidget() {}

void GuidesPropertiesWidget::setupUi() {
	QVBoxLayout *main_layout = new QVBoxLayout(this);

	m_table = new QTableWidget(0, 3, this);
	m_table->setHorizontalHeaderLabels({tr("Orientation"), tr("Position"), tr("Couleur")});
	m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

	m_add_btn = new QPushButton(tr("Ajouter"), this);
	m_remove_btn = new QPushButton(tr("Supprimer"), this);

	QHBoxLayout *btn_layout = new QHBoxLayout();
	btn_layout->addWidget(m_add_btn);
	btn_layout->addWidget(m_remove_btn);
	btn_layout->addStretch();

	main_layout->addWidget(m_table);
	main_layout->addLayout(btn_layout);

	connect(m_add_btn, &QPushButton::clicked, this, &GuidesPropertiesWidget::addGuide);
	connect(m_remove_btn, &QPushButton::clicked, this, &GuidesPropertiesWidget::removeGuide);
}

QList<Diagram::Guide> GuidesPropertiesWidget::guides() const {
	QList<Diagram::Guide> list;
	for (int row = 0; row < m_table->rowCount(); ++row) {
		QComboBox *combo = qobject_cast<QComboBox*>(m_table->cellWidget(row, 0));
		QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(m_table->cellWidget(row, 1));
		QPushButton *colorBtn = qobject_cast<QPushButton*>(m_table->cellWidget(row, 2));

		if (combo && spin && colorBtn) {
			Diagram::Guide g;
			g.orientation = (combo->currentIndex() == 0) ? Diagram::Guide::Horizontal : Diagram::Guide::Vertical;
			g.position = spin->value();
			g.color = colorBtn->property("color").value<QColor>();
			list.append(g);
		}
	}
	return list;
}

void GuidesPropertiesWidget::setGuides(const QList<Diagram::Guide> &guides) {
	m_table->setRowCount(0);
	for (const auto &g : guides) {
		addGuide();
		int row = m_table->rowCount() - 1;

		QComboBox *combo = qobject_cast<QComboBox*>(m_table->cellWidget(row, 0));
		QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(m_table->cellWidget(row, 1));
		QPushButton *colorBtn = qobject_cast<QPushButton*>(m_table->cellWidget(row, 2));

		if (combo && spin && colorBtn) {
			combo->setCurrentIndex(g.orientation == Diagram::Guide::Horizontal ? 0 : 1);
			spin->setValue(g.position);
			colorBtn->setProperty("color", g.color);
			colorBtn->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold;").arg(g.color.name()));
		}
	}
}

void GuidesPropertiesWidget::addGuide() {
	int row = m_table->rowCount();
	m_table->insertRow(row);

	QComboBox *combo = new QComboBox(this);
	combo->addItems({tr("Horizontal"), tr("Vertical")});
	m_table->setCellWidget(row, 0, combo);

	QDoubleSpinBox *spin = new QDoubleSpinBox(this);
	spin->setRange(-10000.0, 10000.0);
	spin->setDecimals(2);
	spin->setValue(100.0);
	m_table->setCellWidget(row, 1, spin);

	QPushButton *colorBtn = new QPushButton(tr("Couleur"), this);
	QColor defaultColor = Qt::lightGray;
	colorBtn->setProperty("color", defaultColor);
	colorBtn->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold;").arg(defaultColor.name()));

	connect(colorBtn, &QPushButton::clicked, [this, colorBtn]() {
		QColor c = QColorDialog::getColor(colorBtn->property("color").value<QColor>(), this);
		if (c.isValid()) {
			colorBtn->setProperty("color", c);
			colorBtn->setStyleSheet(QString("background-color: %1; color: white; font-weight: bold;").arg(c.name()));
		}
	});
	m_table->setCellWidget(row, 2, colorBtn);
}

void GuidesPropertiesWidget::removeGuide() {
	int row = m_table->currentRow();
	if (row >= 0) {
		m_table->removeRow(row);
	}
}
