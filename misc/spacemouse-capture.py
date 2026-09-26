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
r"""Record raw USB reports from a 3Dconnexion 3D mouse, for QElectroTech.

QElectroTech's Windows/macOS 3D mouse support reads the device directly
over USB, so it has to decode each device model's raw reports itself.
This records what your device sends while you make a few guided
movements, and saves it to one file you can attach to discussion #599.
Nothing is sent anywhere. One recording per device model is enough, from
any of the three systems: the device sends the same reports on all of them.

It takes about two minutes. Each step says what to do: press Enter, make
the movement and hold it until the next prompt. Push firmly.

Linux
    Python 3 is already installed.
    1. Download:  curl -LO https://raw.githubusercontent.com/qelectrotech/qelectrotech-source-mirror/master/misc/spacemouse-capture.py
    2. List:      sudo python3 spacemouse-capture.py --list
    3. Record:    sudo python3 spacemouse-capture.py --seconds 3
       If --list shows several devices (a Logitech receiver shows up as
       several), add --device /dev/hidrawN with the 3D mouse's line.
    sudo is needed because /dev/hidraw* is readable by root only.
    spacenavd can keep running; if the recording comes out empty, stop it
    (sudo systemctl stop spacenavd) and try again.

macOS
    Python 3 comes with the Xcode command line tools; if "python3" asks
    to install them, accept. Open Terminal, then:
    1. Download:  curl -LO https://raw.githubusercontent.com/qelectrotech/qelectrotech-source-mirror/master/misc/spacemouse-capture.py
    2. Record:    python3 spacemouse-capture.py --seconds 3
    No sudo. If it says another program holds the device, quit 3DxWare
    (or uninstall it) and try again. If it says macOS refused access,
    allow Terminal in System Settings > Privacy & Security > Input
    Monitoring, and try again. (Rather not use Terminal? The SpaceMouse
    Check app does the same with a window: see discussion #599.)

Windows
    Install Python 3 from https://www.python.org/downloads/ (tick "Add
    python.exe to PATH") or from the Microsoft Store. Open a Command
    Prompt (Windows key, type cmd, Enter), then:
    1. cd %USERPROFILE%\Downloads
    2. Download:  curl -LO https://raw.githubusercontent.com/qelectrotech/qelectrotech-source-mirror/master/misc/spacemouse-capture.py
    3. Record:    python spacemouse-capture.py --seconds 3
    No administrator rights needed, and 3DxWare can keep running.
    Windows does not give out the device's report descriptor, so a
    recording made on Linux or macOS is a little more complete.

Every system
    --list          only show the 3D mice found
    --device PATH   pick one, if several are found (a path from --list)
    --seconds N     multiply the time per step (3 triples it)
    -o FILE         where to save (default: spacemouse-capture-<id>.json)

The file is saved in the current folder: attach it to discussion #599.
Only Python's standard library is used.
"""
import argparse
import datetime
import glob
import json
import os
import platform
import select
import subprocess
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

# Processes that may also be reading the device.
OTHER_READERS = ('3dconnexion', '3dx', 'spacenavd')  # 3DxWare: 3DconnexionHelper, 3DxNLServer...


def other_readers():
    if sys.platform == 'win32':
        try:
            out = subprocess.run(['tasklist', '/fo', 'csv', '/nh'], capture_output=True,
                                 text=True, timeout=10).stdout
        except (OSError, subprocess.SubprocessError):
            return ['unknown']
        names = {line.split('","')[0].strip('"') for line in out.splitlines() if line}
        return sorted(n for n in names if any(r in n.lower() for r in OTHER_READERS))
    try:
        out = subprocess.run(['ps', '-A', '-o', 'comm='], capture_output=True,
                             text=True, timeout=5).stdout
    except (OSError, subprocess.SubprocessError):
        return ['unknown']
    names = {os.path.basename(line.strip()) for line in out.splitlines()}
    return sorted(n for n in names if any(r in n.lower() for r in OTHER_READERS))


# --- Linux: /dev/hidraw ----------------------------------------------------

