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
#ifndef PREFIXCONFIGURATIONDIALOG_H
#define PREFIXCONFIGURATIONDIALOG_H

#include <QDialog>
#include <QList>
#include <QStringList>

class QetLabelsFile;
class QTreeWidget;

/**
	@brief The PrefixConfigurationDialog class
	
	Shows the whole directory tree of the user elements collection, one
	text field per directory, filled with the prefix that directory
	already has in qet_labels.xml (empty when it has none, which means it
	inherits the prefix of its parent directory).
	
	Nothing is written while the dialog is open : validating calls
	QetLabelsFile::save(), cancelling leaves the file exactly as it was.
*/
class PrefixConfigurationDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit PrefixConfigurationDialog(QetLabelsFile &labels, const QList<QStringList> &folders, QWidget *parent = nullptr);
		~PrefixConfigurationDialog() override;

	public slots:
		void accept() override;

	private:
		void askWhatToDoWithOrphans();
		void buildTree();
	
	protected:
		bool eventFilter(QObject *watched, QEvent *event) override;

	// attributes
	private:
		QetLabelsFile &m_labels;
		QList<QStringList> m_folders;
		QTreeWidget *m_tree;
		bool m_remove_orphans = false;
};

#endif // PREFIXCONFIGURATIONDIALOG_H
