/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/

#include "diagrameventaddimage.h"

#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../diagram.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/diagramimageitem.h"

#include <QStatusBar>
#include <QTimer>

/**
	@brief DiagramEventAddImage::DiagramEventAddImage
	Default constructor
	@param diagram : diagram where operate this event
*/
DiagramEventAddImage::DiagramEventAddImage(Diagram *diagram) :
	DiagramEventInterface(diagram),
	m_image (nullptr),
	m_is_added (false)
{
	openDialog();
	if (m_running)
	{
		// Deferred for the same reason as the shape tools' own
		// constructor-time hint: Diagram::setEventInterface() destroys
		// whatever tool was previously active *after* this constructor
		// returns, and that tool's own destructor clears the status bar
		// -- an immediate show here would just get wiped out moments
		// later by that cleanup.
		QTimer::singleShot(0, this, [this]() { showHint(); });
	}
}

/**
	@brief DiagramEventAddImage::~DiagramEventAddImage
*/
DiagramEventAddImage::~DiagramEventAddImage()
{
	if (m_running || m_abort)
	{
		if (m_is_added) m_diagram->removeItem(m_image);
		delete m_image;
	}

	if (!m_diagram->views().isEmpty())
	{
		if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
			editor->statusBar()->clearMessage();
	}

	foreach (QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy((Qt::DefaultContextMenu));
}

/**
	@brief DiagramEventAddImage::showHint
	Re-asserted on every move (see mouseMoveEvent), not just once at
	activation: Qt's own built-in "show an action's statusTip on hover"
	has its own internal "restore whatever was there before" logic for
	when the hover ends. Since this message is first shown *during* that
	same hover session (the user is still over the toolbar icon when the
	deferred constructor-time call above fires), Qt's hover-tracking has
	no idea this code changed the status bar in the meantime -- the
	moment the mouse leaves the icon for the canvas, it silently
	restores whatever it remembers being there before its own tip
	started, overwriting this one. Re-showing it on every move within
	the canvas simply outlasts that one-time restore -- the exact same
	issue already found and fixed for the shape tools.
*/
void DiagramEventAddImage::showHint() const
{
	if (m_diagram->views().isEmpty())
		return;
	if (auto *editor = QETApp::diagramEditorAncestorOf(m_diagram->views().constFirst()))
		editor->statusBar()->showMessage(tr("Clic : positionner à la taille d'origine. "
		                                     "Cliquer-glisser : positionner et redimensionner. "
		                                     "Clic droit : pivoter de 90°. Ctrl+molette : ajuster la taille."));
}

/**
	@brief DiagramEventAddImage::mousePressEvent
	Left button: starts a potential drag-to-resize, anchored here -- but
	doesn't commit to anything yet. A quick click-release (see
	mouseMoveEvent's threshold check) still places the image at its
	original size, matching the previous behavior exactly; only an
	actual drag switches to resizing. Right button still rotates in 90
	degree steps, unchanged, and only while not already left-dragging.
	@param event : event of mouse pressed
*/
void DiagramEventAddImage::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_image && event->button() == Qt::LeftButton)
	{
		m_pressed = true;
		m_resize_engaged = false;
		m_press_pos = event->scenePos();
		event->setAccepted(true);
	}
	else if (m_image && !m_pressed && event->button() == Qt::RightButton)
	{
		m_image->setRotation(m_image->rotation() + 90);
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddImage::mouseMoveEvent
	Action when mouse move
	@param event : event of mouse move
*/
void DiagramEventAddImage::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_image) {
		return;
	}

	showHint();

	QPointF pos = event->scenePos();

	if (!m_is_added)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::NoContextMenu));
		}

		m_diagram->addItem(m_image);
		m_is_added = true;
	}

	if (m_pressed)
	{
		// Anchored on m_press_pos, not the item's own current position:
		// dragging in any direction has to visibly grow the image from
		// where the click started, not from wherever the "no button
		// held" preview phase happened to leave it centered.
		const QPointF delta = pos - m_press_pos;
		if (!m_resize_engaged && QLineF(m_press_pos, pos).length() >= 4.0)
			m_resize_engaged = true;   // latched: crossing back within the threshold afterward must not un-engage it

		if (!m_resize_engaged)
		{
			// Still just a (so far) plain click -- keep behaving like
			// the pre-drag preview: original size, centered here, so
			// releasing right now reproduces the old click-to-place
			// behavior exactly.
			m_image->setPos(m_press_pos - m_image->boundingRect().center());
		}
		else
		{
			const QSizeF naturalSize = m_image->boundingRect().size();
			if (naturalSize.width() > 0 && naturalSize.height() > 0)
			{
				const qreal scaleX = qAbs(delta.x()) / naturalSize.width();
				const qreal scaleY = qAbs(delta.y()) / naturalSize.height();
				// The larger of the two, not a per-axis stretch: images
				// only support a single uniform scale today (see
				// boundingRect()/paint(), which never touch aspect
				// ratio), so this is a diagonal-drag size, not a
				// free-form one -- breaking aspect ratio on purpose is
				// its own, separate, larger piece of work.
				const qreal newScale = qBound(0.01, qMax(scaleX, scaleY), 50.0);
				m_image->setScale(newScale);
			}
			m_image->setPos(qMin(m_press_pos.x(), pos.x()), qMin(m_press_pos.y(), pos.y()));
		}
	}
	else
	{
		m_image->setPos(pos - m_image->boundingRect().center());
	}

	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::mouseReleaseEvent
	Left button release commits whatever mouseMoveEvent last set --
	original size and centered if the press never turned into a real
	drag, or the dragged-out size and position otherwise. Either way,
	this is the only place placement is actually finalized now; a plain
	click no longer finishes inside mousePressEvent itself, since it has
	to wait and see whether a drag follows.
	@param event : event of mouse release
