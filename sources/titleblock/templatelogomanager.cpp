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
#include "templatelogomanager.h"
#include "titleblocktemplate.h"
#include "qeticons.h"

/**
	Constructor
	@param managed_template Title block template this widget manages logos for.
	@param parent Parent QWidget.
*/
TitleBlockTemplateLogoManager::TitleBlockTemplateLogoManager(TitleBlockTemplate *managed_template, QWidget *parent) :
	QWidget(parent),
	managed_template_(managed_template)
{
	initWidgets();
	fillView();
}

/**
	Destructor
*/
TitleBlockTemplateLogoManager::~TitleBlockTemplateLogoManager() {
}

/**
	@return the name of the currently selected logo, or a null QString if none
	is selected.
*/
QString TitleBlockTemplateLogoManager::currentLogo() const {
	if (!managed_template_) return QString();
	
	QListWidgetItem *current_item = logos_view_ -> currentItem();
	if (!current_item) return QString();
	
	return(current_item -> text());
}

/**
	@return Whether this logo manager should allow logo edition
	(renaming, addition, deletion).
*/
bool TitleBlockTemplateLogoManager::isReadOnly() const {
	return(read_only_);
}

/**
	Emit the logosChanged() signal.
*/
void TitleBlockTemplateLogoManager::emitLogosChangedSignal() {
	emit(logosChanged(const_cast<const TitleBlockTemplate *>(managed_template_)));
}

/**
	Initialize widgets composing the Logo manager
*/
void TitleBlockTemplateLogoManager::initWidgets() {
	open_dialog_dir_ = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
	
	setWindowTitle(tr("Gestionnaire de logos"));
	setWindowIcon(QET::Icons::InsertImage);
	setWindowFlags(Qt::Dialog);
	logos_label_ = new QLabel(tr("Logos embarqu\351s dans ce mod\350le :"));
	logos_view_ = new QListWidget();
	logos_view_ -> setViewMode(QListView::IconMode);
	logos_view_ -> setGridSize(iconsize() * 1.4);
	logos_view_ -> setMinimumSize(iconsize() * 2.9);
	logos_view_ -> setIconSize(iconsize());
	logos_view_ -> setWrapping(true);
	logos_view_ -> setMovement(QListView::Static);
	logos_view_ -> setResizeMode(QListView::Adjust);
	add_button_ = new QPushButton(QET::Icons::Add, tr("Ajouter un logo"));
	export_button_ = new QPushButton(QET::Icons::DocumentExport, tr("Exporter ce logo"));
	delete_button_ = new QPushButton(QET::Icons::Remove, tr("Supprimer ce logo"));
	logo_box_ = new QGroupBox(tr("Propri\351t\351s"));
	logo_name_label_ = new QLabel(tr("Nom :"));
	logo_name_ = new QLineEdit();
	rename_button_ = new QPushButton(QET::Icons::EditRename, tr("Renommer"));
	logo_type_ = new QLabel(tr("Type :"));
	buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok);
	
	hlayout1_ = new QHBoxLayout();
	hlayout1_ -> addWidget(logo_name_label_);
	hlayout1_ -> addWidget(logo_name_);
	hlayout1_ -> addWidget(rename_button_);
	
	hlayout0_ = new QHBoxLayout();
	hlayout0_ -> addWidget(export_button_);
	hlayout0_ -> addWidget(delete_button_);
	
	vlayout1_ = new QVBoxLayout();
	vlayout1_ -> addLayout(hlayout1_);
	vlayout1_ -> addWidget(logo_type_);
	logo_box_ -> setLayout(vlayout1_);
	
	vlayout0_ = new QVBoxLayout();
	vlayout0_ -> addWidget(logos_label_);
	vlayout0_ -> addWidget(logos_view_);
	vlayout0_ -> addWidget(add_button_);
	vlayout0_ -> addLayout(hlayout0_);
	vlayout0_ -> addWidget(logo_box_);
	setLayout(vlayout0_);
	
	connect(
		logos_view_,
		SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		this,
		SLOT(updateLogoInformations(QListWidgetItem *, QListWidgetItem *))
	);
	connect(add_button_, SIGNAL(released()), this, SLOT(addLogo()));
	connect(export_button_, SIGNAL(released()), this, SLOT(exportLogo()));
	connect(delete_button_, SIGNAL(released()), this, SLOT(removeLogo()));
	connect(rename_button_, SIGNAL(released()), this, SLOT(renameLogo()));
}

