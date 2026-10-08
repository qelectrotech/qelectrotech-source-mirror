#!/usr/bin/env python3
"""Run identical Release component workloads; preserve samples and variability."""
import argparse
import csv
import io
import os
import pathlib
import statistics
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--reference', required=True)
p.add_argument('--modified', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
env = os.environ.copy()
env['QT_QPA_PLATFORM'] = 'offscreen'
rows = []
# Compare x1 first, then exercise every remaining size on the modified code.
for size in [10, 2.5, 5, 7.5, 20, 30]:
    for count in [16, 2048]:
        for operation in ['zoom', 'pan', 'manipulate']:
            variants = [('reference', a.reference), ('modified', a.modified)] if size == 10 else [('modified', a.modified)]
            for variant, binary in variants:
                result = subprocess.run([binary, str(size), str(count), operation], env=env,
                                        capture_output=True, text=True, check=True, timeout=120)
                samples = list(csv.DictReader(io.StringIO(result.stdout)))
                assert len(samples) == 7
                rows.extend(dict(variant=variant, **sample) for sample in samples)
output = pathlib.Path(a.output)
output.mkdir(parents=True, exist_ok=True)
with (output / 'samples.csv').open('w', newline='', encoding='utf-8') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
groups = {}
for row in rows:
    key = (row['variant'], row['size'], row['handles'], row['operation'])
    groups.setdefault(key, []).append(row)
lines = ['# Release handle refresh measurements', '',
         'Synthetic QGraphicsView raster rendering at 1280×800, offscreen, on the same PC.',
         'Each sample comprises 120 frames; 20 warm-up frames precede seven samples.',
         '16 or 2048 handles; same scenes and workload code for both builds.',
         'Manipulate changes position, rectangle size and rotation of all scene objects.',
         'This is not full-editor mouse latency, GPU frame presentation or human usability.', '',
         '| Code | Factor | Handles | Workload | Mean ms/frame ± SD | Min–max | CPU ms/frame | Working set MiB |',
         '|---|---:|---:|---|---:|---:|---:|---:|']
for (variant, size, count, operation), samples in groups.items():
    wall = [float(s['ms_per_frame']) for s in samples]
    cpu = [float(s['cpu_ms_per_frame']) for s in samples]
    memory = [int(s['working_set_bytes']) / 1048576 for s in samples]
    lines.append(f'| {variant} | {float(size)/10:g} | {count} | {operation} | '
                 f'{statistics.mean(wall):.3f} ± {statistics.stdev(wall):.3f} | '
                 f'{min(wall):.3f}–{max(wall):.3f} | {statistics.mean(cpu):.3f} | '
                 f'{statistics.median(memory):.1f} |')
(output / 'measurements.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
print(f'{len(rows)} samples saved in {output}')
