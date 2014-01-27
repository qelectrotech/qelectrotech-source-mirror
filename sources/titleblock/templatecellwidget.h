/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_CELL_WIDGET_H
#define TITLEBLOCK_SLASH_TEMPLATE_CELL_WIDGET_H
#include <QtGui>
#include "qet.h"
class ModifyTitleBlockCellCommand;
class TitleBlockTemplate;
class TitleBlockCell;
class NamesList;

/**
	This class implements an edition widget for cells that compose a title
	block template.
*/
class TitleBlockTemplateCellWidget : public QWidget {
	Q_OBJECT
	
	// constructor, destructor
	public:
	TitleBlockTemplateCellWidget(TitleBlockTemplate * = 0, QWidget * = 0);
	virtual ~TitleBlockTemplateCellWidget();
	private:
	TitleBlockTemplateCellWidget(const TitleBlockTemplateCellWidget &);
	
	// attributes
	private:
	/// is the template read-only?
	bool read_only_;
	QLabel    *cell_type_label_;
	QComboBox *cell_type_input_;
	
	QLabel *empty_label_;
	
	QLabel      *logo_label_;
	QComboBox   *logo_input_;
	QPushButton *add_logo_input_;
	
	QLabel        *name_label_;
	QLineEdit     *name_input_;
	QCheckBox     *label_checkbox_;
	QLineEdit     *label_input_;
	QPushButton   *label_edit_;
	QLabel        *value_label_;
	QLineEdit     *value_input_;
	QPushButton   *value_edit_;
	QLabel        *align_label_;
	QLabel        *horiz_align_label_;
	QComboBox     *horiz_align_input_;
	QHash<int, int> horiz_align_indexes_;
	QLabel        *vert_align_label_;
	QComboBox     *vert_align_input_;
	QHash<int, int> vert_align_indexes_;
	QLabel        *font_size_label_;
	QSpinBox      *font_size_input_;
	QCheckBox     *font_adjust_input_;
	QVBoxLayout   *cell_editor_layout_;
	QHBoxLayout   *cell_editor_type_and_name_layout_;
	QGridLayout   *cell_editor_text_layout_;
	QHBoxLayout   *cell_editor_image_layout_;
	
	TitleBlockCell *edited_cell_;
	
	// methods
	public:
	int horizontalAlignment() const;
	int verticalAlignment() const;
	int alignment() const;
	bool isReadOnly() const;
	
	protected:
	void editTranslatableValue(NamesList &, const QString &, const QString &) const;
	void emitModification(const QString &, const QVariant &) const;
	QString defaultVariablesString() const;
	QString labelValueInformationString() const;
	
	private:
	void initWidgets();
	
	public slots:
	void updateFormType(int);
	void edit(TitleBlockCell *);
	void editType();
	void editName();
	void editLabelDisplayed();
	void editLabel();
	void editValue();
	void editAlignment();
	void editFontSize();
	void editAdjust();
	void editLogo();
	void updateLogosComboBox(const TitleBlockTemplate *);
	void setReadOnly(bool);
	
	private slots:
	
	
	signals:
	void logoEditionRequested();
	void cellModified(ModifyTitleBlockCellCommand *) const;
};

#endif