*/
void DiagramEventAddImage::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_image && m_pressed && event->button() == Qt::LeftButton)
	{
		m_diagram->undoStack().push(new AddGraphicsObjectCommand(m_image, m_diagram, m_image->pos()));

		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::DefaultContextMenu));
		}

		m_running = false;
		emit finish();
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddImage::mouseDoubleClickEvent
	This method is used only to overwrite double click.
	When double click, image properties dialog isn't open.
	@param event : event of mouse double click.
*/
void DiagramEventAddImage::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::wheelEvent
	Action when mouse wheel is rotated
	@param event: evet of mouse wheel
*/
void DiagramEventAddImage::wheelEvent(QGraphicsSceneWheelEvent *event)
{
	// !m_pressed added alongside the modifier fix: without it, wheel
	// scaling could fight with an active drag-resize, both trying to
	// set scale() from different sources in the same gesture.
	// event->modifiers() & Qt::ControlModifier, not != Qt::CTRL: the
	// same exact-equality bug already found and fixed several times
	// this session elsewhere -- Ctrl held together with any other
	// modifier would silently fail to register as Ctrl at all.
	if (!m_is_added || !m_image || m_pressed || !(event->modifiers() & Qt::ControlModifier)) {
		return;
	}
	
	qreal scaling = m_image->scale();
	event->delta() > 1? scaling += 0.01 : scaling -= 0.01;
	if (scaling>0.01 && scaling <= 2) {
		m_image->setScale(scaling);
	}
	
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddImage::isNull
	@return : true if image can't be loaded, otherwise return false.
*/
bool DiagramEventAddImage::isNull() const
{
	if (!m_image) return true;
	return false;
}

/**
	@brief DiagramEventAddImage::openDialog
	Open dialog to select the image to add.
*/
void DiagramEventAddImage::openDialog()
{
	if (m_diagram -> isReadOnly()) return;
	
	//Open dialog to select image
	QString pathPictures = QETApp::pictureDir();
	QString fileName = QFileDialog::getOpenFileName(m_diagram->views().isEmpty()? nullptr : m_diagram->views().first(), QObject::tr("Selectionner une image..."), pathPictures, QObject::tr("Image Files (*.png *.jpg  *.jpeg *.bmp *.svg)"));
	
	if (fileName.isEmpty()) return;
	
	QImage image(fileName);
	if(image.isNull())
	{
		QMessageBox::critical(m_diagram->views().isEmpty()? nullptr : m_diagram->views().first(), QObject::tr("Erreur"), QObject::tr("Impossible de charger l'image."));
		return;
	}
	
	m_image = new DiagramImageItem (QPixmap::fromImage(image));
	m_running = true;
}
