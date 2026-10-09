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
#include "imagepropertieswidget.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../qeticons.h"
#include "../ui_imagepropertieswidget.h"
#include <QSignalBlocker>
#include <QDialog>
#include <QEvent>

/**
	@brief ImagePropertiesWidget::ImagePropertiesWidget
	Constructor
	@param image : image to edit properties
	@param parent : parent widget
*/
ImagePropertiesWidget::ImagePropertiesWidget(DiagramImageItem *image, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::ImagePropertiesWidget),
	m_image(nullptr)
{
	ui->setupUi(this);
	// Matches the .ui file's own initial checked="true" -- toggled()
	// only fires on a subsequent CHANGE, not for the state a widget
	// already starts in, so the icon needs setting explicitly here or
	// the button would show blank until first clicked.
	ui->m_lock_ratio_tb->setIcon(QET::Icons::ObjectLocked);
	this->setDisabled(true);
	setImageItem(image);
}

/**
	@brief ImagePropertiesWidget::~ImagePropertiesWidget
	Destructor
*/
ImagePropertiesWidget::~ImagePropertiesWidget()
{
	delete ui;
}

bool ImagePropertiesWidget::event(QEvent *event)
{
	// The generic properties dialog resets on its Cancel button, but its
	// title-bar close and Escape reach reject() directly. Roll back the
	// same preview in all rejection paths, without changing other editors.
	if (event->type() == QEvent::ParentChange) {
		if (auto *dialog = qobject_cast<QDialog *>(parentWidget())) {
			connect(dialog, &QDialog::rejected, this, &ImagePropertiesWidget::reset, Qt::UniqueConnection);
		}
	}
	return PropertiesEditorWidget::event(event);
}

/**
	@brief ImagePropertiesWidget::setImageItem
	Set the image to edit properties
	@param image : image to edit
*/
void ImagePropertiesWidget::setImageItem(DiagramImageItem *image)
{
	if(!image) return;
	this->setEnabled(true);
	if (m_image == image) return;
	if (m_image)
	{
		disconnect(m_image, &DiagramImageItem::transformChanged, this, &ImagePropertiesWidget::updateUi);
		disconnect(m_image, &DiagramImageItem::labelChanged, this, &ImagePropertiesWidget::updateUi);
		disconnect(m_image, &DiagramImageItem::adaptToDarkThemeChanged, this, &ImagePropertiesWidget::updateUi);
	}

	m_image = image;
	connect(m_image, &DiagramImageItem::transformChanged, this, &ImagePropertiesWidget::updateUi);
	connect(m_image, &DiagramImageItem::labelChanged, this, &ImagePropertiesWidget::updateUi);
	connect(m_image, &DiagramImageItem::adaptToDarkThemeChanged, this, &ImagePropertiesWidget::updateUi);
	m_movable = image->isMovable();
	m_adapt_to_dark_theme = image->adaptToDarkTheme();
	m_scaleX = m_image->scaleFactorX();
	m_scaleY = m_image->scaleFactorY();
	m_rotation = m_image->rotationAngle();
	m_skewX = m_image->skewX();
	m_skewY = m_image->skewY();
	m_label = m_image->label();
	updateUi();
}

/**
	@brief ImagePropertiesWidget::apply
	Apply the change
*/
void ImagePropertiesWidget::apply()
{
	if(!m_image) return;

	if (m_image->diagram())
	{
		if (m_live_edit) disconnect(m_image, &DiagramImageItem::transformChanged, this, &ImagePropertiesWidget::updateUi);

		QUndoCommand *undo = associatedUndo();
		if (undo)
			m_image->diagram()->undoStack().push(undo);

		if (m_live_edit) connect(m_image, &DiagramImageItem::transformChanged, this, &ImagePropertiesWidget::updateUi);
	}

	m_scaleX = m_image->scaleFactorX();
	m_scaleY = m_image->scaleFactorY();
	m_rotation = m_image->rotationAngle();
	m_skewX = m_image->skewX();
	m_skewY = m_image->skewY();
	m_label = m_image->label();
	m_adapt_to_dark_theme = m_image->adaptToDarkTheme();
}

