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
#include "dwgreader.h"

#include <QRegularExpression>
#include <QtEndian>

#include <cmath>
#include <cstring>

namespace {

// DWG object types with a fixed number (the rest are classes >= 500)
enum Type {
	T_TEXT = 1, T_ATTRIB = 2, T_ATTDEF = 3, T_BLOCK = 4, T_ENDBLK = 5, T_SEQEND = 6, T_INSERT = 7,
	T_MINSERT = 8, T_VERTEX_2D = 10, T_VERTEX_3D = 11, T_POLYLINE_2D = 15, T_POLYLINE_3D = 16,
	T_ARC = 17, T_CIRCLE = 18, T_LINE = 19, T_POINT = 27, T_SOLID = 31, T_VIEWPORT = 34,
	T_ELLIPSE = 35, T_SPLINE = 36, T_MTEXT = 44, T_BLOCK_HEADER = 49, T_LWPOLYLINE = 77, T_HATCH = 78
};

bool isEntity(int t)
{
	switch (t) {
		case T_TEXT: case T_ATTRIB: case T_ATTDEF: case T_BLOCK: case T_ENDBLK: case T_SEQEND:
		case T_INSERT: case T_MINSERT: case T_VERTEX_2D: case T_VERTEX_3D: case T_POLYLINE_2D:
		case T_POLYLINE_3D: case T_ARC: case T_CIRCLE: case T_LINE: case T_POINT: case T_SOLID:
		case T_VIEWPORT: case T_ELLIPSE: case T_SPLINE: case T_MTEXT: case T_LWPOLYLINE: case T_HATCH:
			return true;
		default:
			return false;
	}
}

quint32 le32(const QByteArray &d, qint64 at)
{
	if (at < 0 || at + 4 > d.size()) return 0;
	return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(d.constData() + at));
}

quint64 le64(const QByteArray &d, qint64 at)
{
	if (at < 0 || at + 8 > d.size()) return 0;
	return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(d.constData() + at));
}

/**
	The R2004 compression: an LZ77 variant with opcodes 0x10 to 0xFF,
	literal runs between them and 0x11 as end mark.
*/
QByteArray decompress(const QByteArray &src, int size)
{
	QByteArray dst;
	dst.reserve(size);
	int i = 0;
	const int n = src.size();
	auto byte = [&]() -> int { return i < n ? uchar(src.at(i++)) : -1; };
	auto literalLength = [&](int &opcode) -> int {
		int b = byte();
		opcode = 0;
		if (b >= 0x01 && b <= 0x0F) return b + 3;
		if (b == 0) {
			int total = 0x0F;
			while ((b = byte()) == 0) total += 0xFF;
			return b < 0 ? 0 : total + b + 3;
		}
		opcode = b < 0 ? 0 : b;
		return 0;
	};
	auto longLength = [&]() -> int {
		int b = byte(), total = 0;
		if (b == 0) {
			total = 0xFF;
			while ((b = byte()) == 0) total += 0xFF;
		}
		return b < 0 ? 0 : total + b;
	};
	auto twoByteOffset = [&](int &lit) -> int {
		const int a = byte(), b = byte();
		if (a < 0 || b < 0) return 0;
		lit = a & 0x03;
		return (a >> 2) | (b << 6);
	};
	auto copyLiteral = [&](int count) {
		count = qMax(0, qMin(count, n - i));
		dst.append(src.constData() + i, count);
		i += count;
	};

	int op = 0;
	copyLiteral(literalLength(op));
	while (i < n && dst.size() < size) {
		if (op == 0) op = byte();
		int count = 0, offset = 0, lit = 0;
		if (op >= 0x40) {
			count = ((op & 0xF0) >> 4) - 1;
			const int op2 = byte();
			offset = (op2 << 2) | ((op & 0x0C) >> 2);
			if (op & 0x03) { lit = op & 0x03; op = 0; }
			else lit = literalLength(op);
		} else if (op >= 0x10 && op <= 0x3F && op != 0x11) {
			if (op >= 0x21) {
				count = op - 0x1E;
				offset = twoByteOffset(lit);
			} else if (op == 0x20) {
				count = longLength() + 0x21;
				offset = twoByteOffset(lit);
			} else if (op == 0x10) {
				count = longLength() + 9;
				offset = twoByteOffset(lit) + 0x3FFF;
			} else {
				count = (op & 0x0F) + 2;
				offset = twoByteOffset(lit) + 0x3FFF;
			}
			if (lit) op = 0;
			else lit = literalLength(op);
		} else {
			break;                                   // 0x11: end, anything else: damaged
		}
		const int from = dst.size() - offset - 1;
		if (from < 0) break;
		for (int k = 0; k < count; ++k) dst.append(dst.at(from + k));
		copyLiteral(lit);
	}
	dst.truncate(size);
	return dst;
}