class HidrawDevice:
    backend = 'hidraw'

    def __init__(self, path, name='?', vendor='?', product='?', sysfs=None):
        self.path, self.name, self.vendor, self.product = path, name, vendor, product
        self.sysfs = sysfs
        self.fd = None

    @staticmethod
    def find():
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
            found.append(HidrawDevice('/dev/' + os.path.basename(sysdir),
                                      uevent.get('HID_NAME', '?'),
                                      '%04x' % vendor, '%04x' % product, sysdir))
        return found

    def descriptor(self):
        if not self.sysfs:
            return 'unknown'
        try:
            with open(os.path.join(self.sysfs, 'device', 'report_descriptor'), 'rb') as f:
                return f.read().hex()
        except OSError as e:
            return 'unreadable: %s' % e

    def open(self):
        try:
            self.fd = os.open(self.path, os.O_RDONLY | os.O_NONBLOCK)
        except PermissionError:
            sys.exit('Permission denied on %s -- run with sudo.' % self.path)

    def record(self, seconds):
        """Read every report arriving within `seconds`; return [[ms, hex], ...]."""
        reports = []
        start = time.monotonic()
        while True:
            left = seconds - (time.monotonic() - start)
            if left <= 0:
                return reports
            ready, _, _ = select.select([self.fd], [], [], left)
            if not ready:
                continue
            try:
                data = os.read(self.fd, 64)
            except BlockingIOError:
                continue
            if not data:  # only a test FIFO with no writer does this
                time.sleep(0.01)
                continue
            reports.append([round((time.monotonic() - start) * 1000, 1), data.hex()])

    def drain(self):
        """Drop reports queued while waiting for Enter."""
        while True:
            try:
                if not os.read(self.fd, 64):
                    return
            except BlockingIOError:
                return

    def close(self):
        os.close(self.fd)

    def empty_hint(self):
        return 'Try stopping spacenavd first: sudo systemctl stop spacenavd'


# --- macOS: IOKit, the same calls hidapi's mac backend makes ----------------

