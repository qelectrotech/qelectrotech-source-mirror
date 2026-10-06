// SPDX-License-Identifier: GPL-2.0-or-later
// Linked by run_image_dark_probe.py against the application's own objects.
#include <QApplication>
#include <QBuffer>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFile>
#include <QFontDatabase>
#include <QPrinter>
#include <QPushButton>
#include <QSettings>
#include <QSvgGenerator>
#include <QTemporaryDir>
#include <cstdio>
#include <stdexcept>
#include "../../sources/diagram.h"
#include "../../sources/qetproject.h"
#include "../../sources/qetresult.h"
#include "../../sources/qetmessagebox.h"
#include "../../sources/qetpalette.h"
#include "../../sources/palettegraphicsview.h"
#include "../../sources/qetgraphicsitem/diagramimageitem.h"
#include "../../sources/ui/imagepropertieswidget.h"
#include "../../sources/PropertiesEditor/propertieseditordialog.h"

static void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
static QByteArray png(const QPixmap &pixmap) {
	QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly); pixmap.save(&buffer,"PNG"); return bytes;
}
static QByteArray read(const QString &path) { QFile file(path); check(file.open(QIODevice::ReadOnly),"read export"); return file.readAll(); }
static DiagramImageItem *findImage(Diagram *diagram) {
	for (auto *item : diagram->items()) if (auto *image = dynamic_cast<DiagramImageItem *>(item)) return image;
	return nullptr;
}
int main(int argc, char **argv)
{
	QApplication app(argc,argv);
	std::freopen(argv[2],"w",stdout); std::freopen(argv[2],"a",stderr);
	QTemporaryDir settings;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
	QCoreApplication::setOrganizationName("QETImageDarkRegression");
	QETProject::setBackupEnabled(false); QET::QetMessageBox::setNonInteractive(true);
	QFontDatabase::addApplicationFont(":/fonts/LiberationSans-Regular.ttf");
	try {
		const QString output=QString::fromLocal8Bit(argv[3]);
		QETProject project(QString::fromLocal8Bit(argv[1])); check(project.state()==QETProject::Ok,"load fixture");
		auto *diagram=project.diagrams().first();
		QImage original(40,30,QImage::Format_ARGB32); original.fill(QColor(197,81,33));
		for(int y=0;y<30;++y) for(int x=10;x<20;++x) original.setPixelColor(x,y,QColor(197,81,33,128));
		for(int y=10;y<16;++y) for(int x=24;x<30;++x) original.setPixelColor(x,y,Qt::transparent);
		QPixmap source=QPixmap::fromImage(original);
		auto *image=new DiagramImageItem(source); diagram->addItem(image);
		image->setPos(55,55); image->setLabel("Image test"); image->setRotationAngle(17); image->setScaleFactorX(1.3); image->setSkewX(8);
		check(!image->adaptToDarkTheme(),"new image defaults to original colours");
		QDomDocument xml; auto legacy=image->toXml(xml);
		check(!legacy.hasAttribute("adapt_to_dark_theme"),"default does not add an attribute to old files");
		const QRect crop(4,3,24,20);
		legacy.firstChild().setNodeValue(QString::fromLatin1(png(source.copy(crop)).toBase64()));
		auto base=xml.createElement("image_base"); base.appendChild(xml.createTextNode(QString::fromLatin1(png(source).toBase64()))); legacy.appendChild(base);
		auto cropElement=xml.createElement("crop"); cropElement.setAttribute("x",4); cropElement.setAttribute("y",3); cropElement.setAttribute("w",24); cropElement.setAttribute("h",20); legacy.appendChild(cropElement);
		image->setAdaptToDarkTheme(true); check(image->fromXml(legacy),"load cropped legacy image");
		check(!image->adaptToDarkTheme(),"missing attribute resets an existing image to original colours");
		check(image->pixmap().size()==crop.size(),"crop survives");
		const QByteArray pristine=png(image->pixmap()); const auto transform=image->transform(); const auto bounds=image->boundingRect();
		ImagePropertiesWidget widget(image); auto *box=widget.findChild<QCheckBox *>("m_adapt_to_dark_theme_cb");
		check(box && !box->isChecked(),"unchecked properties default");
		check(box->text()==QString::fromUtf8("Adapter l’image au thème sombre"),"checkbox wording");
		check(box->toolTip()==QString::fromUtf8("Adapte les couleurs à l’affichage sombre uniquement. L’image originale, les exports et les impressions restent inchangés."),"tooltip wording");
		box->setChecked(true); check(image->adaptToDarkTheme(),"preview"); widget.reset(); check(!image->adaptToDarkTheme(),"reset cancels preview");
		diagram->undoStack().clear(); box->setChecked(true); widget.apply(); check(image->adaptToDarkTheme(),"apply");
		diagram->undoStack().undo(); check(!image->adaptToDarkTheme()&&!box->isChecked(),"undo updates checkbox");
		diagram->undoStack().redo(); check(image->adaptToDarkTheme()&&box->isChecked(),"redo updates checkbox");
		widget.setLiveEdit(true); box->click(); check(!image->adaptToDarkTheme(),"live edit");
		check(diagram->undoStack().count()==2,"deliberate toggles do not merge");
		diagram->undoStack().undo(); check(image->adaptToDarkTheme(),"live undo");
		diagram->undoStack().redo(); check(!image->adaptToDarkTheme(),"live redo");
		box->click(); check(image->adaptToDarkTheme()&&diagram->undoStack().count()==3,"edit after undo has current baseline");
		for (auto role : {QDialogButtonBox::Cancel,QDialogButtonBox::Reset,QDialogButtonBox::Apply}) {
			image->setAdaptToDarkTheme(false);
			auto *editor=new ImagePropertiesWidget(image); PropertiesEditorDialog dialog(editor);
			editor->findChild<QCheckBox *>("m_adapt_to_dark_theme_cb")->setChecked(true);
			dialog.findChild<QDialogButtonBox *>()->button(role)->click();
			check(image->adaptToDarkTheme()==(role==QDialogButtonBox::Apply),"actual dialog button semantics");
		}
		check(png(image->pixmap())==pristine && image->transform()==transform && image->boundingRect()==bounds && image->label()=="Image test","choice cannot modify pixels, transform, crop or caption");
		image->setAdaptToDarkTheme(false);
		{
			auto *editor=new ImagePropertiesWidget(image); PropertiesEditorDialog dialog(editor);
			editor->findChild<QCheckBox *>("m_adapt_to_dark_theme_cb")->setChecked(true);
			dialog.reject(); check(!image->adaptToDarkTheme(),"Escape/title-bar rejection cancels preview");
		}
		PaletteGraphicsView view(diagram); view.setSceneRect(0,0,180,180); view.setFrameShape(QFrame::NoFrame);
		view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		view.setAlignment(Qt::AlignLeft|Qt::AlignTop); view.resize(180,180); view.setAttribute(Qt::WA_DontShowOnScreen); view.show();
		for (bool dark : {false,true}) for(bool adapt : {false,true}) {
			const auto palette=dark?QET::Palette::fusionDark():QET::Palette::fusionLight(); QApplication::setPalette(palette);
			image->setAdaptToDarkTheme(adapt); QApplication::processEvents(); const auto shown=view.viewport()->grab().toImage();
			shown.save(output+QString("/screen-%1-%2.png").arg(dark).arg(adapt));
			const auto point=view.mapFromScene(image->mapToScene(QPointF(2,2))); const auto ratio=shown.devicePixelRatio();
			QImage expected(1,1,QImage::Format_RGB32); expected.fill(QColor(197,81,33));
			if(dark&&adapt) QET::Palette::invertLightness(expected,palette.color(QPalette::Base),palette.color(QPalette::Text));
			check(shown.pixelColor(qRound(point.x()*ratio),qRound(point.y()*ratio))==expected.pixelColor(0,0),"actual rotated/cropped image display colour");
			project.setFilePath(output+QString("/project-%1-%2.qet").arg(dark).arg(adapt)); check(project.write().isOk(),"project save");
			QETProject reopened(project.filePath()); check(reopened.state()==QETProject::Ok,"project reopen");
			auto *loaded=findImage(reopened.diagrams().first()); check(loaded && loaded->adaptToDarkTheme()==adapt,"per-image persisted choice");
			check(png(loaded->pixmap())==pristine && loaded->transform()==transform && loaded->label()=="Image test","reopen keeps original image/crop/transform/caption");
		}
		QApplication::setPalette(QET::Palette::fusionDark()); QImage exports[2];
		for(int i=0;i<2;++i) {
			image->setAdaptToDarkTheme(i); exports[i]=QImage(180,180,QImage::Format_RGB32); exports[i].fill(Qt::white);
			{QPainter painter(&exports[i]); diagram->render(&painter,QRectF(0,0,180,180),QRectF(0,0,180,180));}
			exports[i].save(output+QString("/export-%1.png").arg(i));
			QSvgGenerator svg; svg.setFileName(output+QString("/export-%1.svg").arg(i)); svg.setSize(QSize(180,180)); svg.setViewBox(QRect(0,0,180,180));
			{QPainter painter(&svg); diagram->render(&painter,QRectF(0,0,180,180),QRectF(0,0,180,180));}
			QPrinter printer; printer.setOutputFormat(QPrinter::PdfFormat); printer.setOutputFileName(output+QString("/print-%1.pdf").arg(i)); printer.setResolution(72);
			{QPainter painter(&printer); check(painter.isActive(),"PDF print painter"); diagram->render(&painter,QRectF(0,0,180,180),QRectF(0,0,180,180));}
		}
		check(exports[0]==exports[1],"raster export unchanged"); check(read(output+"/export-0.svg")==read(output+"/export-1.svg"),"SVG export unchanged");
		std::puts("PASS: image defaults, XML crop/alpha/transform/label, properties preview/apply/reset/cancel, undo/redo and live edit, four display modes, project save/reopen, raster/SVG export, PDF print generation");
	} catch(const std::exception &error) {std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
	return 0;
}
