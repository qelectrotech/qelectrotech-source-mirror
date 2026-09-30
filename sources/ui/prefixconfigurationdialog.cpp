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
#include "prefixconfigurationdialog.h"

#include "../ElementsCollection/qetlabelsfile.h"

#include <QDialogButtonBox>
#include <QHash>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

/**
	@brief PrefixConfigurationDialog::PrefixConfigurationDialog
	@param labels : the loaded labels file the dialog edits
	@param folders : every directory of the collection, as returned by
		QetLabelsFile::scanFolders()
	@param parent
*/
PrefixConfigurationDialog::PrefixConfigurationDialog(QetLabelsFile &labels, const QList<QStringList> &folders, QWidget *parent) :
	QDialog(parent),
	m_labels(labels),
	m_folders(folders),
	m_tree(new QTreeWidget(this))
{
	setWindowTitle(tr("Préfixes de la collection utilisateur", "title of the dialog configuring the prefixes of the user collection"));
	setModal(true);
	resize(700, 500);

	auto *layout = new QVBoxLayout(this);

	auto *hint = new QLabel(this);
	hint->setWordWrap(true);
	hint->setText(tr("Chaque dossier de la collection possède un préfixe : il est ajouté devant l'étiquette des éléments du dossier.\n"
					 "Un champ vide signifie que le dossier reprend le préfixe de son dossier parent."));
	layout->addWidget(hint);

	m_tree->setColumnCount(2);
	m_tree->setHeaderLabels(QStringList() << tr("Dossier", "column header of the folder tree") << tr("Préfixe", "column header of the prefix column"));
	m_tree->setRootIsDecorated(true);
	m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tree->setSelectionMode(QAbstractItemView::NoSelection);
	m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	layout->addWidget(m_tree);

	auto *expand_pb = new QPushButton(tr("Tout déplier"), this);
	auto *collapse_pb = new QPushButton(tr("Tout replier"), this);
	connect(expand_pb, &QPushButton::clicked, m_tree, &QTreeWidget::expandAll);
	connect(collapse_pb, &QPushButton::clicked, m_tree, &QTreeWidget::collapseAll);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &PrefixConfigurationDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &PrefixConfigurationDialog::reject);

	auto *bottom_layout = new QHBoxLayout();
	bottom_layout->addWidget(expand_pb);
	bottom_layout->addWidget(collapse_pb);
	bottom_layout->addStretch();
	bottom_layout->addWidget(buttons);
	layout->addLayout(bottom_layout);

		//Asking before the tree is shown keeps the answer from appearing
		//in the middle of a dialog the user has not seen yet.
	askWhatToDoWithOrphans();
	buildTree();
}

PrefixConfigurationDialog::~PrefixConfigurationDialog()
{
}

/**
	@brief PrefixConfigurationDialog::askWhatToDoWithOrphans
	When the labels file describes directories the collection does not
	have (deleted or renamed folders, or entries written by hand in an
	other layout), the user chooses whether they are kept or dropped.
	The choice is only applied by accept().
*/
void PrefixConfigurationDialog::askWhatToDoWithOrphans()
{
	const QStringList orphans = m_labels.orphanPaths(m_folders);
	if (orphans.isEmpty()) {
		return;
	}

	QMessageBox box(QMessageBox::Question,
					tr("Entrées sans dossier"),
					tr("%n entrée(s) de qet_labels.xml ne correspond à aucun dossier de la collection :\n"
					   "les conserver ou les supprimer ?", nullptr, static_cast<int>(orphans.size())),
					QMessageBox::NoButton,
					parentWidget());
	auto *keep_button = box.addButton(tr("Conserver"), QMessageBox::AcceptRole);
	box.addButton(tr("Supprimer"), QMessageBox::DestructiveRole);
	box.setDetailedText(orphans.join(QLatin1Char('\n')));
	box.exec();

		//Closing the box without choosing keeps the entries : deleting
		//data must always be an explicit decision.
	m_remove_orphans = box.clickedButton() != nullptr
			&& box.clickedButton() != keep_button;
}