class MacHid:
    """ctypes bindings to the few CoreFoundation/IOKit calls needed."""

    def __init__(self):
        import ctypes as c
        self.c = c
        cf = c.CDLL('/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation')
        io = c.CDLL('/System/Library/Frameworks/IOKit.framework/IOKit')
        vp, i32, u32, idx = c.c_void_p, c.c_int32, c.c_uint32, c.c_long

        def fn(lib, name, res, *args):
            f = getattr(lib, name)
            f.restype, f.argtypes = res, list(args)
            return f

        self.CFStringCreateWithCString = fn(cf, 'CFStringCreateWithCString', vp, vp, c.c_char_p, u32)
        self.CFStringGetCString = fn(cf, 'CFStringGetCString', c.c_bool, vp, c.c_char_p, idx, u32)
        self.CFGetTypeID = fn(cf, 'CFGetTypeID', c.c_ulong, vp)
        self.CFNumberGetTypeID = fn(cf, 'CFNumberGetTypeID', c.c_ulong)
        self.CFStringGetTypeID = fn(cf, 'CFStringGetTypeID', c.c_ulong)
        self.CFDataGetTypeID = fn(cf, 'CFDataGetTypeID', c.c_ulong)
        self.CFNumberGetValue = fn(cf, 'CFNumberGetValue', c.c_bool, vp, idx, vp)
        self.CFDataGetLength = fn(cf, 'CFDataGetLength', idx, vp)
        self.CFDataGetBytePtr = fn(cf, 'CFDataGetBytePtr', vp, vp)
        self.CFSetGetCount = fn(cf, 'CFSetGetCount', idx, vp)
        self.CFSetGetValues = fn(cf, 'CFSetGetValues', None, vp, c.POINTER(vp))
        self.CFRunLoopGetCurrent = fn(cf, 'CFRunLoopGetCurrent', vp)
        self.CFRunLoopRunInMode = fn(cf, 'CFRunLoopRunInMode', i32, vp, c.c_double, c.c_bool)
        self.kCFRunLoopDefaultMode = vp.in_dll(cf, 'kCFRunLoopDefaultMode').value

        self.IOHIDManagerCreate = fn(io, 'IOHIDManagerCreate', vp, vp, u32)
        self.IOHIDManagerSetDeviceMatching = fn(io, 'IOHIDManagerSetDeviceMatching', None, vp, vp)
        self.IOHIDManagerScheduleWithRunLoop = fn(io, 'IOHIDManagerScheduleWithRunLoop', None, vp, vp, vp)
        self.IOHIDManagerCopyDevices = fn(io, 'IOHIDManagerCopyDevices', vp, vp)
        self.IOHIDDeviceGetProperty = fn(io, 'IOHIDDeviceGetProperty', vp, vp, vp)
        self.IOHIDDeviceOpen = fn(io, 'IOHIDDeviceOpen', i32, vp, u32)
        self.IOHIDDeviceClose = fn(io, 'IOHIDDeviceClose', i32, vp, u32)
        self.IOHIDDeviceScheduleWithRunLoop = fn(io, 'IOHIDDeviceScheduleWithRunLoop', None, vp, vp, vp)
        self.IOHIDDeviceUnscheduleFromRunLoop = fn(io, 'IOHIDDeviceUnscheduleFromRunLoop', None, vp, vp, vp)
        # void (*)(void *ctx, IOReturn, void *sender, IOHIDReportType, uint32_t id, uint8_t *, CFIndex)
        self.ReportCallback = c.CFUNCTYPE(None, vp, i32, vp, c.c_int, u32, c.POINTER(c.c_uint8), idx)
        self.IOHIDDeviceRegisterInputReportCallback = fn(
            io, 'IOHIDDeviceRegisterInputReportCallback', None, vp, vp, idx, self.ReportCallback, vp)

    def cfstr(self, s):
        return self.CFStringCreateWithCString(None, s.encode(), 0x08000100)  # UTF-8

    def prop(self, dev, key):
        """A device property as int, str or bytes, or None."""
        c = self.c
        ref = self.IOHIDDeviceGetProperty(dev, self.cfstr(key))
        if not ref:
            return None
        t = self.CFGetTypeID(ref)
        if t == self.CFNumberGetTypeID():
            v = c.c_int64()
            self.CFNumberGetValue(ref, 4, c.byref(v))  # kCFNumberSInt64Type
            return v.value
        if t == self.CFStringGetTypeID():
            buf = c.create_string_buffer(256)
            return buf.value.decode('utf-8', 'replace') if self.CFStringGetCString(
                ref, buf, len(buf), 0x08000100) else None
        if t == self.CFDataGetTypeID():
            return c.string_at(self.CFDataGetBytePtr(ref), self.CFDataGetLength(ref))
        return None