/**
	Bitwise reading of the DWG data types (B, BB, BS, BL, BD, DD, RD, RC,
	MC, MS, H, ...). Reading past the end sets bad() instead of throwing.
*/
class Bits
{
	public:
		Bits(const QByteArray &d, qint64 bit = 0) : m_d(d), m_p(bit) {}
		qint64 pos() const { return m_p; }
		void seek(qint64 bit) { m_p = bit; }
		bool bad() const { return m_bad; }

		int B()
		{
			if (m_p < 0 || (m_p >> 3) >= m_d.size()) { m_bad = true; return 0; }
			const int b = (uchar(m_d.at(int(m_p >> 3))) >> (7 - (m_p & 7))) & 1;
			++m_p;
			return b;
		}
		quint32 bits(int n)
		{
			quint32 w = 0;
			for (int k = 0; k < n; ++k) w = (w << 1) | quint32(B());
			return w;
		}
		quint8 RC() { return quint8(bits(8)); }
		QByteArray raw(qint64 n)
		{
			if (n < 0 || (m_p >> 3) + n > m_d.size()) { m_bad = true; return {}; }
			QByteArray out;
			if ((m_p & 7) == 0) {
				out = m_d.mid(int(m_p >> 3), int(n));
				m_p += 8 * n;
			} else {
				out.reserve(int(n));
				for (qint64 k = 0; k < n; ++k) out.append(char(RC()));
			}
			return out;
		}
		quint16 RSu()
		{
			const QByteArray b = raw(2);
			return b.size() == 2 ? qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(b.constData())) : 0;
		}
		qint16 RS() { return qint16(RSu()); }
		qint32 RL()
		{
			const QByteArray b = raw(4);
			return b.size() == 4 ? qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(b.constData())) : 0;
		}
		double RD()
		{
			const QByteArray b = raw(8);
			if (b.size() != 8) return 0;
			double v;
			std::memcpy(&v, b.constData(), 8);       // IEEE little endian, as on all supported hosts
			return std::isfinite(v) ? v : 0;
		}
		int BB() { return int(bits(2)); }
		int BS()
		{
			switch (BB()) {
				case 0: return RS();
				case 1: return RC();
				case 2: return 0;
				default: return 256;
			}
		}
		qint64 BL()
		{
			switch (BB()) {
				case 0: return RL();
				case 1: return RC();
				default: return 0;
			}
		}
		quint64 BLL()
		{
			const int n = int(bits(3));
			quint64 w = 0;
			for (int k = 0; k < n; ++k) w |= quint64(RC()) << (8 * k);
			return w;
		}
		double BD()
		{
			switch (BB()) {
				case 0: return RD();
				case 1: return 1.0;
				default: return 0.0;
			}
		}
		double DD(double def)
		{
			const int c = BB();
			if (c == 0) return def;
			if (c == 3) return RD();
			uchar b[8];
			std::memcpy(b, &def, 8);
			if (c == 1) {
				const QByteArray r = raw(4);
				if (r.size() == 4) std::memcpy(b, r.constData(), 4);
			} else {
				const QByteArray r1 = raw(2), r2 = raw(4);
				if (r1.size() == 2) std::memcpy(b + 4, r1.constData(), 2);
				if (r2.size() == 4) std::memcpy(b, r2.constData(), 4);
			}
			double v;
			std::memcpy(&v, b, 8);
			return std::isfinite(v) ? v : def;
		}
		double BT() { return B() ? 0.0 : BD(); }
		void BE() { if (!B()) { BD(); BD(); BD(); } }
		QPointF P2RD() { const double x = RD(); return {x, RD()}; }
		QPointF P3BD() { const double x = BD(), y = BD(); BD(); return {x, y}; }
		qint64 MC(bool sign = true)
		{
			qint64 w = 0;
			int s = 0;
			for (int k = 0; k < 10 && !m_bad; ++k) {
				const int b = RC();
				if (b & 0x80) { w |= qint64(b & 0x7F) << s; s += 7; continue; }
				if (sign) {
					w |= qint64(b & 0x3F) << s;
					return (b & 0x40) ? -w : w;
				}
				return w | (qint64(b & 0x7F) << s);
			}
			return w;
		}
		qint64 MS()
		{
			qint64 w = 0;
			int s = 0;
			for (int k = 0; k < 4 && !m_bad; ++k) {
				const int lo = RC(), hi = RC();
				const int v = lo | (hi << 8);
				if (v & 0x8000) { w |= qint64(v & 0x7FFF) << s; s += 15; continue; }
				return w | (qint64(v) << s);
			}
			return w;
		}
		/// handle reference; relative codes 6, 8, A, C refer to the own handle
		quint64 H(quint64 own = 0)
		{
			const int code = int(bits(4)), n = int(bits(4));
			quint64 w = 0;
			for (int k = 0; k < n; ++k) w = (w << 8) | RC();
			switch (code) {
				case 0x6: return own + 1;
				case 0x8: return own - 1;
				case 0xA: return own + w;
				case 0xC: return own - w;
				default: return w;
			}
		}
		int ENC()
		{
			const int idx = BS();
			if (idx) {
				const int flags = idx >> 8;
				if (flags & 0x80) BL();
				if (flags & 0x20) BL();
			}
			return idx & 0x1FF;
		}

	private:
		const QByteArray &m_d;
		qint64 m_p;
		bool m_bad = false;
};

