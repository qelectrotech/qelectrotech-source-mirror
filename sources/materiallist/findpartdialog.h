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
#ifndef FINDPARTDIALOG_H
#define FINDPARTDIALOG_H

#include "materiallist.h"

#include <QDialog>
#include <QList>
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>

class Element;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTreeWidget;
class QWidget;

/**
	@brief The FindPartDialog class
	A window to pick a part of the material list (no editing): a search
	line which narrows the list at every keystroke (PartSearch), and
	filters which only offer what still gives a result.

	Every filter combo lists the values the parts left by the search and
	the other filters have, each with its count ("Relay (14)"), so a
	filter never leads to an empty list. What is on is shown as chips
	which a click removes. "Suits this symbol" keeps the parts whose
	device type draws the symbol being placed (when none does, it is off
	and every part is offered). Parts already used in the project come
	first, starred, among the parts of an equal rank.

	The filters are not remembered: each opening starts afresh.
	The logic is in matches() and facetCounts(), which need no widget.
*/
class FindPartDialog : public QDialog
{
		Q_OBJECT

	public:
			/// The things a part can be filtered on, besides the search text
		enum Facet {
			Category,
			Manufacturer,
			DeviceTypeFacet,
			Voltage,
			Current,
			Supplier,
			FacetCount
		};

			/// What narrows the list: an empty value is "All"
		struct Filter
		{
			QString text;               ///< the words searched
			QString values[FacetCount]; ///< per facet, the value wanted
			bool suits = false;         ///< only the parts of @a suitable
			QSet<int> suitable;         ///< the rows which suit the symbol

			bool isActive(Facet facet) const {return !values[facet].isEmpty();}
				/// The number of things narrowing the list (the chips)
			int activeCount() const;
		};

		FindPartDialog(const MaterialListData &data, Element *symbol, const QString &label,
					   const QStringList &usedParts, QWidget *parent = nullptr);

		MaterialRecord selectedRecord() const;
		QString partNumber() const;
		void setSearchText(const QString &text);
		Filter filter() const {return m_filter;}
		int resultCount() const {return m_results.size();}

			/// The rows of @p data @p filter keeps, best first (PartSearch
			/// order); @p used part numbers first among equal ranks
		static QList<int> matches(const MaterialListData &data, const Filter &filter,
								  const QStringList &used = QStringList());
			/// Per facet, the value -> number of rows the search text and
			/// the filters other than that facet's keep: what its combo offers
		static QList<QMap<QString, int>> facetCounts(const MaterialListData &data, const Filter &filter);
			/// The value of @p record for @p facet (the category's key);
			/// empty when it has none
		static QString facetValue(const MaterialRecord &record, Facet facet);
			/// How a value of @p facet is shown
		static QString facetText(Facet facet, const QString &value);
			/// The rows of @p data which suit @p symbol: those whose device
			/// type has a part drawn with the symbol's element. Empty
			/// without symbol or lookup, when no device type suits it too
		static QSet<int> suitingRows(const MaterialListData &data, Element *symbol);
			/// What is on in @p filter, as the chips say it ("Category: Relay")
		static QStringList describe(const Filter &filter);

	private:
		void update();
		void readWidgets();
		void fillCombo(Facet facet, const QMap<QString, int> &counts);
		void fillChips();
		void fillList();
		void clearChip(const QString &chip);
		void resetFilters();
		void clearFacets();
		void showAllParts();
		void searchAllParts();
		void selectionChanged();
		bool eventFilter(QObject *watched, QEvent *event) override;
		static QString starred(const QString &number);

		MaterialListData m_data;
		QPointer<Element> m_symbol;
		QStringList m_used;
		bool m_any_suits = false;
		QSet<int> m_suitable;
		Filter m_filter;
		QList<int> m_results;
		bool m_updating = false;

		QLineEdit *m_search = nullptr;
		QComboBox *m_combos[FacetCount] = {};
		QCheckBox *m_suits = nullptr;
		QWidget *m_chips = nullptr;
		QLabel *m_count = nullptr;
		QStackedWidget *m_pages = nullptr;
		QTreeWidget *m_list = nullptr;
		QWidget *m_empty = nullptr;
		QLabel *m_empty_filters = nullptr;
		QDialogButtonBox *m_buttons = nullptr;
};

#endif // FINDPARTDIALOG_H
