/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "qeticons.h"
#include <QApplication>

// on redeclare ici les icones
namespace QET {
	namespace Icons {
		QIcon Add;
		QIcon AddFolio;
		QIcon Allowed;
		QIcon ApplicationExit;
		QIcon ArrowLeft;
		QIcon ArrowLeftDouble;
		QIcon ArrowRight;
		QIcon ArrowRightDouble;
		QIcon Autoconnect;
		QIcon BringForward;
		QIcon Cancel;
		QIcon Conductor;
		QIcon ConductorSettings;
		QIcon Configure;
		QIcon ConfigureToolbars;
		QIcon IC_CopyFile;
		QIcon DefaultConductor;
		QIcon DiagramAdd;
		QIcon Diagram;
		QIcon DiagramBg;
		QIcon DiagramDelete;
		QIcon DialogCancel;
		QIcon DialogInformation;
		QIcon DialogOk;
		QIcon DocumentClose;
		QIcon DocumentExport;
		QIcon DocumentSpreadsheet;
		QIcon DocumentImport;
		QIcon DocumentNew;
		QIcon DocumentOpen;
		QIcon DocumentOpenRecent;
		QIcon DocumentPrint;
		QIcon DocumentPrintFrame;
		QIcon DocumentSave;
		QIcon DocumentSaveAll;
		QIcon DocumentSaveAs;
		QIcon East;
		QIcon EditClear;
		QIcon EditClearLocationBar;
		QIcon EditCopy;
		QIcon EditCut;
		QIcon EditDelete;
		QIcon EditPaste;
		QIcon EditRedo;
		QIcon EditRename;
		QIcon EditSelectAll;
		QIcon EditTableCellMerge;
		QIcon EditTableCellSplit;
		QIcon EditTableDeleteColumn;
		QIcon EditTableDeleteRow;
		QIcon EditTableInsertColumnLeft;
		QIcon EditTableInsertColumnRight;
		QIcon EditTableInsertRowAbove;
		QIcon EditTableInsertRowUnder;
		QIcon EditText;
		QIcon EditUndo;
		QIcon Element;
		QIcon ElementDelete;
		QIcon ElementEdit;
		QIcon ElementNew;
		QIcon EndLineCircle;
		QIcon EndLineDiamond;
		QIcon EndLineNone;
		QIcon EndLineSimple;
		QIcon EndLineTriangle;
		QIcon EPS;
		QIcon Folder;
		QIcon FolderDelete;
		QIcon FolderEdit;
		QIcon FolderNew;
		QIcon Forbidden;
		QIcon FullScreenEnter;
		QIcon FullScreenExit;
		QIcon GoDown;
		QIcon GoUp;
		QIcon Ground;
		QIcon Hide;
		QIcon Home;
		QIcon HotSpot;
		QIcon InsertImage;
		QIcon Lower;
		QIcon IC_MoveFile;
		QIcon Names;
		QIcon Neutral;
		QIcon NewDiagram;
		QIcon North;
		QIcon ObjectLocked;
		QIcon ObjectRotateRight;
		QIcon ObjectUnlocked;
		QIcon Orientations;
		QIcon PartArc;
		QIcon PartCircle;
		QIcon PartEllipse;
		QIcon PartLine;
		QIcon PartPolygon;
		QIcon PartRectangle;
		QIcon PartSelect;
		QIcon PartText;
		QIcon PartTextField;
		QIcon PDF;
		QIcon Phase;
		QIcon PrintAllPages;
		QIcon Printer;
		QIcon PrintLandscape;
		QIcon PrintPortrait;
		QIcon PrintTwoPages;
		QIcon Project;
		QIcon ProjectClose;
		QIcon ProjectNew;
		QIcon Projects;
		QIcon ProjectFile;
		QIcon ProjectFileGP;
		QIcon QETIcon;
		QIcon QETLogo;
		QIcon QETOxygenLogo;
		QIcon QtLogo;
		QIcon Raise;
		QIcon Remove;
		QIcon Restore;
		QIcon SendBackward;
		QIcon Settings;
		QIcon SinglePage;
		QIcon South;
		QIcon Start;
		QIcon Terminal;
		QIcon TitleBlock;
		QIcon UserInformations;
		QIcon ViewFitWidth;
		QIcon ViewFitWindow;
		QIcon ViewMove;
		QIcon ViewRefresh;
		QIcon West;
		QIcon WindowNew;
		QIcon XmlTextFile;
		QIcon ZoomDraw;
		QIcon ZoomFitBest;
		QIcon ZoomIn;
		QIcon ZoomOriginal;
		QIcon ZoomOut;
		QIcon adding_image;
		QIcon resize_image;
		QIcon ar;
		QIcon br;
		QIcon catalonia;
		QIcon cs;
		QIcon de;
		QIcon da;
		QIcon gr;
		QIcon en;
		QIcon es;
		QIcon fr;
		QIcon hr;
		QIcon it;
		QIcon pl;
		QIcon pt;
		QIcon ro;
		QIcon ru;
		QIcon sl;
		QIcon nl;
		QIcon be;
		QIcon translation;
		QIcon listDrawings;
		QIcon AutoNum;
	}
}