/// object data from R2007 on: strings are in a stream of their own at the end
class ObjectBits : public Bits
{
	public:
		ObjectBits(const QByteArray &d, qint64 start, qint64 databits, bool r2007)
			: Bits(d, start), m_str(d, 0)
		{
			if (!r2007) return;
			qint64 s = start + databits - 1;             // last data bit: "has strings"
			Bits t(d, s);
			if (!t.B()) return;
			s -= 16;
			t.seek(s);
			qint64 size = t.RSu();
			if (size & 0x8000) {
				s -= 16;
				t.seek(s);
				size = (size & 0x7FFF) | (qint64(t.RSu()) << 15);
			}
			m_str.seek(s - size);
			m_has = true;
		}
		QString TV()
		{
			if (!m_has) return {};
			const int n = m_str.BS();
			if (n <= 0 || n > 4096) return {};
			const QByteArray b = m_str.raw(2 * n);
			QString s = QString::fromUtf16(reinterpret_cast<const char16_t *>(b.constData()), b.size() / 2);
			while (s.endsWith(QChar(0))) s.chop(1);
			return s;
		}
	private:
		Bits m_str;
		bool m_has = false;
};

/// block names come XML-escaped ("_x002d_" = "-")
QString unescape(QString n)
{
	static const QRegularExpression esc(QStringLiteral("_x([0-9a-fA-F]{4})_"));
	QRegularExpressionMatch m;
	while ((m = esc.match(n)).hasMatch())
		n.replace(m.capturedStart(), m.capturedLength(), QChar(m.captured(1).toInt(nullptr, 16)));
	return n;
}

} // namespace

bool DwgReader::read(const QByteArray &data)
{
	m_error.clear();
	if (!sections(data)) return false;
	if (!readObjects()) return false;
	for (const Object &o : std::as_const(m_objects)) {
		if (isEntity(o.type) && o.mode == 0 && m_objects.contains(o.owner)
		    && m_objects.value(o.owner).type == T_BLOCK_HEADER)
			m_contents[o.owner].append(o.handle);
	}
	for (auto &list : m_contents) std::sort(list.begin(), list.end());
	// the block(s) inserted in model space are the symbol
	QList<quint64> handles = m_objects.keys();
	std::sort(handles.begin(), handles.end());
	for (quint64 h : std::as_const(handles)) {
		const Object &o = m_objects[h];
		if ((o.type != T_INSERT && o.type != T_MINSERT) || o.mode != 2 || o.refs.isEmpty()) continue;
		const quint64 block = o.refs.first();
		if (!m_objects.contains(block)) continue;
		if (m_block.isEmpty()) m_block = unescape(m_objects.value(block).text);
		collect(block, {{o.p, m_objects.value(block).base, o.sx, o.sy, o.rot}}, 0);
	}
	if (m_lines.isEmpty() && m_texts.isEmpty()) {
		m_error = QStringLiteral("The DWG holds no block drawing in model space");
		return false;
	}
	// the block name ends in a GUID in WSCAD exports
	static const QRegularExpression guid(QStringLiteral("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
							    "[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$"));
	m_block.remove(guid);
	m_block = m_block.trimmed();
	findPins();
	return true;
}