/**
	Update the logos display.
*/
void TitleBlockTemplateLogoManager::fillView() {
	if (!managed_template_) return;
	logos_view_ -> clear();
	
	foreach (QString logo_name, managed_template_ -> logos()) {
		QIcon current_icon;
		QPixmap current_logo = managed_template_ -> bitmapLogo(logo_name);
		if (!current_logo.isNull()) {
			current_icon = QIcon(current_logo);
		} else {
			QSvgRenderer *svg_logo = managed_template_ -> vectorLogo(logo_name);
			if (svg_logo) {
				QPixmap *svg_pixmap = new QPixmap(iconsize());
				svg_pixmap -> fill();
				QPainter p;
				p.begin(svg_pixmap);
				svg_logo -> render(&p);
				p.end();
				current_icon = QIcon(*svg_pixmap);
			}
		}
		QListWidgetItem *qlwi = new QListWidgetItem(current_icon, logo_name);
		qlwi -> setTextAlignment(Qt::AlignBottom | Qt::AlignHCenter);
		logos_view_ -> insertItem(0, qlwi);
	}
	
	QListWidgetItem *current_item = logos_view_ -> currentItem();
	updateLogoInformations(current_item, 0);
}

/**
	@return the icon size to display the logos embedded within the managed
	template.
*/
QSize TitleBlockTemplateLogoManager::iconsize() const {
	return(QSize(80, 80));
}

/**
	When adding a logo, it may occur its name is already used by another
	pre-existing logo. This method asks users whether they want to erase the
	existing logo, change the initial name or simply cancel the operation.
	@param initial_name Initial name of the logo to be added
	@return Either a null QString if the user cancelled the operation, or the
	name to be used when adding the logo.
*/
QString TitleBlockTemplateLogoManager::confirmLogoName(const QString &initial_name) {
	QString name = initial_name;
	QDialog *rename_dialog = 0;
	QLabel *rd_label = 0;
	QLineEdit *rd_input = 0;
	while (managed_template_ -> logos().contains(name)) {
		if (!rename_dialog) {
			rename_dialog = new QDialog(this);
			rename_dialog -> setWindowTitle(tr("Logo d\351j\340 existant"));
			
			rd_label = new QLabel();
			rd_label -> setWordWrap(true);
			rd_input = new QLineEdit();
			QDialogButtonBox *rd_buttons = new QDialogButtonBox();
			QPushButton *replace_button = rd_buttons -> addButton(tr("Remplacer"), QDialogButtonBox::YesRole);
			QPushButton *rename_button  = rd_buttons -> addButton(tr("Renommer"),  QDialogButtonBox::NoRole);
			QPushButton *cancel_button  = rd_buttons -> addButton(QDialogButtonBox::Cancel);
			
			QVBoxLayout *rd_vlayout0 = new QVBoxLayout();
			rd_vlayout0 -> addWidget(rd_label);
			rd_vlayout0 -> addWidget(rd_input);
			rd_vlayout0 -> addWidget(rd_buttons);
			rename_dialog -> setLayout(rd_vlayout0);
			
			QSignalMapper *signal_mapper = new QSignalMapper(rename_dialog);
			signal_mapper -> setMapping(replace_button, QDialogButtonBox::YesRole);
			signal_mapper -> setMapping(rename_button,  QDialogButtonBox::NoRole);
			signal_mapper -> setMapping(cancel_button,  QDialogButtonBox::RejectRole);
			connect(replace_button, SIGNAL(clicked()), signal_mapper, SLOT(map()));
			connect(rename_button,  SIGNAL(clicked()), signal_mapper, SLOT(map()));
			connect(cancel_button,  SIGNAL(clicked()), signal_mapper, SLOT(map()));
			connect(signal_mapper, SIGNAL(mapped(int)), rename_dialog, SLOT(done(int)));
		}
		rd_label -> setText(
			QString(tr(
				"Il existe d\351j\340 un logo portant le nom \"%1\" au sein de "
				"ce mod\350le de cartouche. Voulez-vous le remplacer ou "
				"pr\351f\351rez-vous sp\351cifier un autre nom pour ce nouveau "
				"logo ?"
			)).arg(name)
		);
		rd_input -> setText(name);
		int answer = rename_dialog -> exec();
		if (answer == QDialogButtonBox::YesRole) {
			// we can use the initial name
			break;
		} else if (answer == QDialogButtonBox::NoRole) {
			// the user provided another name
			name = rd_input -> text();
			/// TODO prevent the user from entering an empty name
		} else {
			// the user cancelled the operation
			return(QString());
		}
	};
	return(name);
}

