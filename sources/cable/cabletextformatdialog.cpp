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
#include "cabletextformatdialog.h"

#include "../qetapp.h"
#include "../utils/qetutils.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
	/**
		@brief The way a text lines up, as the dialog lists it: right
		first, which is where every text stands unless he says otherwise.
		@param index the place in the list of the dialog
		@return the flags the drawing understands
	*/
	Qt::Alignment alignmentFromIndex(int index)
	{
		switch (index)
		{
		case 1: return Qt::AlignHCenter;
		case 2: return Qt::AlignLeft;
		default: return Qt::AlignRight;
		}
	}

	/**
		@brief The place in the list of the dialog of an alignment.
		@param alignment Qt::AlignLeft, Qt::AlignHCenter or Qt::AlignRight
		@return the index, 0 (right) when nothing is named
	*/
	int alignmentIndex(int alignment)
	{
		switch (alignment)
		{
		case int(Qt::AlignHCenter): return 1;
		case int(Qt::AlignLeft): return 2;
		default: return 0;
		}
	}

	/**
		Whether two fonts would be read the very same way on the sheet:
		the family he picks, the size he gives it, and the style he
		chose among the four offered.
		@param a the font he has chosen
		@param b the font the preferences would write it with
		@return true when nothing of them differs
	*/
	bool sameFont(const QFont &a, const QFont &b)
	{
		return a.family() == b.family()
				&& qFuzzyCompare(a.pointSizeF(), b.pointSizeF())
				&& a.bold() == b.bold()
				&& a.italic() == b.italic();
	}
}

/**
	@brief CableTextFormatDialog::CableTextFormatDialog
	@param format what this cable writes that text with right now
	@param cores true for the colours of the cores
	@param parent
*/
CableTextFormatDialog::CableTextFormatDialog(const CableTextFormat &format,
											 bool cores,
											 QWidget *parent) :
	QDialog(parent),
	m_cores(cores)
{
	setWindowTitle(cores ? tr("Core colors")
						 : tr("Text format"));
	buildUi();

		//The font this text is written with right now: the one this
		//cable asks for, or the one the preferences offer when it asks
		//for none -- which is what he sees on the sheet as he opens this.
	QFont font;
	bool restored = false;
	if (!format.font.isEmpty()) {
		restored = QETUtils::fontFromString(font, format.font);
	}
	if (!restored) {
		font = cores ? QETApp::cableCoreFont() : QETApp::cableTextsFont();
	}

		//Where this text stands right now, in the same way: the answer
		//of the cable itself, or -- when it has none -- the one the
		//preferences offer, so that he opens this onto what the sheet
		//really shows rather than onto whatever the default used to be.
	m_initial_alignment = cores ? 0
			: (format.alignment ? format.alignment
								: int(QETApp::cableTextAlignment()));

	m_family->setCurrentFont(font);
	m_size->setValue(font.pointSizeF());
	if (font.bold() && font.italic()) m_style->setCurrentIndex(3);
	else if (font.italic()) m_style->setCurrentIndex(2);
	else if (font.bold()) m_style->setCurrentIndex(1);
	else m_style->setCurrentIndex(0);
	if (m_alignment) {
		m_alignment->setCurrentIndex(alignmentIndex(m_initial_alignment));
	}

		//Going back to the preferences means something only while this
		//cable has an answer of its own: when it already follows them,
		//there is nothing here to give back.
	m_reset->setEnabled(!format.font.isEmpty() || format.alignment != 0);
	updatePreview();
}