bool DwgReader::sections(const QByteArray &data)
{
	m_version = QString::fromLatin1(data.left(6));
	if (m_version < QLatin1String("AC1018") || m_version > QLatin1String("AC1032")) {
		m_error = QStringLiteral("DWG version %1 is not supported (AutoCAD 2004 to 2018 only)").arg(m_version);
		return false;
	}
	// the R2004 file header (0x6C bytes at 0x80) is XOR-ed with a fixed pseudo-random stream
	QByteArray head = data.mid(0x80, 0x6C);
	quint32 seed = 1;
	for (char &c : head) {
		seed = seed * 0x343FD + 0x269EC3;
		c = char(uchar(c) ^ ((seed >> 16) & 0xFF));
	}
	if (!head.startsWith("AcFssFcAJMB")) {
		m_error = QStringLiteral("The DWG file header cannot be read");
		return false;
	}
	const quint32 map_id = le32(head, 0x50);
	const quint64 map_address = le64(head, 0x54);
	const quint32 info_id = le32(head, 0x5C);

	auto systemPage = [&](qint64 at) {
		const quint32 size = le32(data, at + 4), packed = le32(data, at + 8), kind = le32(data, at + 12);
		if (size > 64u * 1024 * 1024) return QByteArray();
		const QByteArray content = data.mid(int(at + 20), int(packed));
		return kind == 2 ? decompress(content, int(size)) : content;
	};
	// page map: page number -> file address (pages follow each other from 0x100)
	const QByteArray map = systemPage(qint64(map_address) + 0x100);
	QHash<quint32, qint64> pages;
	qint64 address = 0x100;
	for (int p = 0; p + 8 <= map.size();) {
		const qint32 nr = qint32(le32(map, p));
		const quint32 size = le32(map, p + 4);
		p += 8;
		if (nr < 0) p += 16;                                   // gap: parent, left, right, 0
		else pages.insert(quint32(nr), address);
		address += size;
	}
	if (!pages.contains(map_id) || !pages.contains(info_id)) {
		m_error = QStringLiteral("The DWG page map is incomplete");
		return false;
	}
	const QByteArray info = systemPage(pages.value(info_id));
	const quint32 count = le32(info, 0);
	for (int p = 20, k = 0; k < int(count) && p + 96 <= info.size(); ++k) {
		const quint64 total = le64(info, p);
		const quint32 page_count = le32(info, p + 8), max_size = le32(info, p + 12);
		const quint32 compressed = le32(info, p + 20);
		const QString name = QString::fromLatin1(info.mid(p + 32, 64).constData());
		p += 96;
		if (total > 64u * 1024 * 1024 || max_size > 64u * 1024 * 1024) {
			m_error = QStringLiteral("DWG section %1 is implausibly large").arg(name);
			return false;
		}
		QByteArray buffer(int(total), '\0');
		for (quint32 n = 0; n < page_count && p + 16 <= info.size(); ++n) {
			const quint32 page = le32(info, p);
			const quint64 start = le64(info, p + 8);
			p += 16;
			if (!pages.contains(page)) continue;
			const qint64 at = pages.value(page);
			// the data page header (32 bytes) is XOR-ed with 0x4164536b ^ its address
			const quint32 mask = 0x4164536B ^ quint32(at);
			const quint32 packed = le32(data, at + 8) ^ mask;
			const QByteArray content = data.mid(int(at + 32), int(packed));
			const QByteArray block = compressed == 2 ? decompress(content, int(max_size)) : content;
			const qint64 end = qMin<qint64>(qint64(start) + block.size(), qint64(total));
			if (qint64(start) < end)
				std::memcpy(buffer.data() + start, block.constData(), size_t(end - qint64(start)));
		}
		if (!name.isEmpty()) m_sections.insert(name, buffer);
	}
	if (!m_sections.contains(QStringLiteral("AcDb:Handles"))
	    || !m_sections.contains(QStringLiteral("AcDb:AcDbObjects"))) {
		m_error = QStringLiteral("The DWG has no object data");
		return false;
	}
	return true;
}