/**
	Update the displayed informations relative to the currently selected logo.
	@param current  Newly selected logo item
	@param previous Previously selected logo item
*/
void TitleBlockTemplateLogoManager::updateLogoInformations(QListWidgetItem *current, QListWidgetItem *previous) {
	Q_UNUSED(previous);
	if (current) {
		QString logo_name = current -> text();
		logo_name_ -> setText(logo_name);
		if (managed_template_) {
			QString logo_type = managed_template_ -> logoType(logo_name);
			logo_type_ -> setText(tr("Type : %1").arg(logo_type));
		}
	} else {
		logo_name_ -> setText(QString());
		logo_type_ -> setText(tr("Type :"));
	}
}

/**
	Ask the user for a filepath, and add it as a new logo in the managed
	template.
*/
void TitleBlockTemplateLogoManager::addLogo() {
	if (!managed_template_) return;
	
	QString filepath = QFileDialog::getOpenFileName(
		this,
		tr("Choisir une image / un logo"),
		open_dialog_dir_.absolutePath(),
		tr("Images vectorielles (*.svg);;Images bitmap (*.png *.jpg *.jpeg *.gif *.bmp *.xpm);;Tous les fichiers (*)")
	);
	if (filepath.isEmpty()) return;
	
	// that filepath needs to point to a valid, readable file
	QFileInfo filepath_info(filepath);
	if (!filepath_info.exists() || !filepath_info.isReadable()) {
		QMessageBox::critical(this, tr("Erreur"), tr("Impossible d'ouvrir le fichier sp\351cifi\351"));
		return;
	}
	
	// ensure we can use the file name to add the logo
	QString logo_name = confirmLogoName(filepath_info.fileName());
	if (logo_name.isNull()) return;
	
	open_dialog_dir_ = QDir(filepath);
	if (managed_template_ -> addLogoFromFile(filepath, logo_name)) {
		fillView();
		emitLogosChangedSignal();
	}
}

/**
	Export the currently selected logo
*/
void TitleBlockTemplateLogoManager::exportLogo() {
	QString current_logo = currentLogo();
	if (current_logo.isNull()) return;
	
	QString filepath = QFileDialog::getSaveFileName(
		this,
		tr("Choisir un fichier pour exporter ce logo"),
		open_dialog_dir_.absolutePath() + "/" + current_logo,
		tr("Tous les fichiers (*);;Images vectorielles (*.svg);;Images bitmap (*.png *.jpg *.jpeg *.gif *.bmp *.xpm)")
	);
	if (filepath.isEmpty()) return;
	
	bool save_logo = managed_template_ -> saveLogoToFile(current_logo, filepath);
	if (!save_logo) {
		QMessageBox::critical(this, tr("Erreur"), QString(tr("Impossible d'exporter vers le fichier sp\351cifi\351")));
	} else {
		open_dialog_dir_ = QDir(filepath);
	}
}

/**
	Delete the currently selected logo.
*/
void TitleBlockTemplateLogoManager::removeLogo() {
	QString current_logo = currentLogo();
	if (current_logo.isNull()) return;
	
	if (managed_template_ -> removeLogo(current_logo)) {
		fillView();
		emitLogosChangedSignal();
	}
}

/**
	Rename currently selected logo.
*/
void TitleBlockTemplateLogoManager::renameLogo() {
	QString current_logo = currentLogo();
	if (current_logo.isNull()) return;
	
	QString entered_name = logo_name_ -> text();
	QString warning_title = tr("Renommer un logo");
	if (entered_name == current_logo) {
		QMessageBox::warning(
			this,
			warning_title,
			tr("Vous devez saisir un nouveau nom.")
		);
		return;
	}
	
	if (entered_name.trimmed().isEmpty()) {
		QMessageBox::warning(
			this,
			warning_title,
			tr("Le nouveau nom ne peut pas \352tre vide.")
		);
		return;
	}
	
	if (managed_template_ -> logos().contains(entered_name)) {
		QMessageBox::warning(
			this,
			warning_title,
			tr("Le nom saisi est d\351j\340 utilis\351 par un autre logo.")
		);
		return;
	}
	
	if (managed_template_ -> renameLogo(current_logo, entered_name)) {
		fillView();
		emitLogosChangedSignal();
	}
}

/**
	@param read_only Whether this logo manager should allow logo edition
	(renaming, addition, deletion)
*/
void TitleBlockTemplateLogoManager::setReadOnly(bool read_only) {
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	
	add_button_ -> setEnabled(!read_only_);
	delete_button_ -> setEnabled(!read_only_);
	rename_button_ -> setEnabled(!read_only_);
	logo_name_ -> setReadOnly(read_only_);
}
