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
#include "qetparity.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

#if (defined(__GNUC__) || defined(__clang__)) && (defined(__x86_64__) || defined(__i386__))
#define QET_PARITY_AVX2 1
#include <immintrin.h>
#endif

namespace {

	//GF(2^8) with the polynomial 0x11d
struct Field
{
	quint8 exp[512];
	quint8 log[256];
	quint8 mul[256][256];

	Field()
	{
		int x = 1;
		for (int i = 0 ; i < 255 ; ++i) {
			exp[i] = quint8(x);
			log[x] = quint8(i);
			x <<= 1;
			if (x & 0x100) x ^= 0x11d;
		}
		for (int i = 255 ; i < 512 ; ++i) exp[i] = exp[i - 255];
		log[0] = 0;
		for (int a = 0 ; a < 256 ; ++a)
			for (int b = 0 ; b < 256 ; ++b)
				mul[a][b] = (a && b) ? exp[log[a] + log[b]] : 0;
	}

	quint8 inverse(quint8 a) const { return exp[255 - log[a]]; }
};

const Field &field()
{
	static const Field f;
	return f;
}

std::atomic<bool> portable_only{false};

	//The Cauchy coefficient of parity page @p j for the group's data page @p i
quint8 coefficient(int j, int i, int m)
{
	return field().inverse(quint8(j ^ (m + i)));
}

void mulAddPortable(quint8 *dst, const quint8 *src, quint8 c, int size)
{
	const quint8 *t = field().mul[c];
	for (int b = 0 ; b < size ; ++b) dst[b] ^= t[src[b]];
}

#ifdef QET_PARITY_AVX2
	//The product by c of each 4-bit half looked up 32 bytes at a time
__attribute__((target("avx2")))
void mulAddAvx2(quint8 *dst, const quint8 *src, quint8 c, int size)
{
	alignas(16) quint8 lo[16], hi[16];
	for (int x = 0 ; x < 16 ; ++x) {
		lo[x] = field().mul[c][x];
		hi[x] = field().mul[c][x << 4];
	}
	const __m256i tl = _mm256_broadcastsi128_si256(_mm_load_si128(reinterpret_cast<const __m128i *>(lo)));
	const __m256i th = _mm256_broadcastsi128_si256(_mm_load_si128(reinterpret_cast<const __m128i *>(hi)));
	const __m256i mask = _mm256_set1_epi8(0x0f);
	int b = 0;
	for ( ; b + 32 <= size ; b += 32) {
		const __m256i s = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + b));
		const __m256i l = _mm256_shuffle_epi8(tl, _mm256_and_si256(s, mask));
		const __m256i h = _mm256_shuffle_epi8(th, _mm256_and_si256(_mm256_srli_epi64(s, 4), mask));
		const __m256i d = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(dst + b));
		_mm256_storeu_si256(reinterpret_cast<__m256i *>(dst + b), _mm256_xor_si256(d, _mm256_xor_si256(l, h)));
	}
	mulAddPortable(dst + b, src + b, c, size - b);
}

bool hasAvx2()
{
	static const bool has = __builtin_cpu_supports("avx2");
	return has;
}
#endif

	//dst ^= c * src, @p size bytes
void mulAdd(quint8 *dst, const quint8 *src, quint8 c, int size)
{
	if (!c) return;
	if (c == 1) {
		for (int b = 0 ; b < size ; ++b) dst[b] ^= src[b];
		return;
	}
#ifdef QET_PARITY_AVX2
	if (!portable_only && hasAvx2()) {
		mulAddAvx2(dst, src, c, size);
		return;
	}
#endif
	mulAddPortable(dst, src, c, size);
}

	//Inverts the e x e matrix @p a in place; false if it has no inverse
bool invert(std::vector<quint8> &a, int e)
{
	const Field &f = field();
	std::vector<quint8> inv(size_t(e) * e, 0);
	for (int i = 0 ; i < e ; ++i) inv[size_t(i) * e + i] = 1;
	for (int c = 0 ; c < e ; ++c) {
		int p = c;
		while (p < e && !a[size_t(p) * e + c]) ++p;
		if (p == e) return false;
		for (int k = 0 ; k < e ; ++k) {
			std::swap(a[size_t(c) * e + k], a[size_t(p) * e + k]);
			std::swap(inv[size_t(c) * e + k], inv[size_t(p) * e + k]);
		}
		const quint8 s = f.inverse(a[size_t(c) * e + c]);
		for (int k = 0 ; k < e ; ++k) {
			a[size_t(c) * e + k] = f.mul[s][a[size_t(c) * e + k]];
			inv[size_t(c) * e + k] = f.mul[s][inv[size_t(c) * e + k]];
		}
		for (int r = 0 ; r < e ; ++r) {
			const quint8 factor = a[size_t(r) * e + c];
			if (r == c || !factor) continue;
			for (int k = 0 ; k < e ; ++k) {
				a[size_t(r) * e + k] ^= f.mul[factor][a[size_t(c) * e + k]];
				inv[size_t(r) * e + k] ^= f.mul[factor][inv[size_t(c) * e + k]];
			}
		}
	}
	a = inv;
	return true;
}

bool validLayout(int page_size, int k, int m)
{
	return page_size > 0 && k > 0 && m > 0 && k + m <= 255;
}

}