bool DwgReader::readObjects()
{
	// AcDb:Handles: blocks of (big-endian size, pairs of handle and offset deltas, CRC)
	const QByteArray map = m_sections.value(QStringLiteral("AcDb:Handles"));
	for (int p = 0; p + 2 <= map.size();) {
		const int size = (uchar(map.at(p)) << 8) | uchar(map.at(p + 1));
		if (size <= 2) break;
		Bits b(map, qint64(p + 2) * 8);
		const qint64 end = p + size;
		quint64 h = 0;
		qint64 address = 0;
		while ((b.pos() >> 3) < end && !b.bad()) {
			h += quint64(b.MC(false));
			address += b.MC();
			readObject(address, h);
		}
		p = int(end) + 2;
	}
	if (m_objects.isEmpty()) {
		m_error = QStringLiteral("No objects found in the DWG");
		return false;
	}
	return true;
}

void DwgReader::readObject(qint64 address, quint64 expected)
{
	const QByteArray &data = m_sections[QStringLiteral("AcDb:AcDbObjects")];
	if (address < 0 || address >= data.size()) return;
	const bool r2007 = m_version >= QLatin1String("AC1021");
	const bool r2010 = m_version >= QLatin1String("AC1024");
	Bits head(data, address * 8);
	const qint64 size = head.MS();
	const qint64 handlebits = r2010 ? head.MC(false) : 0;
	const qint64 start = head.pos();
	qint64 databits = size * 8 - handlebits;
	if (size <= 0 || databits <= 0 || (start >> 3) + size > data.size()) return;
	ObjectBits o(data, start, databits, r2007);

	Object ob;
	if (r2010) {
		const int c = o.BB();
		ob.type = c == 0 ? o.RC() : c == 1 ? o.RC() + 0x1F0 : o.RSu();
	} else {
		ob.type = o.BS();
		databits = o.RL();                                    // R2000 to R2007: size in bits
	}
	ob.handle = o.H();
	if (ob.handle != expected) return;
	for (int n = o.BS(), k = 0; n && k < 64 && !o.bad(); n = o.BS(), ++k) {    // extended data
		o.H();
		o.raw(n);
	}
	Bits h(data, start + databits);                              // handle stream
	const qint64 handles_end = start + size * 8;

	if (isEntity(ob.type)) {
		if (o.B()) o.raw(qint64(r2010 ? o.BLL() : quint64(o.RL())));   // graphic
		ob.mode = o.BB();
		const qint64 reactors = o.BL();
		const bool no_xdic = o.B();
		if (m_version < QLatin1String("AC1018")) o.B();       // nolinks (up to R2000)
		o.ENC();
		o.BD();                                               // linetype scale
		const int lt = o.BB(), ps = o.BB();
		const int mat = r2007 ? o.BB() : 0;
		if (r2007) o.RC();                                    // shadow
		QVector<bool> styles;
		if (r2010) styles << o.B() << o.B() << o.B();         // visual styles
		ob.invisible = o.BS() & 1;
		o.RC();                                               // lineweight

		switch (ob.type) {
			case T_LINE: {
				const bool z0 = o.B();
				const double x1 = o.RD(), x2 = o.DD(x1), y1 = o.RD(), y2 = o.DD(y1);
				if (!z0) { o.RD(); o.DD(0); }
				ob.p = {x1, y1};
				ob.q = {x2, y2};
				break;
			}
			case T_ARC: case T_CIRCLE:
				ob.p = o.P3BD();
				ob.r = o.BD();
				o.BT();
				o.BE();
				if (ob.type == T_ARC) { ob.a1 = o.BD(); ob.a2 = o.BD(); }
				break;
			case T_TEXT: case T_ATTRIB: case T_ATTDEF: {
				const int df = o.RC();
				if (!(df & 0x01)) o.RD();
				ob.p = o.P2RD();
				if (!(df & 0x02)) {
					const double ax = o.DD(ob.p.x());
					ob.align = {ax, o.DD(ob.p.y())};
					ob.has_align = true;
				}
				o.BE();
				o.BT();
				if (!(df & 0x04)) o.RD();
				ob.rot = (df & 0x08) ? 0.0 : o.RD();
				ob.h = o.RD();
				if (!(df & 0x10)) o.RD();
				ob.text = o.TV();
				if (!(df & 0x20)) o.BS();
				ob.halign = (df & 0x40) ? 0 : o.BS();
				ob.valign = (df & 0x80) ? 0 : o.BS();
				if (ob.type != T_TEXT) {
					if (r2010) o.RC();
					ob.tag = o.TV();
					o.BS();
					if (o.RC() & 1) ob.invisible = true;      // attribute flag 1: invisible
					o.B();
				}
				break;
			}
			case T_INSERT: case T_MINSERT: {
				ob.p = o.P3BD();
				switch (o.BB()) {
					case 3: ob.sx = ob.sy = 1; break;
					case 1: ob.sx = 1; ob.sy = o.DD(1); o.DD(1); break;
					case 2: ob.sx = ob.sy = o.RD(); break;
					default: ob.sx = o.RD(); ob.sy = o.DD(ob.sx); o.DD(ob.sx); break;
				}
				ob.rot = o.BD();
				o.P3BD();
				if (o.B()) o.BL();                                // has attributes: owned count
				break;
			}
			case T_LWPOLYLINE: {
				const int f = o.BS();
				if (f & 4) o.BD();
				if (f & 8) o.BD();
				if (f & 2) o.BD();
				if (f & 1) o.P3BD();
				const qint64 n = o.BL();
				const qint64 nb = (f & 16) ? o.BL() : 0;
				if (f & 1024) o.BL();                             // vertex ids
				if (f & 32) o.BL();                               // widths
				if (n < 0 || n > 100000 || nb < 0 || nb > n + 1) { ob.broken = true; break; }
				for (qint64 k = 0; k < n && !o.bad(); ++k) {
					if (k == 0) {
						ob.pts << o.P2RD();
					} else {
						const double x = o.DD(ob.pts.last().x());
						ob.pts << QPointF(x, o.DD(ob.pts.last().y()));
					}
				}
				for (qint64 k = 0; k < nb && !o.bad(); ++k) ob.bulges << o.BD();
				ob.closed = f & 512;
				break;
			}
			case T_SOLID:
				o.BT();
				o.BD();
				for (int k = 0; k < 4; ++k) ob.pts << o.P2RD();
				o.BE();
				break;
			case T_ELLIPSE: {
				ob.p = o.P3BD();
				const QPointF axis = o.P3BD();
				o.P3BD();
				ob.ratio = o.BD();
				ob.a1 = o.BD();
				ob.a2 = o.BD();
				ob.r = std::hypot(axis.x(), axis.y());
				ob.rot = std::atan2(axis.y(), axis.x());
				break;
			}
			case T_MTEXT: {
				ob.p = o.P3BD();
				o.P3BD();
				const QPointF dir = o.P3BD();
				ob.rot = std::atan2(dir.y(), dir.x());
				o.BD();
				if (r2007) o.BD();
				ob.h = o.BD();
				o.BS(); o.BS(); o.BD(); o.BD();
				ob.text = o.TV();
				break;
			}
			case T_BLOCK:
				ob.text = o.TV();
				break;
			default:
				break;
		}
		if (o.bad()) ob.broken = true;
		// handle stream: owner (mode 0), reactors, xdictionary, layer, linetype, material,
		// plot style, visual styles, then the entity's own references (insert: block header)
		if (ob.mode == 0) ob.owner = h.H(ob.handle);
		for (qint64 k = 0; k < reactors && k < 4096; ++k) h.H(ob.handle);
		if (!no_xdic) h.H(ob.handle);
		h.H(ob.handle);                                       // layer
		if (lt == 3) h.H(ob.handle);
		if (mat == 3) h.H(ob.handle);
		if (ps == 3) h.H(ob.handle);
		for (bool s : std::as_const(styles)) {
			if (s) h.H(ob.handle);
		}
		while (h.pos() + 8 <= handles_end && !h.bad() && ob.refs.size() < 4096) ob.refs << h.H(ob.handle);
	} else {
		const qint64 reactors = o.BL();
		const bool no_xdic = o.B();
		if (ob.type == T_BLOCK_HEADER) {
			ob.text = o.TV();
			o.B();                                            // 64 flag
			if (!r2007) o.BS();                               // xref index (gone from R2007 on)
			o.B();                                            // xref dependent
			o.B();                                            // anonymous
			o.B();                                            // has attributes
			const bool xref = o.B();
			const bool overlaid = o.B();
			o.B();                                            // loaded
			if (!xref && !overlaid) o.BL();                   // owned objects
			ob.base = o.P3BD();
		}
		ob.owner = h.H(ob.handle);
		for (qint64 k = 0; k < reactors && k < 4096; ++k) h.H(ob.handle);
		if (!no_xdic) h.H(ob.handle);
		if (o.bad()) ob.broken = true;
	}
	m_objects.insert(ob.handle, ob);
}

