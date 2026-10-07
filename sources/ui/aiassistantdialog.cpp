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
#include "aiassistantdialog.h"
#include "../qet.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

using AiAssistantSetup::Client;

namespace {
const char *const GUIDE_URL =
	"https://github.com/qelectrotech/qelectrotech-source-mirror/wiki/ai_assistants";
}

AiAssistantDialog::AiAssistantDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Connect an AI assistant"));
	resize(720, 560);

#ifdef Q_OS_WIN
	const bool windows = true;
#else
	const bool windows = false;
#endif
	m_paths = AiAssistantSetup::detect(QCoreApplication::applicationDirPath(),
					   QCoreApplication::applicationFilePath(),
					   windows);

	auto *layout = new QVBoxLayout(this);

	auto *intro = new QLabel(
		tr("An AI assistant (Claude, GitHub Copilot, Gemini…) can open, check "
		   "and edit your diagrams through QElectroTech's MCP server, which "
		   "runs on this computer. Copy the text below into your assistant's "
		   "configuration. <a href=\"%1\">Detailed guide</a>").arg(QLatin1String(GUIDE_URL)),
		this);
	intro->setWordWrap(true);
	intro->setOpenExternalLinks(true);
	layout->addWidget(intro);

	if (m_paths.server.isEmpty()) {
		auto *missing = new QLabel(
			tr("<b>The MCP server is not installed with this version of "
			   "QElectroTech.</b> The guide explains how to get it."),
			this);
		missing->setWordWrap(true);
		layout->addWidget(missing);
	}

	if (!m_paths.server.isEmpty()
	    && m_paths.python_status != AiAssistantSetup::PythonStatus::Found) {
		QString text;
		if (m_paths.python_status == AiAssistantSetup::PythonStatus::StoreShortcut)
			text = tr("<b>Python may not be installed.</b> Only the Microsoft Store "
				  "“python” shortcut was found: without Python, it opens the "
				  "Store instead of starting the server.");
		else
			text = tr("<b>Python was not found on this computer</b> (command "
				  "“%1”). The server needs it.").arg(m_paths.python);
		if (windows)
			text += QLatin1Char(' ') + tr("Run the QElectroTech installer again and "
						      "tick “Python for the AI assistant”, or "
						      "install Python from python.org.");
		else
			text += QLatin1Char(' ') + tr("Install Python 3 with your system's "
						      "package manager.");
		auto *python = new QLabel(text, this);
		python->setWordWrap(true);
		layout->addWidget(python);
	}

	auto *form = new QFormLayout();
	m_client = new QComboBox(this);
	m_client->addItem(QStringLiteral("Claude Desktop"), int(Client::ClaudeDesktop));
	m_client->addItem(QStringLiteral("Claude Code"), int(Client::ClaudeCode));
	m_client->addItem(QStringLiteral("GitHub Copilot (VS Code)"), int(Client::VsCode));
	m_client->addItem(QStringLiteral("Cursor"), int(Client::Cursor));
	m_client->addItem(QStringLiteral("Gemini CLI"), int(Client::GeminiCli));
	m_client->addItem(QStringLiteral("Codex CLI"), int(Client::CodexCli));
	m_client->addItem(QStringLiteral("LM Studio"), int(Client::LmStudio));
	form->addRow(tr("Assistant:"), m_client);

	auto *workspace_row = new QHBoxLayout();
	m_workspace = new QLineEdit(this);
	m_workspace->setPlaceholderText(tr("Your diagrams folder"));
	auto *browse = new QPushButton(tr("Browse…"), this);
	workspace_row->addWidget(m_workspace);
	workspace_row->addWidget(browse);
	form->addRow(tr("Folder it can use:"), workspace_row);

	m_allow_edit = new QCheckBox(tr("Allow the assistant to edit diagrams"), this);
	form->addRow(QString(), m_allow_edit);
	layout->addLayout(form);

	auto *scope = new QLabel(
		tr("The assistant can only read and write in this folder. Without "
		   "editing allowed, it can only read, compare and export. An "
		   "assistant reads the text in projects (labels, notes…): text "
		   "written as an instruction can influence it. Leave editing off "
		   "for diagrams you received from other people."),
		this);
	scope->setWordWrap(true);
	layout->addWidget(scope);

	m_where = new QLabel(this);
	m_where->setWordWrap(true);
	m_where->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(m_where);

	m_text = new QPlainTextEdit(this);
	m_text->setReadOnly(true);
	m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	layout->addWidget(m_text);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	m_copy = buttons->addButton(tr("Copy"), QDialogButtonBox::ActionRole);
	layout->addWidget(buttons);

	connect(m_client, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &AiAssistantDialog::refresh);
	connect(m_workspace, &QLineEdit::textChanged, this, &AiAssistantDialog::refresh);
	connect(m_allow_edit, &QCheckBox::toggled, this, &AiAssistantDialog::refresh);
	connect(browse, &QPushButton::clicked, this, &AiAssistantDialog::chooseWorkspace);
	connect(m_copy, &QPushButton::clicked, this, &AiAssistantDialog::copy);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	refresh();

	QET::trackDialogGeometry(this);
}

void AiAssistantDialog::refresh()
{
	const auto client = Client(m_client->currentData().toInt());
	m_where->setText(whereItGoes(client) + QLatin1Char(' ')
			 + tr("If the file already lists other servers, add "
			      "only the “qet” entry."));

	const QString workspace = m_workspace->text().trimmed();
	const bool ready = !m_paths.server.isEmpty() && !workspace.isEmpty();
	m_copy->setEnabled(ready);
	if (m_paths.server.isEmpty())
		m_text->setPlainText(QString());
	else if (workspace.isEmpty())
		m_text->setPlainText(tr("Choose your diagrams folder first."));
	else
		m_text->setPlainText(AiAssistantSetup::configuration(
			client, m_paths, workspace, m_allow_edit->isChecked()));
}

void AiAssistantDialog::chooseWorkspace()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr("Folder the assistant can use"), m_workspace->text());
	if (!dir.isEmpty())
		m_workspace->setText(QDir::toNativeSeparators(dir));
}

void AiAssistantDialog::copy()
{
	QApplication::clipboard()->setText(m_text->toPlainText());
	m_copy->setText(tr("Copied"));
}

QString AiAssistantDialog::whereItGoes(Client client) const
{
	switch (client) {
		case Client::ClaudeDesktop:
			return tr("In Claude Desktop: Settings → Developer → Edit Config. Then "
				  "quit and restart Claude.");
		case Client::ClaudeCode:
			return tr("Save it as .mcp.json in your diagrams folder.");
		case Client::VsCode:
			return tr("Save it as .vscode/mcp.json in the folder open in VS Code. "
				  "Copilot uses the tools in agent mode.");
		case Client::Cursor:
			return tr("Add it to the .cursor/mcp.json file in your home folder.");
		case Client::GeminiCli:
			return tr("Add it to the .gemini/settings.json file in your home folder. "
				  "Gemini CLI asks you to trust the folder the first time.");
		case Client::CodexCli:
			return tr("Add it to the .codex/config.toml file in your home folder.");
		case Client::LmStudio:
			return tr("In LM Studio: Program tab → Install → Edit mcp.json.");
	}
	return QString();
}