/**
	Initialise les icones de l'application QElectroTech
*/
void QET::Icons::initIcons() {
	// we may need to mirror some icons for right-to-left languages
	bool rtl = QApplication::isRightToLeft();
	QTransform reverse = QTransform().scale(-1, 1);
	
	Add                 .addFile(":/ico/16x16/list-add.png");
	Add                 .addFile(":/ico/22x22/list-add.png");
	AddFolio            .addFile(":/ico/16x16/folio-new.png");
	Allowed             .addFile(":/ico/16x16/user-online.png");
	ApplicationExit     .addFile(":/ico/16x16/application-exit.png");
	ApplicationExit     .addFile(":/ico/22x22/application-exit.png");
	ArrowLeft           .addFile(":/ico/16x16/arrow-left.png");
	ArrowLeft           .addFile(":/ico/22x22/arrow-left.png");
	ArrowLeftDouble     .addFile(":/ico/16x16/arrow-left-double.png");
	ArrowLeftDouble     .addFile(":/ico/22x22/arrow-left-double.png");
	ArrowRight          .addFile(":/ico/16x16/arrow-right.png");
	ArrowRight          .addFile(":/ico/22x22/arrow-right.png");
	ArrowRightDouble    .addFile(":/ico/16x16/arrow-right-double.png");
	ArrowRightDouble    .addFile(":/ico/22x22/arrow-right-double.png");
	Autoconnect         .addFile(":/ico/22x22/autoconnect.png");
	BringForward        .addFile(":/ico/22x22/bring_forward.png");
	Cancel              .addFile(":/ico/16x16/item_cancel.png");
	Conductor           .addFile(":/ico/22x22/conductor.png");
	ConductorSettings   .addFile(":/ico/22x22/conductor2.png");
	Configure           .addFile(":/ico/16x16/configure.png");
	Configure           .addFile(":/ico/22x22/configure.png");
	ConfigureToolbars   .addFile(":/ico/16x16/configure-toolbars.png");
	ConfigureToolbars   .addFile(":/ico/22x22/configure-toolbars.png");
	IC_CopyFile            .addFile(":/ico/16x16/item_copy.png");
	DefaultConductor    .addFile(":/ico/22x22/conductor3.png");
	DiagramAdd          .addFile(":/ico/22x22/diagram_add.png");
	Diagram             .addFile(":/ico/diagram.png");
    DiagramBg           .addFile(":/ico/22x22/diagram_bg.png");
    DiagramDelete       .addFile(":/ico/22x22/diagram_del.png");
	DialogCancel        .addFile(":/ico/16x16/dialog-cancel.png");
	DialogCancel        .addFile(":/ico/22x22/dialog-cancel.png");
	DialogInformation   .addFile(":/ico/22x22/dialog-information.png");
	DialogOk            .addFile(":/ico/16x16/dialog-ok.png");
	DialogOk            .addFile(":/ico/22x22/dialog-ok.png");
	DocumentClose       .addFile(":/ico/16x16/document-close.png");
	DocumentClose       .addFile(":/ico/22x22/document-close.png");
	DocumentExport      .addFile(":/ico/16x16/document-export.png");
	DocumentExport      .addFile(":/ico/22x22/document-export.png");
	DocumentSpreadsheet .addFile(":/ico/22x22/qelectrotab-into.png");
	DocumentExport      .addFile(":/ico/128x128/document-export.png");
	DocumentImport      .addFile(":/ico/16x16/document-import.png");
	DocumentImport      .addFile(":/ico/22x22/document-import.png");
	DocumentNew         .addFile(":/ico/16x16/document-new.png");
	DocumentNew         .addFile(":/ico/22x22/document-new.png");
	DocumentOpen        .addFile(":/ico/16x16/document-open.png");
	DocumentOpen        .addFile(":/ico/22x22/document-open.png");
	DocumentOpenRecent  .addFile(":/ico/16x16/document-open-recent.png");
	DocumentOpenRecent  .addFile(":/ico/22x22/document-open-recent.png");
	DocumentPrint       .addFile(":/ico/16x16/document-print.png");
	DocumentPrint       .addFile(":/ico/22x22/document-print.png");
	DocumentPrintFrame  .addFile(":/ico/16x16/document-print-frame.png");
	DocumentPrintFrame  .addFile(":/ico/48x48/document-print-frame.png");
	DocumentSave        .addFile(":/ico/16x16/document-save.png");
	DocumentSave        .addFile(":/ico/22x22/document-save.png");
	DocumentSaveAll     .addFile(":/ico/16x16/document-save-all.png");
	DocumentSaveAll     .addFile(":/ico/22x22/document-save-all.png");
	DocumentSaveAs      .addFile(":/ico/16x16/document-save-as.png");
	DocumentSaveAs      .addFile(":/ico/22x22/document-save-as.png");
	East                .addFile(":/ico/16x16/east.png");
	EditClear           .addFile(":/ico/16x16/edit-clear.png");
	EditClear           .addFile(":/ico/22x22/edit-clear.png");
    EditText            .addFile(":/ico/22x22/names.png");
	adding_image        .addFile(":/ico/22x22/insert-image.png");
	
	if (rtl) {
		EditClearLocationBar.addPixmap(QPixmap(":/ico/16x16/edit-clear-locationbar-ltr.png").transformed(reverse));
		EditClearLocationBar.addPixmap(QPixmap(":/ico/22x22/edit-clear-locationbar-ltr.png").transformed(reverse));
	} else {
		EditClearLocationBar.addFile(":/ico/16x16/edit-clear-locationbar-ltr.png");
		EditClearLocationBar.addFile(":/ico/22x22/edit-clear-locationbar-ltr.png");
	}
	EditCopy            .addFile(":/ico/16x16/edit-copy.png");
	EditCopy            .addFile(":/ico/22x22/edit-copy.png");
	EditCut             .addFile(":/ico/16x16/edit-cut.png");
	EditCut             .addFile(":/ico/22x22/edit-cut.png");
	EditDelete          .addFile(":/ico/16x16/edit-delete.png");
	EditDelete          .addFile(":/ico/22x22/edit-delete.png");
	EditPaste           .addFile(":/ico/22x22/edit-paste.png");
	EditPaste           .addFile(":/ico/16x16/edit-paste.png");
	if (rtl) {
		EditRedo.addPixmap(QPixmap(":/ico/16x16/edit-redo.png").transformed(reverse));
		EditRedo.addPixmap(QPixmap(":/ico/22x22/edit-redo.png").transformed(reverse));
	} else {
		EditRedo            .addFile(":/ico/16x16/edit-redo.png");
		EditRedo            .addFile(":/ico/22x22/edit-redo.png");
	}
	EditRename          .addFile(":/ico/16x16/edit-rename.png");
	EditRename          .addFile(":/ico/22x22/edit-rename.png");
	EditSelectAll       .addFile(":/ico/16x16/edit-select-all.png");
	EditSelectAll       .addFile(":/ico/22x22/edit-select-all.png");
	EditTableCellMerge        .addFile(":ico/16x16/edit-table-cell-merge.png");
	EditTableCellMerge        .addFile(":ico/22x22/edit-table-cell-merge.png");
	EditTableCellSplit        .addFile(":ico/16x16/edit-table-cell-split.png");
	EditTableCellSplit        .addFile(":ico/22x22/edit-table-cell-split.png");
	EditTableDeleteColumn     .addFile(":ico/16x16/edit-table-delete-column.png");
	EditTableDeleteColumn     .addFile(":ico/22x22/edit-table-delete-column.png");
	EditTableDeleteRow        .addFile(":ico/16x16/edit-table-delete-row.png");
	EditTableDeleteRow        .addFile(":ico/22x22/edit-table-delete-row.png");
	EditTableInsertColumnLeft .addFile(":ico/16x16/edit-table-insert-column-left.png");
	EditTableInsertColumnLeft .addFile(":ico/22x22/edit-table-insert-column-left.png");
	EditTableInsertColumnRight.addFile(":ico/16x16/edit-table-insert-column-right.png");
	EditTableInsertColumnRight.addFile(":ico/22x22/edit-table-insert-column-right.png");
	EditTableInsertRowAbove   .addFile(":ico/16x16/edit-table-insert-row-above.png");
	EditTableInsertRowAbove   .addFile(":ico/22x22/edit-table-insert-row-above.png");
	EditTableInsertRowUnder   .addFile(":ico/16x16/edit-table-insert-row-under.png");
	EditTableInsertRowUnder   .addFile(":ico/22x22/edit-table-insert-row-under.png");
	if (rtl) {
		EditUndo.addPixmap(QPixmap(":/ico/16x16/edit-undo.png").transformed(reverse));
		EditUndo.addPixmap(QPixmap(":/ico/22x22/edit-undo.png").transformed(reverse));
	} else {
		EditUndo            .addFile(":/ico/16x16/edit-undo.png");
		EditUndo            .addFile(":/ico/22x22/edit-undo.png");
	}
	Element             .addFile(":/ico/oxygen-icons/16x16/mimetypes/application-x-qet-element.png");
	Element             .addFile(":/ico/oxygen-icons/22x22/mimetypes/application-x-qet-element.png");
	Element             .addFile(":/ico/oxygen-icons/32x32/mimetypes/application-x-qet-element.png");
	ElementDelete       .addFile(":/ico/22x22/element-delete.png");
	ElementEdit         .addFile(":/ico/22x22/element-edit.png");
	ElementNew          .addFile(":/ico/22x22/element-new.png");
	EndLineCircle       .addFile(":/ico/16x16/endline-circle.png");
	EndLineDiamond      .addFile(":/ico/16x16/endline-diamond.png");
	EndLineNone         .addFile(":/ico/16x16/endline-none.png");
	EndLineSimple       .addFile(":/ico/16x16/endline-simple.png");
	EndLineTriangle     .addFile(":/ico/16x16/endline-triangle.png");
	EPS                 .addFile(":/ico/32x32/image-x-eps.png");
	Folder              .addFile(":/ico/16x16/folder.png");
	FolderDelete        .addFile(":/ico/22x22/folder-delete.png");
	FolderEdit          .addFile(":/ico/22x22/folder-edit.png");
	FolderNew           .addFile(":/ico/16x16/folder-new.png");
	FolderNew           .addFile(":/ico/22x22/folder-new.png");
	Forbidden           .addFile(":/ico/16x16/user-busy.png");
	FullScreenEnter     .addFile(":/ico/16x16/view-fullscreen.png");
	FullScreenEnter     .addFile(":/ico/22x22/view-fullscreen.png");
	FullScreenExit      .addFile(":/ico/16x16/view-restore.png");
	FullScreenExit      .addFile(":/ico/22x22/view-restore.png");
	GoDown              .addFile(":/ico/16x16/go-down.png");
	GoDown              .addFile(":/ico/22x22/go-down.png");
	GoUp                .addFile(":/ico/16x16/go-up.png");
	GoUp                .addFile(":/ico/22x22/go-up.png");
	Ground              .addFile(":/ico/16x16/ground.png");
	Hide                .addFile(":/ico/16x16/masquer.png");
	Home                .addFile(":/ico/16x16/go-home.png");
	Home                .addFile(":/ico/22x22/go-home.png");
	HotSpot             .addFile(":/ico/22x22/hotspot.png");
	InsertImage         .addFile(":/ico/22x22/insert-image.png");
	Lower               .addFile(":/ico/22x22/lower.png");
	IC_MoveFile         .addFile(":/ico/16x16/item_move.png");
	Names               .addFile(":/ico/22x22/names.png");
	Neutral             .addFile(":/ico/16x16/neutral.png");
	NewDiagram          .addFile(":/ico/128x128/diagram.png");
	North               .addFile(":/ico/16x16/north.png");
	ObjectLocked        .addFile(":/ico/22x22/object-locked.png");
	ObjectRotateRight   .addFile(":/ico/22x22/object-rotate-right.png");
	ObjectUnlocked      .addFile(":/ico/22x22/object-unlocked.png");
	Orientations        .addFile(":/ico/16x16/orientations.png");
	PartArc             .addFile(":/ico/22x22/arc.png");
	PartCircle          .addFile(":/ico/16x16/circle.png");
	PartEllipse         .addFile(":/ico/22x22/ellipse.png");
	PartLine            .addFile(":/ico/22x22/line.png");
	PartPolygon         .addFile(":/ico/22x22/polygon.png");
	PartRectangle       .addFile(":/ico/22x22/rectangle.png");
	PartSelect          .addFile(":/ico/22x22/select.png");
	PartText            .addFile(":/ico/22x22/text.png");
	PartTextField       .addFile(":/ico/22x22/textfield.png");
	PDF                 .addFile(":/ico/32x32/application-pdf.png");
	Phase               .addFile(":/ico/16x16/phase.png");
	PrintAllPages       .addFile(":/ico/22x22/all_pages.png");
	Printer             .addFile(":/ico/32x32/printer.png");
	Printer             .addFile(":/ico/128x128/printer.png");
	PrintLandscape      .addFile(":/ico/22x22/landscape.png");
	PrintPortrait       .addFile(":/ico/22x22/portrait.png");
	PrintTwoPages       .addFile(":/ico/22x22/two_pages.png");
	Projects            .addFile(":/ico/128x128/project.png");
	ProjectClose        .addFile(":/ico/16x16/project-WH-close.png");
	ProjectClose        .addFile(":/ico/22x22/project-WH-close.png");
	ProjectNew          .addFile(":/ico/16x16/project-WH-new.png");
	ProjectNew          .addFile(":/ico/22x22/project-WH-new.png");
	ProjectFile         .addFile(":/ico/oxygen-icons/16x16/mimetypes/application-x-qet-project.png");
	ProjectFile         .addFile(":/ico/oxygen-icons/22x22/mimetypes/application-x-qet-project.png");
	ProjectFile         .addFile(":/ico/oxygen-icons/32x32/mimetypes/application-x-qet-project.png");
	QETIcon             .addFile(":/ico/256x256/qelectrotech.png");
	ProjectFileGP       .addFile(":/ico/16x16/project-WH.png");
	QETLogo             .addFile(":/ico/16x16/qet.png");
	QETLogo             .addFile(":/ico/256x256/qet.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/128x128/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/16x16/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/22x22/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/256x256/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/32x32/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/48x48/apps/qelectrotech.png");
	QETOxygenLogo       .addFile(":/ico/oxygen-icons/64x64/apps/qelectrotech.png");
	QtLogo              .addFile(":/ico/32x32/qt.png");
	Raise               .addFile(":/ico/22x22/raise.png");
	Remove              .addFile(":/ico/16x16/list-remove.png");
	Remove              .addFile(":/ico/22x22/list-remove.png");
	resize_image        .addFile(":/ico/22x22/transform-scale.png");
	Restore             .addFile(":/ico/22x22/restaurer.png");
	SendBackward        .addFile(":/ico/22x22/send_backward.png");
	Settings            .addFile(":/ico/128x128/settings.png");
	SinglePage          .addFile(":/ico/22x22/single_page.png");
	South               .addFile(":/ico/16x16/south.png");
	Start               .addFile(":/ico/22x22/start.png");
	Terminal            .addFile(":/ico/22x22/terminal.png");
	TitleBlock          .addFile(":/ico/16x16/label.png");
	TitleBlock          .addFile(":/ico/22x22/label.png");
	UserInformations    .addFile(":/ico/16x16/preferences-desktop-user.png");
	UserInformations    .addFile(":/ico/22x22/preferences-desktop-user.png");
	ViewFitWidth        .addFile(":/ico/22x22/view_fit_width.png");
	ViewFitWindow       .addFile(":/ico/22x22/view_fit_window.png");
	ViewMove            .addFile(":/ico/22x22/move.png");
	if (rtl) {
		ViewRefresh.addPixmap(QPixmap(":/ico/16x16/view-refresh.png").transformed(reverse));
		ViewRefresh.addPixmap(QPixmap(":/ico/22x22/view-refresh.png").transformed(reverse));
	} else {
		ViewRefresh         .addFile(":/ico/16x16/view-refresh.png");
		ViewRefresh         .addFile(":/ico/22x22/view-refresh.png");
	}
	West                .addFile(":/ico/16x16/west.png");
	WindowNew           .addFile(":/ico/16x16/window-new.png");
	WindowNew           .addFile(":/ico/22x22/window-new.png");
	XmlTextFile         .addFile(":/ico/16x16/text-xml.png");
	XmlTextFile         .addFile(":/ico/22x22/text-xml.png");
	XmlTextFile         .addFile(":/ico/32x32/text-xml.png");
	ZoomDraw            .addFile(":/ico/16x16/zoom-draw.png");
	ZoomDraw            .addFile(":/ico/22x22/zoom-draw.png");
	ZoomFitBest         .addFile(":/ico/16x16/zoom-fit-best.png");
	ZoomFitBest         .addFile(":/ico/22x22/zoom-fit-best.png");
	ZoomIn              .addFile(":/ico/16x16/zoom-in.png");
	ZoomIn              .addFile(":/ico/22x22/zoom-in.png");
	ZoomOriginal        .addFile(":/ico/16x16/zoom-original.png");
	ZoomOriginal        .addFile(":/ico/22x22/zoom-original.png");
	ZoomOut             .addFile(":/ico/16x16/zoom-out.png");
	ZoomOut             .addFile(":/ico/22x22/zoom-out.png");
	ar                  .addFile(":/ico/24x16/ar.png");
	br                  .addFile(":/ico/24x16/br.png");
	catalonia           .addFile(":/ico/24x16/catalonia.png");
	cs                  .addFile(":/ico/24x16/cs.png");
	da                  .addFile(":/ico/24x16/dk.png");
	de                  .addFile(":/ico/24x16/de.png");
	gr                  .addFile(":/ico/24x16/gr.png");
	en                  .addFile(":/ico/24x16/en.png");
	es                  .addFile(":/ico/24x16/es.png");
	fr                  .addFile(":/ico/24x16/fr.png");
	hr                  .addFile(":/ico/24x16/hr.png");
	it                  .addFile(":/ico/24x16/it.png");
	pl                  .addFile(":/ico/24x16/pl.png");
	pt                  .addFile(":/ico/24x16/pt.png");
	ro                  .addFile(":/ico/24x16/ro.png");
	ru                  .addFile(":/ico/24x16/ru.png");
	sl                  .addFile(":/ico/24x16/sl.png");
	nl                  .addFile(":/ico/24x16/nl.png");
	be                  .addFile(":/ico/24x16/be.png");
	translation         .addFile(":/ico/22x22/applications-development-translation.png");
	listDrawings        .addFile(":/ico/48x48/view-pim-journal.png");
	AutoNum             .addFile(":/ico/128x128/plasmagik.png");
}