QPointF DwgReader::apply(const QVector<Transform> &stack, const QPointF &p)
{
	QPointF q = p;
	for (int k = stack.size() - 1; k >= 0; --k) {
		const Transform &t = stack.at(k);
		const double x = (q.x() - t.base.x()) * t.sx, y = (q.y() - t.base.y()) * t.sy;
		const double c = std::cos(t.rot), s = std::sin(t.rot);
		q = QPointF(t.origin.x() + c * x - s * y, t.origin.y() + s * x + c * y);
	}
	return q;
}

void DwgReader::addSegments(const Object &o, const QVector<Transform> &stack)
{
	auto polyline = [&](const QVector<QPointF> &pts, bool closed) {
		QVector<QPointF> mapped;
		for (const QPointF &p : pts) mapped << apply(stack, p);
		if (closed && mapped.size() > 2) {
			m_outlines << mapped;
			mapped << mapped.first();
		}
		for (int k = 1; k < mapped.size(); ++k) {
			if (QLineF(mapped.at(k - 1), mapped.at(k)).length() > 1e-6)
				m_lines << QLineF(mapped.at(k - 1), mapped.at(k));
		}
		// only a stroke of its own can be a terminal stub, not a piece of an outline or arc
		if (!closed && mapped.size() == 2) m_stubs << QLineF(mapped.at(0), mapped.at(1));
	};
	auto arc = [&](const QPointF &c, double r, double a0, double a1) {
		if (a1 < a0) a1 += 2 * M_PI;
		const int n = qMax(8, int((a1 - a0) / (M_PI / 16)));
		QVector<QPointF> pts;
		for (int k = 0; k <= n; ++k)
			pts << QPointF(c.x() + r * std::cos(a0 + (a1 - a0) * k / n), c.y() + r * std::sin(a0 + (a1 - a0) * k / n));
		polyline(pts, false);
	};
	switch (o.type) {
		case T_LINE:
			polyline({o.p, o.q}, false);
			break;
		case T_CIRCLE: {
			double scale = 1;
			for (const Transform &t : stack) scale *= qAbs(t.sx);
			m_circles << qMakePair(apply(stack, o.p), o.r * scale);
			break;
		}
		case T_ARC:
			arc(o.p, o.r, o.a1, o.a2);
			break;
		case T_ELLIPSE: {
			double a0 = o.a1, a1 = o.a2;
			if (a1 <= a0) a1 += 2 * M_PI;
			QVector<QPointF> pts;
			const int n = 32;
			for (int k = 0; k <= n; ++k) {
				const double t = a0 + (a1 - a0) * k / n;
				const double x = o.r * std::cos(t), y = o.r * o.ratio * std::sin(t);
				pts << QPointF(o.p.x() + std::cos(o.rot) * x - std::sin(o.rot) * y,
					       o.p.y() + std::sin(o.rot) * x + std::cos(o.rot) * y);
			}
			polyline(pts, false);
			break;
		}
		case T_SOLID:
			if (o.pts.size() == 4) polyline({o.pts[0], o.pts[1], o.pts[3], o.pts[2]}, true);
			break;
		case T_LWPOLYLINE: {
			QVector<QPointF> p = o.pts;
			if (p.isEmpty()) break;
			if (o.closed) p << p.first();
			QVector<QPointF> pts {p.first()};
			for (int k = 0; k + 1 < p.size(); ++k) {
				const double bulge = k < o.bulges.size() ? o.bulges.at(k) : 0;
				const QPointF a = p.at(k), b = p.at(k + 1);
				if (qAbs(bulge) > 1e-9) {                     // arc segment, bulge = tan(angle / 4)
					const double ang = 4 * std::atan(bulge), d = QLineF(a, b).length();
					if (d < 1e-9) continue;
					const double r = d / (2 * std::sin(ang / 2)), hh = r * std::cos(ang / 2);
					const QPointF m = (a + b) / 2, nrm(-(b.y() - a.y()) / d, (b.x() - a.x()) / d);
					const QPointF c = m + nrm * hh;
					const double a0 = std::atan2(a.y() - c.y(), a.x() - c.x());
					for (int s = 1; s <= 8; ++s)
						pts << QPointF(c.x() + qAbs(r) * std::cos(a0 + ang * s / 8),
							       c.y() + qAbs(r) * std::sin(a0 + ang * s / 8));
				} else {
					pts << b;
				}
			}
			if (o.closed && pts.size() > 1) pts.removeLast();
			polyline(pts, o.closed);
			break;
		}
		default:
			break;
	}
}