class MacDevice:
    backend = 'iokit'
    _hid = None

    def __init__(self, ref, name, vendor, product, location):
        self.ref, self.name, self.vendor, self.product = ref, name, vendor, product
        self.path = 'iokit:%08x' % location
        self.reports = []
        self.start = time.monotonic()

    @classmethod
    def hid(cls):
        if cls._hid is None:
            cls._hid = MacHid()
        return cls._hid

    @classmethod
    def find(cls):
        h = cls.hid()
        mgr = h.IOHIDManagerCreate(None, 0)
        h.IOHIDManagerSetDeviceMatching(mgr, None)
        h.IOHIDManagerScheduleWithRunLoop(mgr, h.CFRunLoopGetCurrent(), h.kCFRunLoopDefaultMode)
        devset = h.IOHIDManagerCopyDevices(mgr)
        if not devset:
            return []
        n = h.CFSetGetCount(devset)
        refs = (h.c.c_void_p * n)()
        h.CFSetGetValues(devset, refs)
        found = []
        for ref in refs:
            vendor = h.prop(ref, 'VendorID')
            if vendor not in VENDORS:
                continue
            # The same test as QET's SpaceMouseHid::isSpaceMouse(): Generic
            # Desktop / Multi-axis Controller, or no usage at all. This also
            # skips Logitech's ordinary mice and keyboards.
            usage = (h.prop(ref, 'PrimaryUsagePage') or 0, h.prop(ref, 'PrimaryUsage') or 0)
            if usage not in ((1, 8), (0, 0)):
                continue
            found.append(cls(ref, h.prop(ref, 'Product') or '?', '%04x' % vendor,
                             '%04x' % (h.prop(ref, 'ProductID') or 0),
                             h.prop(ref, 'LocationID') or 0))
        return sorted(found, key=lambda d: d.path)

    def descriptor(self):
        d = self.hid().prop(self.ref, 'ReportDescriptor')
        return d.hex() if d else 'unreadable'

    def open(self):
        h = self.hid()
        ret = h.IOHIDDeviceOpen(self.ref, 0) & 0xFFFFFFFF  # kIOHIDOptionsTypeNone: shared
        if ret == 0xE00002C5:
            sys.exit('The device is held exclusively by another program '
                     '(kIOReturnExclusiveAccess). Quit 3DxWare and try again.')
        if ret in (0xE00002E2, 0xE00002C1):
            sys.exit('macOS refused access (0x%08X). Allow Terminal under System Settings > '
                     'Privacy & Security > Input Monitoring, then try again.' % ret)
        if ret:
            sys.exit('Could not open the device (IOReturn 0x%08X).' % ret)
        size = h.prop(self.ref, 'MaxInputReportSize') or 64
        self.buf = h.c.create_string_buffer(size)

        def on_report(_ctx, _result, _sender, _type, _id, data, length):
            self.reports.append([round((time.monotonic() - self.start) * 1000, 1),
                                 h.c.string_at(data, length).hex()])
        self.callback = h.ReportCallback(on_report)  # must outlive the device
        h.IOHIDDeviceRegisterInputReportCallback(self.ref, self.buf, size, self.callback, None)
        h.IOHIDDeviceScheduleWithRunLoop(self.ref, h.CFRunLoopGetCurrent(), h.kCFRunLoopDefaultMode)

    def record(self, seconds):
        h = self.hid()
        self.reports, self.start = [], time.monotonic()
        while True:
            left = seconds - (time.monotonic() - self.start)
            if left <= 0:
                return self.reports
            h.CFRunLoopRunInMode(h.kCFRunLoopDefaultMode, left, False)

    def drain(self):
        self.hid().CFRunLoopRunInMode(self.hid().kCFRunLoopDefaultMode, 0.05, False)
        self.reports = []

    def close(self):
        h = self.hid()
        h.IOHIDDeviceUnscheduleFromRunLoop(self.ref, h.CFRunLoopGetCurrent(), h.kCFRunLoopDefaultMode)
        h.IOHIDDeviceClose(self.ref, 0)

    def empty_hint(self):
        return ('Quit 3DxWare if it is running, or allow Terminal under System Settings > '
                'Privacy & Security > Input Monitoring, and try again.')


# --- Windows: hid.dll, the same calls hidapi's Windows backend makes ---------