/**
	@brief ImagePropertiesWidget::reset
	Reset the change
*/
void ImagePropertiesWidget::reset()
{
	if(!m_image) return;

	m_image->setScaleFactorX(m_scaleX);
	m_image->setScaleFactorY(m_scaleY);
	m_image->setRotationAngle(m_rotation);
	m_image->setSkewX(m_skewX);
	m_image->setSkewY(m_skewY);
	m_image->setLabel(m_label);
	m_image->setMovable(m_movable);
	m_preview_dark_theme = true;
	m_image->setAdaptToDarkTheme(m_adapt_to_dark_theme);
	m_preview_dark_theme = false;
	updateUi();
}

/**
	@brief ImagePropertiesWidget::setLiveEdit
	@param live_edit true -> enable live edit
 *					false -> disable live edit
	@return always true
*/
bool ImagePropertiesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit)
	{
		connect (ui->m_width_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_height_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_angle_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_skew_x_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_skew_y_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_label_le, &QLineEdit::editingFinished, this, &ImagePropertiesWidget::apply);
		connect (ui->m_adapt_to_dark_theme_cb, &QCheckBox::clicked, this, &ImagePropertiesWidget::apply);
	}
	else
	{
		disconnect (ui->m_width_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_height_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_angle_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_skew_x_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_skew_y_sb, &QDoubleSpinBox::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_label_le, &QLineEdit::editingFinished, this, &ImagePropertiesWidget::apply);
		disconnect (ui->m_adapt_to_dark_theme_cb, &QCheckBox::clicked, this, &ImagePropertiesWidget::apply);
	}

	return true;
}

/**
	@brief ImagePropertiesWidget::associatedUndo
	@return the change in an undo command. If there is no change return
	nullptr. Chains scaleFactorX, scaleFactorY, rotationAngle, skewX and
	skewY as separate QPropertyUndoCommands, matching the established
	"check each field, chain if changed" pattern used elsewhere -- each
	of these is genuinely independent (the resize/rotate/skew handles
	can set any of them without touching the others), so a single
	combined command wouldn't correctly capture "only two of five
	fields actually changed".
*/
QUndoCommand* ImagePropertiesWidget::associatedUndo() const
{
	const qreal newScaleX = ui->m_width_sb->value() / 100.0;
	const qreal newScaleY = ui->m_height_sb->value() / 100.0;
	const qreal newRotation = ui->m_angle_sb->value();
	const qreal newSkewX = ui->m_skew_x_sb->value();
	const qreal newSkewY = ui->m_skew_y_sb->value();

	QUndoCommand *undo = nullptr;
	auto chain = [&](const char *property, qreal oldValue, qreal newValue, const QString &text)
	{
		if (qFuzzyCompare(oldValue, newValue))
			return;
		if (undo)
			new QPropertyUndoCommand(m_image, property, oldValue, newValue, undo);
		else
		{
			auto *change = new QPropertyUndoCommand(m_image, property, oldValue, newValue);
			change->enableAnimation();
			change->setText(text);
			undo = change;
		}
	};

	chain("scaleFactorX", m_scaleX, newScaleX, tr("Change the width of an image"));
	chain("scaleFactorY", m_scaleY, newScaleY, tr("Change the height of an image"));
	chain("rotationAngle", m_rotation, newRotation, tr("Change the angle of an image"));
	chain("skewX", m_skewX, newSkewX, tr("Change the skew of an image"));
	chain("skewY", m_skewY, newSkewY, tr("Change the skew of an image"));

	// Not through chain(): a string cannot be animated.
	const QString newLabel = ui->m_label_le->text();
	if (newLabel != m_label)
	{
		if (undo)
			new QPropertyUndoCommand(m_image, "label", m_label, newLabel, undo);
		else
		{
			undo = new QPropertyUndoCommand(m_image, "label", m_label, newLabel);
			undo->setText(tr("Edit an image label"));
		}
	}

	const bool adapt = ui->m_adapt_to_dark_theme_cb->isChecked();
	if (adapt != m_adapt_to_dark_theme) {
		if (undo) new QPropertyUndoCommand(m_image, "adaptToDarkTheme", m_adapt_to_dark_theme, adapt, undo);
		else {
			undo = new QUndoCommand(tr("Change a picture's dark theme adaptation"));
			new QPropertyUndoCommand(m_image, "adaptToDarkTheme", m_adapt_to_dark_theme, adapt, undo);
		}
	}
	return undo;
}