void DwgReader::collect(quint64 block, QVector<Transform> stack, int depth)
{
	double rotation = 0;
	for (const Transform &t : std::as_const(stack)) rotation += t.rot;
	for (quint64 eh : m_contents.value(block)) {
		const Object &o = m_objects[eh];
		if (o.broken || o.invisible) continue;
		if ((o.type == T_INSERT || o.type == T_MINSERT) && !o.refs.isEmpty() && depth < 8
		    && m_objects.contains(o.refs.first())) {
			QVector<Transform> inner = stack;
			inner << Transform {o.p, m_objects.value(o.refs.first()).base, o.sx, o.sy, o.rot};
			collect(o.refs.first(), inner, depth + 1);
		} else if (o.type == T_TEXT || o.type == T_ATTRIB || o.type == T_MTEXT) {
			const QString s = o.text.trimmed();
			if (s.isEmpty()) continue;
			EplanDxfText t;
			t.text = s;
			t.tag = o.type == T_ATTRIB ? o.tag : QString();
			// the insertion point is the start of the baseline (computed by AutoCAD for aligned
			// texts too), where QElectroTech places texts as well
			t.insert = apply(stack, o.p);
			t.anchor = t.insert;
			t.height = o.h > 0 ? o.h : 2.5;
			t.rotation = (o.rot + rotation) * 180 / M_PI;
			m_texts << t;
		} else {
			addSegments(o, stack);
		}
	}
}

