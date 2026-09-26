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
#include "commandsearchpopup.h"

#include "shortcutmanager.h"

#include <QAction>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScreen>
#include <QVBoxLayout>

#include <algorithm>

/**
	@brief CommandSearchPopup::CommandSearchPopup
	@param owner : the window whose commands are searched; also the parent
*/
CommandSearchPopup::CommandSearchPopup(QWidget *owner) :
	QFrame(owner, Qt::Popup)
{
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);
	setMinimumWidth(380);

	m_search = new QLineEdit(this);
	m_search->setPlaceholderText(tr("Rechercher une commande…"));
	m_search->setClearButtonEnabled(true);
	m_list = new QListWidget(this);
	m_list->setIconSize(QSize(20, 20));
	m_list->setMinimumHeight(280);
	m_list->setFocusPolicy(Qt::NoFocus);

	auto *hint = new QLabel(tr("Entrée pour lancer · Échap pour fermer"), this);
	hint->setEnabled(false);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(6, 6, 6, 6);
	layout->setSpacing(4);
	layout->addWidget(m_search);
	layout->addWidget(m_list);
	layout->addWidget(hint);

	connect(m_search, &QLineEdit::textChanged, this, &CommandSearchPopup::filter);
	connect(m_list, &QListWidget::itemClicked, this, [this]() { runCurrent(); });
}

/**
	@brief CommandSearchPopup::fold
	@return @a text lower-cased, without accents and without the "&" of
	mnemonics, for matching
*/
QString CommandSearchPopup::fold(const QString &text)
{
	QString out;
	const QString decomposed = text.normalized(QString::NormalizationForm_D);
	out.reserve(decomposed.size());
	for (const QChar c : decomposed) {
		if (c.category() != QChar::Mark_NonSpacing && c != QLatin1Char('&')) {
			out.append(c.toLower());
		}
	}
	return out;
}

/**
	@brief CommandSearchPopup::popUpAt
	Rebuild the command list -- actions come and go with windows, and their
	enabled state changes -- then show at @a global_pos, kept on screen.
*/
void CommandSearchPopup::popUpAt(const QPoint &global_pos)
{
	collect();
	m_search->clear();
	filter();

	adjustSize();
	QPoint pos = global_pos;
	if (QScreen *screen = QGuiApplication::screenAt(global_pos)) {
		const QRect avail = screen->availableGeometry();
		pos.setX(qBound(avail.left(), pos.x(), avail.right() - width()));
		pos.setY(qBound(avail.top(), pos.y(), avail.bottom() - height()));
	}
	move(pos);
	show();
	m_search->setFocus();
}

/**
	@brief CommandSearchPopup::collect
	Every command registered by the owning window, except this search.
*/
void CommandSearchPopup::collect()
{
	m_commands.clear();
	for (const ShortcutManager::ShortcutInfo &info :
	     ShortcutManager::instance().allShortcuts())
	{
		if (info.id == QLatin1String("diagrameditor.command_search")) {
			continue;
		}
		QAction *action = ShortcutManager::instance().action(info.id, parentWidget());
		if (!action || !action->isVisible()) {
			continue;
		}
		const QString text = action->text().remove(QLatin1Char('&'));
		if (text.isEmpty()) {
			continue;
		}
		m_commands.append({action, text, fold(text)});
	}
}

/**
	@brief CommandSearchPopup::filter
	Show the commands matching the search text, best first: a name starting
	with it, then a word starting with it, then containing it. An empty
	search lists everything, alphabetically.
*/
void CommandSearchPopup::filter()
{
	const QString needle = fold(m_search->text().trimmed());

	QList<QPair<int, const Command *>> hits;
	for (const Command &command : std::as_const(m_commands))
	{
		int score = 0;
		if (needle.isEmpty()) {
			score = 1;
		} else if (command.folded.startsWith(needle)) {
			score = 3;
		} else if (command.folded.contains(QLatin1Char(' ') + needle)) {
			score = 2;
		} else if (command.folded.contains(needle)) {
			score = 1;
		}
		if (score) {
			hits.append({score, &command});
		}
	}
	std::stable_sort(hits.begin(), hits.end(), [](const auto &a, const auto &b) {
		if (a.first != b.first) {
			return a.first > b.first;
		}
		return a.second->folded < b.second->folded;
	});

	m_list->clear();
	for (const auto &hit : std::as_const(hits))
	{
		QAction *action = hit.second->action;
		const QKeySequence key = action->shortcut();
		auto *item = new QListWidgetItem(
			action->icon(),
			key.isEmpty() ? hit.second->text
				      : QStringLiteral("%1   (%2)").arg(hit.second->text,
					key.toString(QKeySequence::NativeText)));
		item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void *>(action)));
		if (!action->isEnabled()) {
			item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		}
		m_list->addItem(item);
	}

		//Preselect the first command that can run, so Enter works at once
	for (int i = 0 ; i < m_list->count() ; ++i) {
		if (m_list->item(i)->flags() & Qt::ItemIsEnabled) {
			m_list->setCurrentRow(i);
			break;
		}
	}
}

/**
	@brief CommandSearchPopup::runCurrent
	Close, then trigger the highlighted command: it may open a dialog or
	start a tool on the folio, which need the focus the popup holds.
*/
void CommandSearchPopup::runCurrent()
{
	QListWidgetItem *item = m_list->currentItem();
	if (!item || !(item->flags() & Qt::ItemIsEnabled)) {
		return;
	}
	auto *action = static_cast<QAction *>(item->data(Qt::UserRole).value<void *>());
	hide();
	if (action) {
		action->trigger();
	}
}

/**
	@brief CommandSearchPopup::keyPressEvent
	Up and Down move through the list while typing goes on in the search
	field; Enter runs, Esc closes.
*/
void CommandSearchPopup::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
		case Qt::Key_Escape:
			hide();
			return;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			runCurrent();
			return;
		case Qt::Key_Down:
		case Qt::Key_Up:
		case Qt::Key_PageDown:
		case Qt::Key_PageUp:
			QCoreApplication::sendEvent(m_list, event);
			return;
		default:
			break;
	}
	QFrame::keyPressEvent(event);
}