class WinHid:
    """ctypes bindings to the SetupAPI/HID/kernel32 calls needed."""

    def __init__(self):
        import ctypes as c
        from ctypes import wintypes as w
        self.c, self.w = c, w
        self.hid = c.WinDLL('hid')
        self.setupapi = c.WinDLL('setupapi')
        self.k32 = c.WinDLL('kernel32', use_last_error=True)
        k, sa = self.k32, self.setupapi

        class GUID(c.Structure):
            _fields_ = [('Data1', w.DWORD), ('Data2', w.WORD), ('Data3', w.WORD),
                        ('Data4', c.c_ubyte * 8)]

        class InterfaceData(c.Structure):
            _fields_ = [('cbSize', w.DWORD), ('InterfaceClassGuid', GUID),
                        ('Flags', w.DWORD), ('Reserved', c.c_void_p)]

        class Attributes(c.Structure):
            _fields_ = [('Size', w.ULONG), ('VendorID', w.USHORT), ('ProductID', w.USHORT),
                        ('VersionNumber', w.USHORT)]

        class Caps(c.Structure):
            _fields_ = [('Usage', w.USHORT), ('UsagePage', w.USHORT),
                        ('InputReportByteLength', w.USHORT), ('OutputReportByteLength', w.USHORT),
                        ('FeatureReportByteLength', w.USHORT), ('Reserved', w.USHORT * 17),
                        ('rest', w.USHORT * 10)]

        class Overlapped(c.Structure):
            _fields_ = [('Internal', c.c_void_p), ('InternalHigh', c.c_void_p),
                        ('Offset', w.DWORD), ('OffsetHigh', w.DWORD), ('hEvent', w.HANDLE)]

        self.GUID, self.InterfaceData, self.Attributes = GUID, InterfaceData, Attributes
        self.Caps, self.Overlapped = Caps, Overlapped

        k.CreateFileW.restype = w.HANDLE
        k.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, c.c_void_p, w.DWORD, w.DWORD, w.HANDLE]
        k.CreateEventW.restype = w.HANDLE
        k.CreateEventW.argtypes = [c.c_void_p, w.BOOL, w.BOOL, w.LPCWSTR]
        k.ReadFile.argtypes = [w.HANDLE, c.c_void_p, w.DWORD, c.c_void_p, c.c_void_p]
        k.GetOverlappedResult.argtypes = [w.HANDLE, c.c_void_p, c.POINTER(w.DWORD), w.BOOL]
        k.WaitForSingleObject.argtypes = [w.HANDLE, w.DWORD]
        k.WaitForSingleObject.restype = w.DWORD
        k.CancelIo.argtypes = [w.HANDLE]
        k.CloseHandle.argtypes = [w.HANDLE]
        sa.SetupDiGetClassDevsW.restype = w.HANDLE
        sa.SetupDiGetClassDevsW.argtypes = [c.c_void_p, w.LPCWSTR, w.HWND, w.DWORD]
        sa.SetupDiEnumDeviceInterfaces.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, w.DWORD, c.c_void_p]
        sa.SetupDiGetDeviceInterfaceDetailW.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, w.DWORD,
                                                         c.POINTER(w.DWORD), c.c_void_p]
        sa.SetupDiDestroyDeviceInfoList.argtypes = [w.HANDLE]
        self.hid.HidD_GetHidGuid.argtypes = [c.c_void_p]
        self.hid.HidD_GetAttributes.argtypes = [w.HANDLE, c.c_void_p]
        self.hid.HidD_GetPreparsedData.argtypes = [w.HANDLE, c.POINTER(c.c_void_p)]
        self.hid.HidD_FreePreparsedData.argtypes = [c.c_void_p]
        self.hid.HidP_GetCaps.argtypes = [c.c_void_p, c.c_void_p]
        self.hid.HidD_GetProductString.argtypes = [w.HANDLE, c.c_void_p, w.ULONG]

    INVALID = (2 ** 64 - 1, 2 ** 32 - 1, -1)   # INVALID_HANDLE_VALUE, 64/32-bit

    def paths(self):
        """Every HID interface path on the system."""
        c, w = self.c, self.w
        guid = self.GUID()
        self.hid.HidD_GetHidGuid(c.byref(guid))
        info = self.setupapi.SetupDiGetClassDevsW(c.byref(guid), None, None, 0x12)  # PRESENT|INTERFACE
        paths = []
        i = 0
        while True:
            data = self.InterfaceData()
            data.cbSize = c.sizeof(data)
            if not self.setupapi.SetupDiEnumDeviceInterfaces(info, None, c.byref(guid), i, c.byref(data)):
                break
            i += 1
            needed = w.DWORD()
            self.setupapi.SetupDiGetDeviceInterfaceDetailW(info, c.byref(data), None, 0, c.byref(needed), None)
            buf = c.create_string_buffer(needed.value)
            # SP_DEVICE_INTERFACE_DETAIL_DATA_W: DWORD cbSize, then the path.
            c.cast(buf, c.POINTER(w.DWORD))[0] = 8 if c.sizeof(c.c_void_p) == 8 else 6
            if self.setupapi.SetupDiGetDeviceInterfaceDetailW(info, c.byref(data), buf, needed, None, None):
                paths.append(c.wstring_at(c.addressof(buf) + 4))
        self.setupapi.SetupDiDestroyDeviceInfoList(info)
        return paths

    def open(self, path, access):
        # FILE_SHARE_READ|WRITE, OPEN_EXISTING, FILE_FLAG_OVERLAPPED
        h = self.k32.CreateFileW(path, access, 3, None, 3, 0x40000000, None)
        return None if h is None or h in self.INVALID else h

    def describe(self, h):
        """(vendor, product, usage_page, usage, input_length, name) of an open handle."""
        c = self.c
        attrs = self.Attributes()
        attrs.Size = c.sizeof(attrs)
        if not self.hid.HidD_GetAttributes(h, c.byref(attrs)):
            return None
        page = usage = length = 0
        pre = c.c_void_p()
        if self.hid.HidD_GetPreparsedData(h, c.byref(pre)):
            caps = self.Caps()
            self.hid.HidP_GetCaps(pre, c.byref(caps))
            page, usage, length = caps.UsagePage, caps.Usage, caps.InputReportByteLength
            self.hid.HidD_FreePreparsedData(pre)
        name = c.create_unicode_buffer(128)
        if not self.hid.HidD_GetProductString(h, name, c.sizeof(name)):
            name.value = '?'
        return attrs.VendorID, attrs.ProductID, page, usage, length, name.value