/**
	@brief ImagePropertiesWidget::updateUi
	Udpdate the ui, notably when the image to edit change
*/
void ImagePropertiesWidget::updateUi()
{
	// Two different uses of the same flag: an entry guard here (skip
	// entirely if a locked width/height operation is already in
	// progress elsewhere -- its own explicit updateUi() call once
	// finished is what actually refreshes things), and, immediately
	// below, a re-entrancy guard around this function's OWN setValue()
	// calls, so they can't recurse back into
	// on_m_width_sb_valueChanged()/on_m_height_sb_valueChanged() when
	// this runs normally (e.g. from setImageItem(), or a transformChanged
	// signal from a resize handle used directly on the canvas while
	// this dialog is open).
	if (!m_image || m_updating_ratio) return;
	if (m_live_edit && !m_preview_dark_theme) m_adapt_to_dark_theme = m_image->adaptToDarkTheme();
	const QSignalBlocker darkThemeBlocker(ui->m_adapt_to_dark_theme_cb);
	ui->m_adapt_to_dark_theme_cb->setChecked(m_image->adaptToDarkTheme());

	m_updating_ratio = true;
	ui->m_width_sb->setValue(m_image->scaleFactorX() * 100.0);
	ui->m_height_sb->setValue(m_image->scaleFactorY() * 100.0);
	m_updating_ratio = false;

	ui->m_angle_sb->setValue(m_image->rotationAngle());
	ui->m_skew_x_sb->setValue(m_image->skewX());
	ui->m_skew_y_sb->setValue(m_image->skewY());
	ui->m_lock_pos_cb->setChecked(!m_image->isMovable());
	if (ui->m_label_le->text() != m_image->label())
		ui->m_label_le->setText(m_image->label());
}

/**
	@brief ImagePropertiesWidget::on_m_lock_ratio_tb_toggled
	Keeps the button's own icon and tooltip in sync with its checked
	state -- Qt shows the checked/unchecked state visually (a sunken
	look) regardless, but the lock/unlock glyph makes the meaning
	obvious at a glance rather than needing that convention noticed.
	@param checked
*/
void ImagePropertiesWidget::on_m_lock_ratio_tb_toggled(bool checked)
{
	ui->m_lock_ratio_tb->setIcon(checked ? QET::Icons::ObjectLocked : QET::Icons::ObjectUnlocked);
	ui->m_lock_ratio_tb->setToolTip(checked
			? tr("Locked: changing the width or height adjusts the other to keep proportions. Click to unlock.")
			: tr("Unlocked: width and height can be changed independently. Click to lock."));
}

/**
	@brief ImagePropertiesWidget::on_m_restore_ratio_pb_clicked
	The same action as the item's own "Restaurer les proportions"
	context-menu entry, offered here too since this dialog is exactly
	where someone would notice the proportions are off in the first
	place. Deliberately bypasses on_m_height_sb_valueChanged()'s own
	lock-following logic (guarded by m_updating_ratio, then applied
	directly rather than through the spinbox's own signal) rather than
	going through it normally: if the lock happened to be engaged,
	that logic would treat this as an ordinary edit and scale width by
	the same ratio the height change just applied -- compounding into
	some other, unrelated value instead of the plain "make height
	match width" this button promises regardless of lock state.
*/
void ImagePropertiesWidget::on_m_restore_ratio_pb_clicked()
{
	if (!m_image) return;
	const qreal newScaleY = m_image->scaleFactorX();
	m_updating_ratio = true;
	ui->m_height_sb->setValue(newScaleY * 100.0);
	m_updating_ratio = false;
	m_image->setScaleFactorY(newScaleY);
	apply();
}

