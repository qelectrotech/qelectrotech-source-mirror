// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "diagram.h"
#include "materiallist/materiallist.h"
#include "projectview.h"
#include "qetgraphicsitem/element.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "ui/customelementinfopartwidget.h"
#include "ui/elementarticleswidget.h"
#include "ui/elementinfopartwidget.h"
#include "ui/elementinfowidget.h"
#include "ui/elementpropertieswidget.h"
#include "undocommand/changeelementinformationcommand.h"

#include <QDockWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QUndoStack>

class tst_elementarticleswidget : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	Diagram *m_diagram = nullptr;
	Element *m_element = nullptr;

	static ElementInfoPartWidget *findPart(QWidget *parent, const QString &key)
	{
		for (auto *part : parent->findChildren<ElementInfoPartWidget *>()) {
			if (part->key() == key) {
				return part;
			}
		}
		return nullptr;
	}

	private slots:
		void initTestCase()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);

			const QStringList files = QDir(QET_EXAMPLES_DIR).entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name);
			for (const QString &file : files) {
				const QString copy = m_dir.filePath(file);
				QVERIFY(QFile::copy(QDir(QET_EXAMPLES_DIR).filePath(file), copy));
				QVERIFY(editor->openAndAddProject(copy, false));
				QETProject *project = editor->currentProjectView()->project();
				for (Diagram *d : project->diagrams()) {
					for (QGraphicsItem *item : d->items()) {
						if (item->type() == Element::Type) {
							auto *el = static_cast<Element *>(item);
							if (el->linkType() == Element::Simple) {
								m_diagram = d;
								m_element = el;
								break;
							}
						}
					}
					if (m_element) {
						break;
					}
				}
				if (m_element) {
					break;
				}
			}
			QVERIFY2(m_element, "no simple element found in examples");

			DiagramContext ctx;
			ctx.addValue(QETInformation::ELMT_LABEL, QStringLiteral("K1"));
			ctx.addValue(QETInformation::ELMT_MANUFACTURER, QStringLiteral("Siemens"));
			ctx.addValue(QETInformation::ELMT_DESIGNATION, QStringLiteral("815"));
			ctx.addValue(QETInformation::ELMT_DESCRIPTION, QStringLiteral("Sicherung B16"));
			ctx.addValue(QETInformation::ELMT_MANUFACTURER_AUX1, QStringLiteral("Siemens"));
			ctx.addValue(QETInformation::ELMT_DESIGNATION_AUX1, QStringLiteral("955"));
			ctx.addValue(QETInformation::ELMT_DESCRIPTION_AUX1, QStringLiteral("Kupferschiene"));
			ctx.addValue(QStringLiteral("custom_note"), QStringLiteral("KeepMe"));
			m_diagram->undoStack().push(
				new ChangeElementInformationCommand(m_element, m_element->elementInformations(), ctx));
		}

		void init()
		{
			m_diagram->undoStack().setClean();
		}

		void cleanup()
		{
			while (m_diagram->undoStack().index() > m_diagram->undoStack().cleanIndex()) {
				m_diagram->undoStack().undo();
			}
		}

		void articleKeysMovedOutOfInfoTab()
		{
			ElementPropertiesWidget props(m_element);
			auto *info_widget = props.findChild<ElementInfoWidget *>();
			auto *articles_widget = props.findChild<ElementArticlesWidget *>();
			QVERIFY(info_widget);
			QVERIFY(articles_widget);

			const QStringList article_keys = ElementArticlesWidget::managedKeys();
			QStringList info_keys;
			for (auto *part : info_widget->findChildren<ElementInfoPartWidget *>()) {
				info_keys << part->key();
			}
			for (const QString &key : article_keys) {
				QVERIFY2(!info_keys.contains(key),
						 qPrintable(QStringLiteral("Key should not remain in ElementInfoWidget: %1").arg(key)));
			}

			// Article keys must not leak as custom properties in Information tab
			QStringList custom_keys;
			for (auto *custom : info_widget->findChildren<CustomElementInfoPartWidget *>()) {
				custom_keys << custom->key();
			}
			QVERIFY(custom_keys.contains(QStringLiteral("custom_note")));
			for (const QString &key : article_keys) {
				QVERIFY(!custom_keys.contains(key));
			}

			// Element-context fields must stay in the Information tab and not appear in Articles
			const QStringList context_keys = {
				QETInformation::ELMT_FUNCTION,
				QETInformation::ELMT_COMMENT,
				QETInformation::ELMT_NOTES,
				QETInformation::ELMT_PLANT,
				QETInformation::ELMT_LOCATION
			};
			for (const QString &key : context_keys) {
				QVERIFY2(info_keys.contains(key),
						 qPrintable(QStringLiteral("Context key missing in ElementInfoWidget: %1").arg(key)));
				QVERIFY2(findPart(articles_widget, key) == nullptr,
						 qPrintable(QStringLiteral("Context key should not appear in Articles tab: %1").arg(key)));
			}

			// All purely article-related block 0 columns exist on Articles tab
			for (const QString &col : MaterialList::columnsForBlock(0)) {
				if (context_keys.contains(col)) {
					continue;
				}
				const QString key = MaterialList::elementInfoKey(col, 0);
				QVERIFY2(findPart(articles_widget, key) != nullptr,
						 qPrintable(QStringLiteral("Missing block 0 field: %1").arg(key)));
			}

			// Article number (designation) must come before textual description in each block
			const QList<ElementInfoPartWidget *> ordered_parts =
				articles_widget->findChildren<ElementInfoPartWidget *>();
			auto indexOfKey = [&ordered_parts](const QString &key) {
				for (int i = 0; i < ordered_parts.size(); ++i) {
					if (ordered_parts.at(i)->key() == key) {
						return i;
					}
				}
				return -1;
			};
			QVERIFY(indexOfKey(QETInformation::ELMT_DESIGNATION) >= 0);
			QVERIFY(indexOfKey(QETInformation::ELMT_DESIGNATION) < indexOfKey(QETInformation::ELMT_DESCRIPTION));
			QVERIFY(indexOfKey(QETInformation::ELMT_DESIGNATION_AUX1) < indexOfKey(QETInformation::ELMT_DESCRIPTION_AUX1));
			// Auxiliary blocks 1..4 exist as well
			for (int block = 1; block < ElementArticlesWidget::kBlockCount; ++block) {
				for (const QString &col : MaterialList::columnsForBlock(block)) {
					const QString key = MaterialList::elementInfoKey(col, block);
					QVERIFY2(findPart(articles_widget, key) != nullptr,
							 qPrintable(QStringLiteral("Missing block %1 field: %2").arg(block).arg(key)));
				}
			}
		}

		void selectingAddingAndRemovingArticles()
		{
			ElementArticlesWidget widget(m_element);
			auto *list = widget.findChild<QTreeWidget *>(QStringLiteral("m_article_list"));
			auto *stack = widget.findChild<QStackedWidget *>(QStringLiteral("m_article_stack"));
			auto *add_btn = widget.findChild<QPushButton *>(QStringLiteral("m_add_article_btn"));
			auto *remove_btn = widget.findChild<QPushButton *>(QStringLiteral("m_remove_article_btn"));
			QVERIFY(list && stack && add_btn && remove_btn);

			// Initially Article 1 + Article 2 (since aux1 has data)
			QCOMPARE(list->topLevelItemCount(), 2);
			QCOMPARE(list->topLevelItem(0)->data(0, Qt::UserRole + 1).toString(), QStringLiteral("815 / Sicherung B16"));
			QCOMPARE(list->topLevelItem(1)->data(0, Qt::UserRole + 1).toString(), QStringLiteral("955 / Kupferschiene"));
			QCOMPARE(stack->currentIndex(), 0);
			QVERIFY(add_btn->isEnabled());

			// Editing quantity updates column 1 and vice versa
			findPart(&widget, QETInformation::ELMT_QUANTITY)->setText(QStringLiteral("3"));
			QCOMPARE(list->topLevelItem(0)->text(1), QStringLiteral("3"));
			list->topLevelItem(0)->setText(1, QStringLiteral("5"));
			QCOMPARE(findPart(&widget, QETInformation::ELMT_QUANTITY)->text(), QStringLiteral("5"));

			// Switching left row updates right page
			list->setCurrentItem(list->topLevelItem(1));
			QCOMPARE(stack->currentIndex(), 1);
			QCOMPARE(findPart(&widget, QETInformation::ELMT_DESIGNATION_AUX1)->text(), QStringLiteral("955"));

			// Add slots 3, 4, 5
			add_btn->click();
			QCOMPARE(list->topLevelItemCount(), 3);
			QCOMPARE(stack->currentIndex(), 2);
			add_btn->click();
			QCOMPARE(list->topLevelItemCount(), 4);
			add_btn->click();
			QCOMPARE(list->topLevelItemCount(), 5);
			QVERIFY(!add_btn->isEnabled());

			// Remove newly added slot 5
			remove_btn->click();
			QCOMPARE(list->topLevelItemCount(), 4);
			QVERIFY(add_btn->isEnabled());

			// Reset clears transient empty additions
			widget.reset();
			QCOMPARE(list->topLevelItemCount(), 2);
		}

		void applyTogetherWithoutClobbering()
		{
			ElementPropertiesWidget props(m_element);
			auto *info_widget = props.findChild<ElementInfoWidget *>();
			auto *articles_widget = props.findChild<ElementArticlesWidget *>();
			QVERIFY(info_widget && articles_widget);

			findPart(info_widget, QETInformation::ELMT_LABEL)->setText(QStringLiteral("K99"));
			findPart(articles_widget, QETInformation::ELMT_MANUFACTURER)->setText(QStringLiteral("ABB"));
			findPart(articles_widget, QETInformation::ELMT_WIDTH)->setText(QStringLiteral("17.5"));
			findPart(articles_widget, QETInformation::ELMT_DESIGNATION_AUX1)->setText(QStringLiteral("AUX-100"));

			props.apply();

			const DiagramContext saved = m_element->elementInformations();
			QCOMPARE(saved.value(QETInformation::ELMT_LABEL).toString(), QStringLiteral("K99"));
			QCOMPARE(saved.value(QETInformation::ELMT_MANUFACTURER).toString(), QStringLiteral("ABB"));
			QCOMPARE(saved.value(QETInformation::ELMT_WIDTH).toString(), QStringLiteral("17.5"));
			QCOMPARE(saved.value(QETInformation::ELMT_DESIGNATION_AUX1).toString(), QStringLiteral("AUX-100"));
			QCOMPARE(saved.value(QStringLiteral("custom_note")).toString(), QStringLiteral("KeepMe"));

			m_diagram->undoStack().undo();
			const DiagramContext reverted = m_element->elementInformations();
			QCOMPARE(reverted.value(QETInformation::ELMT_LABEL).toString(), QStringLiteral("K1"));
			QCOMPARE(reverted.value(QETInformation::ELMT_MANUFACTURER).toString(), QStringLiteral("Siemens"));
			QCOMPARE(reverted.value(QETInformation::ELMT_DESIGNATION_AUX1).toString(), QStringLiteral("955"));
		}

		void liveEditAndReset()
		{
			ElementArticlesWidget articles(m_element);
			auto *supplier = findPart(&articles, QETInformation::ELMT_SUPPLIER);
			QVERIFY(supplier);

			supplier->setText(QStringLiteral("TempSupplier"));
			QCOMPARE(articles.currentInfo().value(QETInformation::ELMT_SUPPLIER).toString(), QStringLiteral("TempSupplier"));
			articles.reset();
			QCOMPARE(articles.currentInfo().value(QETInformation::ELMT_SUPPLIER).toString(), QString());

			articles.setLiveEdit(true);
			findPart(&articles, QETInformation::ELMT_SUPPLIER)->setText(QStringLiteral("Conrad"));
			QCOMPARE(m_element->elementInformations().value(QETInformation::ELMT_SUPPLIER).toString(), QStringLiteral("Conrad"));
			QCOMPARE(m_element->elementInformations().value(QETInformation::ELMT_LABEL).toString(), QStringLiteral("K1"));
		}

		void dockHostingDoesNotForceWideMinimumWidth()
		{
			QDockWidget dock;
			auto *props = new ElementPropertiesWidget(m_element, &dock);
			dock.setWidget(props);
			auto *articles = props->findChild<ElementArticlesWidget *>();
			QVERIFY(articles);
			QVERIFY2(articles->minimumSizeHint().width() <= 200,
					 qPrintable(QStringLiteral("Articles dock minimum width too large: %1")
									.arg(articles->minimumSizeHint().width())));
			QVERIFY2(dock.minimumSizeHint().width() <= 360,
					 qPrintable(QStringLiteral("Dock minimum width grew too large: %1")
									.arg(dock.minimumSizeHint().width())));
		}

		void materialPickerKeepsInformationTabFields()
		{
			ElementPropertiesWidget props(m_element);
			auto *info_widget = props.findChild<ElementInfoWidget *>();
			auto *articles = props.findChild<ElementArticlesWidget *>();
			QVERIFY(info_widget && articles);

			MaterialRecord record;
			record.values.insert(QETInformation::ELMT_DESIGNATION, QStringLiteral("3RV2011-1AA10"));
			record.values.insert(QETInformation::ELMT_FUNCTION, QStringLiteral("Motor starter"));
			record.values.insert(QETInformation::ELMT_PLANT, QStringLiteral("P1"));
			record.values.insert(QETInformation::ELMT_LOCATION, QStringLiteral("Cab2"));
			record.values.insert(QETInformation::ELMT_COMMENT, QStringLiteral("Main breaker"));
			record.values.insert(QETInformation::ELMT_NOTES, QStringLiteral("Check torque"));

			articles->applyMaterialRecord(record, 0);

			const DiagramContext preview = articles->currentInfo();
			QCOMPARE(preview.value(QETInformation::ELMT_DESIGNATION).toString(), QStringLiteral("3RV2011-1AA10"));
			QCOMPARE(preview.value(QETInformation::ELMT_FUNCTION).toString(), QStringLiteral("Motor starter"));
			QCOMPARE(preview.value(QETInformation::ELMT_PLANT).toString(), QStringLiteral("P1"));
			QCOMPARE(preview.value(QETInformation::ELMT_LOCATION).toString(), QStringLiteral("Cab2"));
			QCOMPARE(preview.value(QETInformation::ELMT_COMMENT).toString(), QStringLiteral("Main breaker"));
			QCOMPARE(preview.value(QETInformation::ELMT_NOTES).toString(), QStringLiteral("Check torque"));

			props.apply();

			const DiagramContext saved = m_element->elementInformations();
			QCOMPARE(saved.value(QETInformation::ELMT_DESIGNATION).toString(), QStringLiteral("3RV2011-1AA10"));
			QCOMPARE(saved.value(QETInformation::ELMT_FUNCTION).toString(), QStringLiteral("Motor starter"));
			QCOMPARE(saved.value(QETInformation::ELMT_PLANT).toString(), QStringLiteral("P1"));
			QCOMPARE(saved.value(QETInformation::ELMT_LOCATION).toString(), QStringLiteral("Cab2"));
			QCOMPARE(saved.value(QETInformation::ELMT_COMMENT).toString(), QStringLiteral("Main breaker"));
			QCOMPARE(saved.value(QETInformation::ELMT_NOTES).toString(), QStringLiteral("Check torque"));
			QCOMPARE(findPart(info_widget, QETInformation::ELMT_FUNCTION)->text(), QStringLiteral("Motor starter"));
			QCOMPARE(findPart(info_widget, QETInformation::ELMT_PLANT)->text(), QStringLiteral("P1"));

			// Live-edit pick also pushes and refreshes Information tab immediately.
			props.setLiveEdit(true);
			MaterialRecord live_record;
			live_record.values.insert(QETInformation::ELMT_DESIGNATION, QStringLiteral("5SY4116-7"));
			live_record.values.insert(QETInformation::ELMT_FUNCTION, QStringLiteral("Circuit breaker"));
			live_record.values.insert(QETInformation::ELMT_PLANT, QStringLiteral("P2"));
			articles->applyMaterialRecord(live_record, 0);

			QCOMPARE(m_element->elementInformations().value(QETInformation::ELMT_DESIGNATION).toString(), QStringLiteral("5SY4116-7"));
			QCOMPARE(m_element->elementInformations().value(QETInformation::ELMT_FUNCTION).toString(), QStringLiteral("Circuit breaker"));
			QCOMPARE(findPart(info_widget, QETInformation::ELMT_FUNCTION)->text(), QStringLiteral("Circuit breaker"));
			QCOMPARE(findPart(info_widget, QETInformation::ELMT_PLANT)->text(), QStringLiteral("P2"));
		}
};

QET_APP_TEST_MAIN(tst_elementarticleswidget)

#include "tst_elementarticleswidget.moc"
