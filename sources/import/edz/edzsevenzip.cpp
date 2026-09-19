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
#include "edzsevenzip.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <vector>

// Bundled public-domain LZMA SDK 7-Zip reader (decode only). The headers
// self-guard with EXTERN_C_BEGIN, so they are included directly from C++.
#include "lzma/7z.h"
#include "lzma/7zAlloc.h"
#include "lzma/7zCrc.h"
#include "lzma/7zFile.h"
#include "lzma/7zTypes.h"

namespace {
const size_t kInputBufSize = (size_t)1 << 18;
} // namespace

bool sevenZipExtract(const QString &archivePath, const QString &destDir,
		     QString &error)
{
	error.clear();

	ISzAlloc allocImp     = { SzAlloc, SzFree };
	ISzAlloc allocTempImp = { SzAllocTemp, SzFreeTemp };

	CFileInStream archiveStream;
#ifdef _WIN32
	if (InFile_OpenW(&archiveStream.file,
			 reinterpret_cast<const wchar_t *>(archivePath.utf16())) != 0)
#else
	const QByteArray apath = QFile::encodeName(archivePath);
	if (InFile_Open(&archiveStream.file, apath.constData()) != 0)
#endif
	{
		error = QStringLiteral("Cannot open archive: %1").arg(archivePath);
		return false;
	}

	FileInStream_CreateVTable(&archiveStream);
	archiveStream.wres = 0;

	CLookToRead2 lookStream;
	LookToRead2_CreateVTable(&lookStream, False);
	lookStream.buf = nullptr;

	SRes res = SZ_OK;
	lookStream.buf = static_cast<Byte *>(ISzAlloc_Alloc(&allocImp, kInputBufSize));
	if (!lookStream.buf) {
		res = SZ_ERROR_MEM;
	} else {
		lookStream.bufSize = kInputBufSize;
		lookStream.realStream = &archiveStream.vt;
		LookToRead2_INIT(&lookStream)
	}

	CrcGenerateTable();

	CSzArEx db;
	SzArEx_Init(&db);

	if (res == SZ_OK) {
		res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
	}

	bool ok = (res == SZ_OK);
	if (ok) {
		const QDir dest(destDir);
		// Cache shared across entries of the same solid block (see SzArEx_Extract).
		UInt32 blockIndex = 0xFFFFFFFF;
		Byte *outBuffer = nullptr;
		size_t outBufferSize = 0;
		std::vector<UInt16> name;

		for (UInt32 i = 0; i < db.NumFiles && res == SZ_OK; ++i) {
			const BoolInt is_dir = SzArEx_IsDir(&db, i);

			const size_t len = SzArEx_GetFileNameUtf16(&db, i, nullptr);
			name.resize(len ? len : 1);
			SzArEx_GetFileNameUtf16(&db, i, name.data());
			QString rel = QString::fromUtf16(
				reinterpret_cast<const char16_t *>(name.data()));
			rel.replace(QLatin1Char('\\'), QLatin1Char('/'));
			if (rel.isEmpty()) {
				continue;
			}

			if (is_dir) {
				dest.mkpath(rel);
				continue;
			}

			const QString out_path = dest.filePath(rel);
			dest.mkpath(QFileInfo(out_path).path());

			size_t offset = 0, outSizeProcessed = 0;
			res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex,
					     &outBuffer, &outBufferSize,
					     &offset, &outSizeProcessed,
					     &allocImp, &allocTempImp);
			if (res != SZ_OK) {
				break;
			}

			QFile f(out_path);
			if (!f.open(QIODevice::WriteOnly)) {
				error = QStringLiteral("Cannot write %1").arg(out_path);
				res = SZ_ERROR_FAIL;
				break;
			}
			const qint64 want = static_cast<qint64>(outSizeProcessed);
			if (f.write(reinterpret_cast<const char *>(outBuffer + offset),
				    want) != want) {
				error = QStringLiteral("Short write to %1").arg(out_path);
				res = SZ_ERROR_FAIL;
				f.close();
				break;
			}
			f.close();
		}
		ISzAlloc_Free(&allocImp, outBuffer);
	}

	SzArEx_Free(&db, &allocImp);
	ISzAlloc_Free(&allocImp, lookStream.buf);
	File_Close(&archiveStream.file);

	if (res != SZ_OK) {
		if (error.isEmpty()) {
			error = QStringLiteral("7z decode failed (code %1)")
					.arg(static_cast<int>(res));
		}
		return false;
	}
	return true;
}