/**
	@brief ImagePropertiesWidget::on_m_width_sb_valueChanged
	Applies the new width live, and -- if "Conserver les proportions" is
	checked -- scales height by the same ratio this change just applied
	to width, so the two stay locked together. Guarded by
	m_updating_ratio: the setValue() call below would otherwise
	re-trigger on_m_height_sb_valueChanged(), which would then try to
	scale width again in response, recursing indefinitely.
	@param value the new width, as a percentage
*/
void ImagePropertiesWidget::on_m_width_sb_valueChanged(double value)
{
	if (!m_image || m_updating_ratio) return;

	const qreal oldScaleX = m_image->scaleFactorX();
	const qreal newScaleX = value / 100.0;
	const bool wasLocked = ui->m_lock_ratio_tb->isChecked();

	// Guards updateUi() itself here, not just the mutual spinbox
	// updates below: setScaleFactorX() emits transformChanged()
	// immediately, synchronously calling updateUi() before this
	// function gets any further -- at that exact moment scaleX has
	// already changed but scaleY hasn't caught up yet, so updateUi()'s
	// "auto-unlock if currently mismatched" check would fire and
	// uncheck the lock before the compensating height change below
	// ever runs, defeating the lock on every single use. Released
	// before the explicit updateUi() call at the end, once both axes
	// genuinely agree again (if locked) and that check is meaningful.
	m_updating_ratio = true;
	m_image->setScaleFactorX(newScaleX);

	if (wasLocked && !qFuzzyIsNull(oldScaleX))
	{
		const qreal ratio = newScaleX / oldScaleX;
		const qreal newScaleY = m_image->scaleFactorY() * ratio;
		ui->m_height_sb->setValue(newScaleY * 100.0);
		m_image->setScaleFactorY(newScaleY);
	}
	m_updating_ratio = false;
	updateUi();
}

/**
	@brief ImagePropertiesWidget::on_m_height_sb_valueChanged
	Mirror of on_m_width_sb_valueChanged() -- see its comment.
	@param value the new height, as a percentage
*/
void ImagePropertiesWidget::on_m_height_sb_valueChanged(double value)
{
	if (!m_image || m_updating_ratio) return;

	const qreal oldScaleY = m_image->scaleFactorY();
	const qreal newScaleY = value / 100.0;
	const bool wasLocked = ui->m_lock_ratio_tb->isChecked();

	m_updating_ratio = true;   // see on_m_width_sb_valueChanged()'s identical comment
	m_image->setScaleFactorY(newScaleY);

	if (wasLocked && !qFuzzyIsNull(oldScaleY))
	{
		const qreal ratio = newScaleY / oldScaleY;
		const qreal newScaleX = m_image->scaleFactorX() * ratio;
		ui->m_width_sb->setValue(newScaleX * 100.0);
		m_image->setScaleFactorX(newScaleX);
	}
	m_updating_ratio = false;
	updateUi();
}

/**
	@brief ImagePropertiesWidget::on_m_angle_sb_valueChanged
	@param value the new rotation, in degrees
*/
void ImagePropertiesWidget::on_m_angle_sb_valueChanged(double value)
{
	if (!m_image) return;
	m_image->setRotationAngle(value);
}

/**
	@brief ImagePropertiesWidget::on_m_skew_x_sb_valueChanged
	@param value the new X skew, in degrees
*/
void ImagePropertiesWidget::on_m_skew_x_sb_valueChanged(double value)
{
	if (!m_image) return;
	m_image->setSkewX(value);
}

/**
	@brief ImagePropertiesWidget::on_m_skew_y_sb_valueChanged
	@param value the new Y skew, in degrees
*/
void ImagePropertiesWidget::on_m_skew_y_sb_valueChanged(double value)
{
	if (!m_image) return;
	m_image->setSkewY(value);
}

/**
	@brief ImagePropertiesWidget::on_m_lock_pos_cb_clicked
	Set movable or not the image according to corresponding check box
*/
void ImagePropertiesWidget::on_m_lock_pos_cb_clicked()
{
	m_image->setMovable(!ui->m_lock_pos_cb->isChecked());
}

void ImagePropertiesWidget::on_m_adapt_to_dark_theme_cb_toggled(bool checked)
{
	if (!m_image) return;
	m_preview_dark_theme = true;
	m_image->setAdaptToDarkTheme(checked);
	m_preview_dark_theme = false;
}
