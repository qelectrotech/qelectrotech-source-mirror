#!/usr/bin/env python3
# Copyright 2006-2026 The QElectroTech Team
# This file is part of QElectroTech.
#
# QElectroTech is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# QElectroTech is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
"""Record raw USB reports from a 3Dconnexion 3D mouse, for QElectroTech.

QElectroTech's Windows/macOS 3D mouse support reads the device directly
over USB, so it has to decode each device model's raw reports itself.
This records what your device sends while you make a few guided
movements, and saves it to one file you can attach to the discussion.
Nothing is sent anywhere.

    sudo python3 spacemouse-capture.py            # finds the device itself
    sudo python3 spacemouse-capture.py --list     # just show what it finds
    sudo python3 spacemouse-capture.py --seconds 3  # 3x the time per step

sudo is needed because /dev/hidraw* is usually readable by root only.
spacenavd can keep running. If the recording comes out empty, stop it
(`sudo systemctl stop spacenavd`) and try again.

Only the standard library is used, so it runs on any Linux with Python 3.
"""
import argparse
import datetime
import glob
import json
import os
import platform
import select
import sys
import time

VENDORS = {0x046D: 'Logitech (older 3Dconnexion)', 0x256F: '3Dconnexion'}

STEPS = [
    ('rest', 'Do not touch the device.', 3),
    ('right', 'Push the cap to the RIGHT and hold it, then let go.', 4),
    ('left', 'Push the cap to the LEFT and hold it, then let go.', 4),
    ('away', 'Push the cap AWAY from you and hold it, then let go.', 4),
    ('toward', 'Pull the cap TOWARDS you and hold it, then let go.', 4),
    ('down', 'Press the cap DOWN and hold it, then let go.', 4),
    ('up', 'Lift the cap UP and hold it, then let go.', 4),
    ('twist_cw', 'TWIST the cap CLOCKWISE (seen from above) and hold, then let go.', 4),
    ('twist_ccw', 'TWIST the cap ANTICLOCKWISE and hold, then let go.', 4),
    ('tilt_away', 'TILT the cap AWAY from you and hold, then let go.', 4),
    ('tilt_right', 'TILT the cap to the RIGHT and hold, then let go.', 4),
    ('buttons', 'Press each button once, slowly, one at a time, in any order.', 15),
]


def find_devices():
    """Return [{hidraw, name, vendor, product, sysfs}] for 3Dconnexion devices."""
    found = []
    for sysdir in sorted(glob.glob('/sys/class/hidraw/hidraw*')):
        try:
            with open(os.path.join(sysdir, 'device', 'uevent')) as f:
                uevent = dict(line.strip().split('=', 1) for line in f if '=' in line)
        except OSError:
            continue
        # HID_ID=0003:0000256F:0000C635  (bus:vendor:product)
        try:
            _bus, vendor, product = (int(x, 16) for x in uevent.get('HID_ID', '').split(':'))
        except ValueError:
            continue
        if vendor not in VENDORS:
            continue
        found.append({
            'hidraw': '/dev/' + os.path.basename(sysdir),
            'name': uevent.get('HID_NAME', '?'),
            'vendor': '%04x' % vendor,
            'product': '%04x' % product,
            'sysfs': sysdir,
        })
    return found


def read_descriptor(sysdir):
    try:
        with open(os.path.join(sysdir, 'device', 'report_descriptor'), 'rb') as f:
            return f.read().hex()
    except OSError as e:
        return 'unreadable: %s' % e


def record(fd, seconds):
    """Read every report arriving within `seconds`; return [[ms, hex], ...]."""
    reports = []
    start = time.monotonic()
    while True:
        left = seconds - (time.monotonic() - start)
        if left <= 0:
            return reports
        ready, _, _ = select.select([fd], [], [], left)
        if not ready:
            continue
        try:
            data = os.read(fd, 64)
        except BlockingIOError:
            continue
        if not data:  # only a test FIFO with no writer does this
            time.sleep(0.01)
            continue
        reports.append([round((time.monotonic() - start) * 1000, 1), data.hex()])


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--list', action='store_true', help='only list matching devices')
    ap.add_argument('--device', help='hidraw path, if more than one device is found')
    ap.add_argument('--descriptor', help=argparse.SUPPRESS)  # testing without a device
    ap.add_argument('--yes', action='store_true', help=argparse.SUPPRESS)  # no Enter prompts
    ap.add_argument('-o', '--output', help='output file (default: spacemouse-capture-<product>.json)')
    ap.add_argument('--seconds', type=float, default=1.0,
                     help='multiply each step\'s recording time (default: 1.0, e.g. 3 triples it)')
    args = ap.parse_args()

    devices = find_devices()
    if args.list:
        for d in devices:
            print('%(hidraw)s  %(vendor)s:%(product)s  %(name)s' % d)
        if not devices:
            print('No 3Dconnexion device found under /sys/class/hidraw.')
        return 0 if devices else 1

    if args.device:
        dev = next((d for d in devices if d['hidraw'] == args.device),
                   {'hidraw': args.device, 'name': '?', 'vendor': '?', 'product': '?', 'sysfs': None})
    elif len(devices) == 1:
        dev = devices[0]
    elif not devices:
        sys.exit('No 3Dconnexion device found. Is it plugged in? (try --list)')
    else:
        sys.exit('Several devices found, pick one with --device:\n' +
                 '\n'.join('  %(hidraw)s  %(name)s' % d for d in devices))

    if args.descriptor:
        with open(args.descriptor, 'rb') as f:
            descriptor = f.read().hex()
    elif dev['sysfs']:
        descriptor = read_descriptor(dev['sysfs'])
    else:
        descriptor = 'unknown'

    try:
        fd = os.open(dev['hidraw'], os.O_RDONLY | os.O_NONBLOCK)
    except PermissionError:
        sys.exit('Permission denied on %s -- run with sudo.' % dev['hidraw'])

    print('Recording from %s (%s, %s:%s).' % (dev['hidraw'], dev['name'], dev['vendor'], dev['product']))
    print('For each step, press Enter, do the movement, and wait for the next prompt.\n')

    result = {
        'tool': 'spacemouse-capture.py 1',
        'date': datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds'),
        'system': platform.platform(),
        'device': {k: dev[k] for k in ('name', 'vendor', 'product')},
        'report_descriptor': descriptor,
        'steps': [],
    }
    try:
        for i, (key, text, seconds) in enumerate(STEPS, 1):
            seconds = seconds * args.seconds
            print('[%d/%d] %s' % (i, len(STEPS), text))
            if not args.yes:
                input('      Press Enter to start (%.0f s)... ' % seconds)
            reports = record(fd, seconds)
            print('      %d reports recorded.\n' % len(reports))
            result['steps'].append({'step': key, 'instruction': text, 'reports': reports})
    except KeyboardInterrupt:
        print('\nStopped early -- saving what was recorded so far.')
    finally:
        os.close(fd)

    out = args.output or 'spacemouse-capture-%s.json' % dev['product']
    with open(out, 'w') as f:
        json.dump(result, f, indent=1)
    total = sum(len(s['reports']) for s in result['steps'])
    print('Saved %s (%d reports in total).' % (out, total))
    if total == 0:
        print('Nothing was recorded. Try stopping spacenavd first: sudo systemctl stop spacenavd')
    else:
        print('Please attach this file to discussion #599. Thank you!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