class WinDevice:
    backend = 'windows-hid'
    _hid = None

    def __init__(self, path, name, vendor, product, length):
        self.path, self.name, self.vendor, self.product = path, name, vendor, product
        self.length = length or 64
        self.handle = None

    @classmethod
    def hid(cls):
        if cls._hid is None:
            cls._hid = WinHid()
        return cls._hid

    @classmethod
    def find(cls):
        h = cls.hid()
        found = []
        for path in h.paths():
            handle = h.open(path, 0)    # no access: enough to read attributes
            if handle is None:
                continue
            try:
                d = h.describe(handle)
            finally:
                h.k32.CloseHandle(handle)
            if not d:
                continue
            vendor, product, page, usage, length, name = d
            # The same test as QET's SpaceMouseHid::isSpaceMouse().
            if vendor not in VENDORS or (page, usage) not in ((1, 8), (0, 0)):
                continue
            found.append(cls(path, name, '%04x' % vendor, '%04x' % product, length))
        return found

    def descriptor(self):
        # Windows only gives out a parsed form of it.
        return ''

    def open(self):
        h = self.hid()
        self.handle = h.open(self.path, 0x80000000)     # GENERIC_READ
        if self.handle is None:
            sys.exit('Could not open the device (error %d).' % h.c.get_last_error())
        self.event = h.k32.CreateEventW(None, True, False, None)
        self.buf = h.c.create_string_buffer(self.length)
        self.pending = False
        self.start = time.monotonic()

    def _read(self, wait_ms):
        """One report if it arrives within wait_ms, else None."""
        h = self.hid()
        c, w = h.c, h.w
        if not self.pending:
            self.ov = h.Overlapped()
            self.ov.hEvent = self.event
            h.k32.ReadFile(self.handle, self.buf, self.length, None, c.byref(self.ov))
            self.pending = True
        if h.k32.WaitForSingleObject(self.event, max(0, int(wait_ms))) != 0:
            return None
        self.pending = False
        n = w.DWORD()
        if not h.k32.GetOverlappedResult(self.handle, c.byref(self.ov), c.byref(n), False):
            return None
        data = self.buf.raw[:n.value]
        # Windows puts a report ID in front even when the device has none;
        # hidapi drops that 0, so QET never sees it.
        return data[1:] if data[:1] == b'\0' else data

    def record(self, seconds):
        reports = []
        start = time.monotonic()
        while True:
            left = seconds - (time.monotonic() - start)
            if left <= 0:
                return reports
            data = self._read(left * 1000)
            if data:
                reports.append([round((time.monotonic() - start) * 1000, 1), data.hex()])

    def drain(self):
        while self._read(0):
            pass

    def close(self):
        h = self.hid()
        if self.pending:
            h.k32.CancelIo(self.handle)
        h.k32.CloseHandle(self.event)
        h.k32.CloseHandle(self.handle)

    def empty_hint(self):
        return 'Check the cable, push the cap firmly, and try again.'


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--list', action='store_true', help='only list matching devices')
    ap.add_argument('--device', help='device path from --list, if more than one is found')
    ap.add_argument('--descriptor', help=argparse.SUPPRESS)  # testing without a device
    ap.add_argument('--yes', action='store_true', help=argparse.SUPPRESS)  # no Enter prompts
    ap.add_argument('-o', '--output', help='output file (default: spacemouse-capture-<product>.json)')
    ap.add_argument('--seconds', type=float, default=1.0,
                    help='multiply each step\'s recording time (default: 1.0, e.g. 3 triples it)')
    args = ap.parse_args()

    Device = {'darwin': MacDevice, 'win32': WinDevice}.get(sys.platform, HidrawDevice)
    devices = Device.find()
    if args.list:
        for d in devices:
            print('%s  %s:%s  %s' % (d.path, d.vendor, d.product, d.name))
        if not devices:
            print('No 3Dconnexion device found.')
        return 0 if devices else 1

    if args.device:
        dev = next((d for d in devices if d.path == args.device), None)
        if dev is None:
            if Device is not HidrawDevice:
                sys.exit('No device %s (try --list)' % args.device)
            dev = HidrawDevice(args.device)
    elif len(devices) == 1:
        dev = devices[0]
    elif not devices:
        sys.exit('No 3Dconnexion device found. Is it plugged in? (try --list)')
    else:
        sys.exit('Several devices found, pick one with --device:\n' +
                 '\n'.join('  %s  %s' % (d.path, d.name) for d in devices))

    if args.descriptor:
        with open(args.descriptor, 'rb') as f:
            descriptor = f.read().hex()
    else:
        descriptor = dev.descriptor()

    dev.open()
    print('Recording from %s (%s, %s:%s).' % (dev.path, dev.name, dev.vendor, dev.product))
    readers = other_readers()
    if readers:
        print('Also running: %s. If nothing gets recorded, quit it and try again.' % ', '.join(readers))
    print('For each step, press Enter, do the movement, and wait for the next prompt.\n')

    result = {
        'tool': 'spacemouse-capture.py 2',
        'date': datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds'),
        'system': platform.platform(),
        'backend': dev.backend,
        'other_readers': readers,
        'device': {'name': dev.name, 'vendor': dev.vendor, 'product': dev.product},
        'report_descriptor': descriptor,
        'steps': [],
    }
    try:
        for i, (key, text, seconds) in enumerate(STEPS, 1):
            seconds = seconds * args.seconds
            print('[%d/%d] %s' % (i, len(STEPS), text))
            if not args.yes:
                input('      Press Enter to start (%.0f s)... ' % seconds)
                dev.drain()
            reports = dev.record(seconds)
            print('      %d reports recorded.\n' % len(reports))
            result['steps'].append({'step': key, 'instruction': text, 'reports': reports})
    except KeyboardInterrupt:
        print('\nStopped early -- saving what was recorded so far.')
    finally:
        dev.close()

    out = args.output or 'spacemouse-capture-%s.json' % dev.product
    with open(out, 'w') as f:
        json.dump(result, f, indent=1)
    total = sum(len(s['reports']) for s in result['steps'])
    print('Saved %s (%d reports in total).' % (out, total))
    if total == 0:
        print('Nothing was recorded. ' + dev.empty_hint())
    else:
        print('Please attach this file to discussion #599. Thank you!')
    return 0


if __name__ == '__main__':
    sys.exit(main())