void DwgReader::findPins()
{
	if (m_outlines.isEmpty()) return;
	auto bounds = [](const QVector<QPointF> &p) {
		double x0 = p.first().x(), x1 = x0, y0 = p.first().y(), y1 = y0;
		for (const QPointF &q : p) {
			x0 = qMin(x0, q.x()); x1 = qMax(x1, q.x());
			y0 = qMin(y0, q.y()); y1 = qMax(y1, q.y());
		}
		return QRectF(QPointF(x0, y0), QPointF(x1, y1));
	};
	QRectF frame = bounds(m_outlines.first());
	for (const auto &p : std::as_const(m_outlines)) {
		const QRectF r = bounds(p);
		if (r.width() * r.height() > frame.width() * frame.height()) frame = r;
	}
	const double e = 0.05;
	auto onFrame = [&](const QPointF &p) {
		const bool hx = p.x() >= frame.left() - e && p.x() <= frame.right() + e;
		const bool vy = p.y() >= frame.top() - e && p.y() <= frame.bottom() + e;
		return (hx && (qAbs(p.y() - frame.top()) < e || qAbs(p.y() - frame.bottom()) < e))
		       || (vy && (qAbs(p.x() - frame.left()) < e || qAbs(p.x() - frame.right()) < e));
	};
	for (const QLineF &l : std::as_const(m_stubs)) {
		if (l.length() > 5.01) continue;
		QPointF a = l.p1(), b = l.p2();
		if (onFrame(b) && !onFrame(a)) std::swap(a, b);
		if (!onFrame(a) || onFrame(b)) continue;
		// name: the nearest text without blanks beside the stub
		const QPointF mid = (a + b) / 2;
		QVector<QPair<double, QString>> near;
		for (const EplanDxfText &t : std::as_const(m_texts)) {
			const double d = QLineF(t.insert, mid).length();
			if (d < 3 && !t.text.contains(QLatin1Char(' '))) near << qMakePair(d, t.text);
		}
		std::sort(near.begin(), near.end());
		EplanDxfPin p;
		p.pos = b;
		p.name = near.isEmpty() ? QString::number(m_pins.size() + 1) : near.first().second;
		m_pins << p;
	}
	// top-left to bottom-right, as the element builder expects
	std::stable_sort(m_pins.begin(), m_pins.end(), [](const EplanDxfPin &a, const EplanDxfPin &b) {
		if (qAbs(a.pos.y() - b.pos.y()) > 0.01) return a.pos.y() > b.pos.y();
		return a.pos.x() < b.pos.x();
	});
}
