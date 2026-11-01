// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef QETPARITY_H
#define QETPARITY_H

#include <QByteArray>
#include <QList>

#include <utility>

/**
	Repair data for the pages of a file, as QR codes and RAID have
	(QET-SQL-FORMAT-SCOPE.md 5.8, QETDB-S4C-PLAN.md): a Reed-Solomon erasure
	code over GF(2^8), systematic Cauchy, the same as the prototype
	tools/qet-sqlite/rescue/qetparity.c, byte for byte.

	The data is n pages. Page i belongs to group i % groups, so that
	neighbouring pages fall in different groups and a run of bad pages costs
	each group one page or so. Each group of at most k pages gets m parity
	pages: any m pages of a group, data or parity, can be lost and rebuilt
	exactly, provided the caller knows which ones are lost.
*/
namespace QetParity
{
	struct Layout
	{
		int k = 0;      ///< the largest group's page count
		int m = 0;      ///< parity pages per group
		int groups = 0;
	};

		/// The largest group of data pages
	constexpr int maxGroup = 200;
		/// Parity pages per group, at least
	constexpr int minParity = 8;

	Layout layout(int pages, int percent);

	QByteArray encode(const QByteArray &data, int page_size, int k, int m);

	int decode(QByteArray &data, int page_size, const QByteArray &parity, int k, int m,
			   const QList<int> &lost_data, const QList<std::pair<int, int>> &lost_parity);

		/// For the tests: whether encode() and decode() use AVX2 here
	bool usesAvx2();
		/// For the tests: use the portable path even where AVX2 is there
	void setPortableOnly(bool portable);
}

#endif // QETPARITY_H