/**
	@brief QetParity::layout
	@return the groups for @p pages data pages at @p percent parity: groups
	of at most maxGroup pages, as even as can be, each with @p percent of its
	pages in parity and at least minParity
*/
QetParity::Layout QetParity::layout(int pages, int percent)
{
	Layout l;
	if (pages < 1) return l;
	l.k = std::min(maxGroup, pages);
	l.groups = (pages + l.k - 1) / l.k;
	l.k = (pages + l.groups - 1) / l.groups;
	l.m = std::max(minParity, int(std::ceil(l.k * percent / 100.0)));
	l.m = std::min(l.m, 255 - l.k);
	return l;
}

/**
	@brief QetParity::encode
	@param data : the pages, @p page_size bytes each; a last part page is
	read as if filled with zeros
	@return the parity pages, groups * @p m of them, group after group;
	empty if @p k and @p m are not a layout
*/
QByteArray QetParity::encode(const QByteArray &data, int page_size, int k, int m)
{
	if (!validLayout(page_size, k, m)) return {};
	const int n = int((data.size() + page_size - 1) / page_size);
	const int groups = (n + k - 1) / k;
	QByteArray padded = data;
	padded.append(QByteArray(qsizetype(n) * page_size - data.size(), '\0'));
	QByteArray parity(qsizetype(groups) * m * page_size, '\0');
	auto *par = reinterpret_cast<quint8 *>(parity.data());
	const auto *pages = reinterpret_cast<const quint8 *>(padded.constData());
	for (int i = 0 ; i < n ; ++i) {
		const int g = i % groups, li = i / groups;
		for (int j = 0 ; j < m ; ++j)
			mulAdd(par + (qsizetype(g) * m + j) * page_size, pages + qsizetype(i) * page_size,
				   coefficient(j, li, m), page_size);
	}
	return parity;
}

/**
	@brief QetParity::decode
	Rebuild the lost pages of @p data, in place, from @p parity (as
	encode() gave it, with the same @p k and @p m).
	@param lost_data : the data pages lost
	@param lost_parity : the parity pages lost, as (group, j)
	@return the number of groups with more losses than parity pages left,
	whose lost pages are left as they were; -1 if the sizes do not match
*/
int QetParity::decode(QByteArray &data, int page_size, const QByteArray &parity, int k, int m,
					  const QList<int> &lost_data, const QList<std::pair<int, int>> &lost_parity)
{
	if (!validLayout(page_size, k, m) || data.size() % page_size) return -1;
	const int n = int(data.size() / page_size);
	const int groups = (n + k - 1) / k;
	if (parity.size() != qsizetype(groups) * m * page_size) return -1;
	std::vector<char> bad_data(size_t(n), 0), bad_parity(size_t(groups) * m, 0);
	for (int i : lost_data)
		if (i >= 0 && i < n) bad_data[size_t(i)] = 1;
	for (const auto &[g, j] : lost_parity)
		if (g >= 0 && g < groups && j >= 0 && j < m) bad_parity[size_t(g) * m + j] = 1;
	auto *pages = reinterpret_cast<quint8 *>(data.data());
	const auto *par = reinterpret_cast<const quint8 *>(parity.constData());
	int failed = 0;
	std::vector<int> lost, use;
	std::vector<quint8> matrix, sums;
	for (int g = 0 ; g < groups ; ++g) {
		lost.clear();
		int size = 0;
		for (int i = g ; i < n ; i += groups, ++size)
			if (bad_data[size_t(i)]) lost.push_back(size);
		const int e = int(lost.size());
		if (!e) continue;
		use.clear();
		for (int j = 0 ; j < m && int(use.size()) < e ; ++j)
			if (!bad_parity[size_t(g) * m + j]) use.push_back(j);
		if (int(use.size()) < e) {
			++failed;
			continue;
		}
			//S_r = P_j - the sum over the pages still there of c(j, i) D_i
		matrix.assign(size_t(e) * e, 0);
		sums.assign(size_t(e) * page_size, 0);
		for (int r = 0 ; r < e ; ++r) {
			quint8 *s = sums.data() + size_t(r) * page_size;
			std::memcpy(s, par + (qsizetype(g) * m + use[size_t(r)]) * page_size, size_t(page_size));
			for (int li = 0 ; li < size ; ++li) {
				const int i = g + li * groups;
				if (!bad_data[size_t(i)])
					mulAdd(s, pages + qsizetype(i) * page_size, coefficient(use[size_t(r)], li, m), page_size);
			}
			for (int c = 0 ; c < e ; ++c)
				matrix[size_t(r) * e + c] = coefficient(use[size_t(r)], lost[size_t(c)], m);
		}
		if (!invert(matrix, e)) {
			++failed;
			continue;
		}
		for (int c = 0 ; c < e ; ++c) {
			quint8 *d = pages + qsizetype(g + lost[size_t(c)] * groups) * page_size;
			std::memset(d, 0, size_t(page_size));
			for (int r = 0 ; r < e ; ++r)
				mulAdd(d, sums.data() + size_t(r) * page_size, matrix[size_t(c) * e + r], page_size);
		}
	}
	return failed;
}

bool QetParity::usesAvx2()
{
#ifdef QET_PARITY_AVX2
	return !portable_only && hasAvx2();
#else
	return false;
#endif
}

void QetParity::setPortableOnly(bool portable)
{
	portable_only = portable;
}