/**
	@brief PrefixConfigurationDialog::buildTree
	One item per directory of the collection, nested the way the
	directories are, each with a line edit holding the prefix already
	stored for that directory.
*/
void PrefixConfigurationDialog::buildTree()
{
	m_tree->setUpdatesEnabled(false);

	QHash<QString, QTreeWidgetItem *> known;
	for (const QStringList &folder : m_folders) {
		QTreeWidgetItem *parent_item = nullptr;
		QString built;
		for (const QString &name : folder) {
			built = built.isEmpty() ? name : built + QLatin1Char('/') + name;

			QTreeWidgetItem *item = known.value(built, nullptr);
			if (item == nullptr) {
				item = parent_item == nullptr
						? new QTreeWidgetItem(m_tree)
						: new QTreeWidgetItem(parent_item);
				item->setText(0, name);
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
				known.insert(built, item);
			}
			parent_item = item;
		}
		if (parent_item == nullptr) {
			continue;
		}
		parent_item->setData(0, Qt::UserRole, folder);
			//What the file holds for that folder, kept to tell a row the
			//user touched from one they left alone (@see accept())
		parent_item->setData(1, Qt::UserRole, m_labels.prefix(folder).trimmed());

		auto *edit = new QLineEdit(m_labels.prefix(folder).trimmed(), m_tree);
		edit->setClearButtonEnabled(true);
			//A folder whose file holds an explicit <prefix/> shows an empty
			//field too, but does not inherit : say so instead of promising
			//an inheritance that will not happen
		edit->setPlaceholderText(m_labels.hasPrefix(folder)
								 ? tr("aucun préfixe : n'hérite pas du parent",
									  "placeholder of an empty prefix field whose folder explicitly has no prefix, which cancels the inheritance")
								 : tr("hériter du dossier parent", "placeholder of an empty prefix field"));
		connect(edit, &QLineEdit::textEdited, this, [parent_item, edit]() {
				//Once the user has typed in the field, whatever it holds
				//when OK is pressed is what the folder gets - an emptied
				//field then means "inherit" again, even when the file had
				//an explicit <prefix/>
			parent_item->setData(1, Qt::UserRole + 1, true);
			edit->setPlaceholderText(tr("hériter du dossier parent", "placeholder of an empty prefix field"));
		});
		edit->installEventFilter(this);
		m_tree->setItemWidget(parent_item, 1, edit);
	}

	m_tree->expandAll();
	m_tree->setUpdatesEnabled(true);
}

/**
	@brief PrefixConfigurationDialog::accept
	Apply every prefix shown in the dialog, add the missing categories,
	optionally drop the entries the user agreed to delete, then write the
	file. When the write fails the dialog stays open so nothing typed in
	it is lost.
*/
void PrefixConfigurationDialog::accept()
{
	m_labels.ensureStructure(m_folders);

	for (QTreeWidgetItemIterator iterator(m_tree) ; *iterator ; ++iterator) {
		QTreeWidgetItem *item = *iterator;
		auto *edit = qobject_cast<QLineEdit *>(m_tree->itemWidget(item, 1));
		if (edit == nullptr) {
			continue;
		}
		const QString text = edit->text().trimmed();
			//A row the user did not touch is left exactly as the file has
			//it. Writing every field back would turn an explicit
			//<prefix/>, which shows empty and cancels the inheritance,
			//into no <prefix> at all, and that folder would silently start
			//inheriting its parent's prefix again.
		if (!item->data(1, Qt::UserRole + 1).toBool()
			&& text == item->data(1, Qt::UserRole).toString()) {
			continue;
		}
		m_labels.setPrefix(item->data(0, Qt::UserRole).toStringList(), text);
	}

	if (m_remove_orphans) {
		m_labels.removeOrphans(m_folders);
	}

	if (!m_labels.save()) {
		QMessageBox::critical(this,
							  tr("Enregistrement impossible"),
							  tr("Le fichier %1 n'a pas pu être enregistré :\n%2")
							  .arg(m_labels.filePath(), m_labels.errorString()));
		return;
	}

	if (!m_labels.backupPath().isEmpty()) {
		QMessageBox::information(this,
								 tr("Fichier endommagé remplacé"),
								 tr("Le fichier %1 était illisible : il a été remplacé.\nSa copie a été conservée sous :\n%2")
								 .arg(m_labels.filePath(), m_labels.backupPath()));
	}

	QDialog::accept();
}

/**
	@brief PrefixConfigurationDialog::eventFilter
	Return and Enter are swallowed while a prefix field has the focus :
	otherwise the event would reach the dialog and immediately validate
	it, closing a dialog the user is still filling in.
*/
bool PrefixConfigurationDialog::eventFilter(QObject *watched, QEvent *event)
{
	if (event->type() == QEvent::KeyPress && qobject_cast<QLineEdit *>(watched) != nullptr) {
		const int key = static_cast<QKeyEvent *>(event)->key();
		if (key == Qt::Key_Return || key == Qt::Key_Enter) {
			return true;
		}
	}
	return QDialog::eventFilter(watched, event);
}
