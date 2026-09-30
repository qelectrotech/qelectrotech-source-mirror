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
	setWindowTitle(tr("Connecter un assistant IA"));
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
		tr("Un assistant IA (Claude, GitHub Copilot, Gemini…) peut ouvrir, "
		   "vérifier et modifier vos schémas grâce au serveur MCP de "
		   "QElectroTech, qui fonctionne sur cet ordinateur. Copiez le texte "
		   "ci-dessous dans la configuration de votre assistant. "
		   "<a href=\"%1\">Guide détaillé</a>").arg(QLatin1String(GUIDE_URL)),
		this);
	intro->setWordWrap(true);
	intro->setOpenExternalLinks(true);
	layout->addWidget(intro);

	if (m_paths.server.isEmpty()) {
		auto *missing = new QLabel(
			tr("<b>Le serveur MCP n'est pas installé avec cette version de "
			   "QElectroTech.</b> Le guide explique comment l'obtenir."),
			this);
		missing->setWordWrap(true);
		layout->addWidget(missing);
	}

	if (!m_paths.server.isEmpty()
	    && m_paths.python_status != AiAssistantSetup::PythonStatus::Found) {
		QString text;
		if (m_paths.python_status == AiAssistantSetup::PythonStatus::StoreShortcut)
			text = tr("<b>Python n'est peut-être pas installé.</b> Seul le raccourci "
				  "« python » du Microsoft Store a été trouvé : sans Python, il "
				  "ouvre le Store au lieu de lancer le serveur.");
		else
			text = tr("<b>Python est introuvable sur cet ordinateur</b> (commande "
				  "« %1 »). Le serveur en a besoin.").arg(m_paths.python);
		if (windows)
			text += QLatin1Char(' ') + tr("Relancez l'installateur de QElectroTech et "
						      "cochez « Python pour l'assistant IA », ou "
						      "installez Python depuis python.org.");
		else
			text += QLatin1Char(' ') + tr("Installez Python 3 avec le gestionnaire "
						      "de paquets de votre système.");
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
	form->addRow(tr("Assistant :"), m_client);

	auto *workspace_row = new QHBoxLayout();
	m_workspace = new QLineEdit(this);
	m_workspace->setPlaceholderText(tr("Le dossier de vos schémas"));
	auto *browse = new QPushButton(tr("Parcourir…"), this);
	workspace_row->addWidget(m_workspace);
	workspace_row->addWidget(browse);
	form->addRow(tr("Dossier accessible :"), workspace_row);

	m_allow_edit = new QCheckBox(tr("Autoriser l'assistant à modifier les schémas"), this);
	form->addRow(QString(), m_allow_edit);
	layout->addLayout(form);

	auto *scope = new QLabel(
		tr("L'assistant ne peut lire et écrire que dans ce dossier. Sans "
		   "modification autorisée, il peut seulement lire, comparer et "
		   "exporter. Un assistant lit le texte des projets (repères, "
		   "notes…) : un texte écrit comme une instruction peut "
		   "l'influencer. Laissez les modifications désactivées pour les "
		   "schémas reçus d'autres personnes."),
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
	m_copy = buttons->addButton(tr("Copier"), QDialogButtonBox::ActionRole);
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
			 + tr("Si le fichier contient déjà d'autres serveurs, "
			      "ajoutez seulement l'entrée « qet »."));

	const QString workspace = m_workspace->text().trimmed();
	const bool ready = !m_paths.server.isEmpty() && !workspace.isEmpty();
	m_copy->setEnabled(ready);
	if (m_paths.server.isEmpty())
		m_text->setPlainText(QString());
	else if (workspace.isEmpty())
		m_text->setPlainText(tr("Choisissez d'abord le dossier de vos schémas."));
	else
		m_text->setPlainText(AiAssistantSetup::configuration(
			client, m_paths, workspace, m_allow_edit->isChecked()));
}

void AiAssistantDialog::chooseWorkspace()
{
	const QString dir = QFileDialog::getExistingDirectory(
		this, tr("Dossier accessible à l'assistant"), m_workspace->text());
	if (!dir.isEmpty())
		m_workspace->setText(QDir::toNativeSeparators(dir));
}

void AiAssistantDialog::copy()
{
	QApplication::clipboard()->setText(m_text->toPlainText());
	m_copy->setText(tr("Copié"));
}

QString AiAssistantDialog::whereItGoes(Client client) const
{
	switch (client) {
		case Client::ClaudeDesktop:
			return tr("Dans Claude Desktop : Paramètres → Développeur → "
				  "Modifier la configuration. Puis quittez et relancez Claude.");
		case Client::ClaudeCode:
			return tr("Enregistrez-le sous le nom .mcp.json dans le dossier de vos schémas.");
		case Client::VsCode:
			return tr("Enregistrez-le sous le nom .vscode/mcp.json dans le dossier "
				  "ouvert dans VS Code. Copilot utilise les outils en mode agent.");
		case Client::Cursor:
			return tr("Ajoutez-le au fichier .cursor/mcp.json de votre dossier "
				  "personnel.");
		case Client::GeminiCli:
			return tr("Ajoutez-le au fichier .gemini/settings.json de votre dossier "
				  "personnel. Gemini CLI demande de faire confiance au dossier "
				  "la première fois.");
		case Client::CodexCli:
			return tr("Ajoutez-le au fichier .codex/config.toml de votre dossier "
				  "personnel.");
		case Client::LmStudio:
			return tr("Dans LM Studio : onglet Program → Install → Edit mcp.json.");
	}
	return QString();
}