/**
	@brief CableTextFormatDialog::buildUi
	One row per thing he may set, then what it looks like -- because a
	size means nothing as a number alone.
*/
void CableTextFormatDialog::buildUi()
{
	auto *form = new QFormLayout;
	form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	form->setHorizontalSpacing(12);

	m_family = new QFontComboBox(this);
	m_family->setFontFilters(QFontComboBox::AllFonts);
	form->addRow(tr("Font:"), m_family);

	m_size = new QDoubleSpinBox(this);
	m_size->setRange(1.0, 300.0);
	m_size->setDecimals(1);
	m_size->setSingleStep(0.5);
	m_size->setSuffix(tr(" pt"));
	form->addRow(tr("Size:"), m_size);

	m_style = new QComboBox(this);
	m_style->addItem(tr("Normal"));
	m_style->addItem(tr("Bold"));
	m_style->addItem(tr("Italic"));
	m_style->addItem(tr("Bold italic"));
	form->addRow(tr("Style:"), m_style);

		//The colours of the cores stand turned by a quarter along their
		//own line: there is nothing for them to line up with.
	if (!m_cores)
	{
		m_alignment = new QComboBox(this);
		m_alignment->addItem(tr("Right aligned"));
		m_alignment->addItem(tr("Centred"));
		m_alignment->addItem(tr("Left aligned"));
		form->addRow(tr("Alignment:"), m_alignment);
	}

	m_preview = new QLabel(tr("15W1 · H07V-K 3G1.5"), this);
	m_preview->setFrameStyle(QFrame::Box | QFrame::Plain);
	m_preview->setMinimumHeight(30);
	m_preview->setIndent(6);
	form->addRow(tr("Preview:"), m_preview);

	auto *buttons = new QDialogButtonBox(
				QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	m_reset = buttons->addButton(tr("Reset"), QDialogButtonBox::ResetRole);
	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_reset, &QPushButton::clicked, this, [this]() {
		m_reset_chosen = true;
		accept();
	});

	auto *layout = new QVBoxLayout(this);
	layout->addLayout(form);
	layout->addWidget(buttons);
	setLayout(layout);

	connect(m_family, &QFontComboBox::currentFontChanged,
			this, &CableTextFormatDialog::updatePreview);
	connect(m_size, &QDoubleSpinBox::valueChanged,
			this, &CableTextFormatDialog::updatePreview);
	connect(m_style, &QComboBox::currentIndexChanged,
			this, &CableTextFormatDialog::updatePreview);
	if (m_alignment) {
		connect(m_alignment, &QComboBox::currentIndexChanged,
				this, &CableTextFormatDialog::updatePreview);
	}
}

/**
	@brief CableTextFormatDialog::chosenFont
	What the dialog offers right now: the family he picked, the size he
	gave it and the style he chose among the four offered -- rather than
	whatever that family happened to be called in the list.
	@return the font
*/
QFont CableTextFormatDialog::chosenFont() const
{
	QFont font = m_family->currentFont();
	font.setPointSizeF(m_size->value());
	const bool italic = m_style->currentIndex() == 2 || m_style->currentIndex() == 3;
	const bool bold = m_style->currentIndex() == 1 || m_style->currentIndex() == 3;
	font.setBold(bold);
	font.setItalic(italic);
	return font;
}

/**
	@brief CableTextFormatDialog::updatePreview
	Write the line offered as an example the way it would be written
	after what he has just set -- the only place where a size becomes
	something one can judge.
*/
void CableTextFormatDialog::updatePreview()
{
	m_preview->setFont(chosenFont());
	if (m_alignment) {
		m_preview->setAlignment(
					alignmentFromIndex(m_alignment->currentIndex()) | Qt::AlignVCenter);
	} else {
		m_preview->setAlignment(Qt::AlignCenter);
	}
}

/**
	@brief CableTextFormatDialog::format
	@return what the cable is to write that text with from now on.

	An empty answer means "the way the preferences say", and the two
	halves of it are worked out apart from each other: he may set the
	way one text lines up without taking away its freedom to follow a
	new default font later, and the other way round. What already
	stands the way the preferences say is left out, so that a cable
	which said nothing keeps saying nothing -- and a later change of
	what those preferences are still reaches it.
*/
CableTextFormat CableTextFormatDialog::format() const
{
	if (m_reset_chosen) return CableTextFormat();

	const QFont font = chosenFont();
	const QFont default_font = m_cores ? QETApp::cableCoreFont()
									   : QETApp::cableTextsFont();

	CableTextFormat answer;
	if (!sameFont(font, default_font)) {
		answer.font = QETUtils::fontToString(font);
	}
		//The colours of the cores stand turned by a quarter: they have
		//no line to line up with, and saying so would only be noise in
		//the file.
	if (!m_cores)
	{
		const int alignment = int(alignmentFromIndex(m_alignment->currentIndex()));
		if (alignment != int(QETApp::cableTextAlignment())) {
			answer.alignment = alignment;
		}
	}
	return answer;
}
