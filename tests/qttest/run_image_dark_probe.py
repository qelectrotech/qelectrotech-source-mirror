#!/usr/bin/env python3
"""Link the real image/widget integration probe using an existing Ninja app build.

Reuses app objects without duplicating the app or introducing a core-library
refactor. No application source or existing executable is replaced. The
generated manifest, probe binary, reports and export fixtures stay in build/.
"""
import argparse
import pathlib
import re
import subprocess
import sys

import ninja_probe
import zlib

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True)
parser.add_argument('--binary', required=True)
parser.add_argument('--source', required=True)
parser.add_argument('--fixture', required=True)
args = parser.parse_args()
build = pathlib.Path(args.build).resolve()
binary = pathlib.Path(args.binary).resolve()
source = pathlib.Path(args.source).resolve()
fixture = pathlib.Path(args.fixture).resolve()
probe_binary = ninja_probe.build_probe(build, binary, source, 'image-dark-probe.ninja')
output = build / 'image-dark-integration'
output.mkdir(exist_ok=True)
report = output / 'results.txt'
result = subprocess.run([str(probe_binary), str(fixture), str(report), str(output)], timeout=ninja_probe.timeout(90))
if report.exists():
    print(report.read_text(encoding='utf-8', errors='replace'))
if result.returncode == 0:
    def print_streams(path):
        streams = re.findall(rb'\nstream\r?\n(.*?)\nendstream', path.read_bytes(), re.DOTALL)
        decoded = []
        for stream in streams:
            try:
                data = zlib.decompress(stream)
            except zlib.error:
                data = stream
            # Qt assigns every PDF a fresh XMP DocumentID (and dates).
            # Metadata is not printing content and must not be compared
            # as if it were a graphics/image/font stream.
            if b'<x:xmpmeta' not in data:
                decoded.append(data)
        return decoded
    first = print_streams(output / 'print-0.pdf')
    second = print_streams(output / 'print-1.pdf')
    if not first or first != second:
        sys.exit('PDF printing changed the decoded graphics/image/font streams.')
    print('PASS: PDF print graphics/image/font streams identical for both choices.')
    try:
        import pymupdf
    except ImportError:
        print('PDF raster comparison not run: optional PyMuPDF is unavailable.')
    else:
        documents = [pymupdf.open(output / f'print-{i}.pdf') for i in range(2)]
        if len(documents[0]) != len(documents[1]):
            sys.exit('PDF page counts differ.')
        for page in range(len(documents[0])):
            pixels = [doc[page].get_pixmap(dpi=144, alpha=False) for doc in documents]
            if (pixels[0].width, pixels[0].height, pixels[0].samples) != (pixels[1].width, pixels[1].height, pixels[1].samples):
                sys.exit('Rasterized PDF print pages differ.')
            for i, image in enumerate(pixels):
                image.save(output / f'print-{i}-page-{page}.png')
        print('PASS: PDF print pages rendered at 144 dpi are pixel-identical.')
sys.exit(result.returncode)
