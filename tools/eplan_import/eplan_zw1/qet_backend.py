"""What the EPLAN import needs from outside: QElectroTech, the MCP module it is driven with, 7-Zip.

The import writes no .qet by hand. It builds a list of edit operations and lets a headless QElectroTech
carry them out through the scripting API, by way of misc/qet-mcp/qet_mcp.py (tool_project_new,
tool_edit, tool_export); what is saved is QElectroTech's own output.

Environment variables (all optional):
- QET_MCP     path of qet_mcp.py; otherwise the copy beside an installed QElectroTech (<data>/mcp/) or the
              one of the source tree (misc/qet-mcp/)
- QET_BINARY  the QElectroTech to run; otherwise the one qet_mcp finds (beside its install, on PATH)
- SEVENZIP    a 7-Zip program (7z, 7zz, 7za) for archives that come packed; QElectroTech's own import dialog
              unpacks the archive itself and passes the folder (--unpacked)
"""
import importlib.util
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

_MCP = None


def mcp_path():
    """qet_mcp.py, or None when there is none."""
    candidates = [os.environ.get("QET_MCP", "").strip(),
                  # installed: <data>/eplan_import/eplan_zw1 -> <data>/mcp
                  os.path.join(HERE, "..", "..", "mcp", "qet_mcp.py"),
                  # source tree: tools/eplan_import/eplan_zw1 -> misc/qet-mcp
                  os.path.join(HERE, "..", "..", "..", "misc", "qet-mcp", "qet_mcp.py")]
    for c in candidates:
        if c and os.path.isfile(c):
            return os.path.abspath(c)
    return None


def mcp():
    """The qet_mcp module, loaded once."""
    global _MCP
    if _MCP is None:
        path = mcp_path()
        if path is None:
            raise RuntimeError("qet_mcp.py not found: set QET_MCP to misc/qet-mcp/qet_mcp.py")
        spec = importlib.util.spec_from_file_location("qet_mcp", path)
        module = importlib.util.module_from_spec(spec)
        sys.modules.setdefault("qet_mcp", module)
        spec.loader.exec_module(module)
        _MCP = module
    return _MCP


def binary():
    """The QElectroTech to run, as a string."""
    found = mcp().resolve_binary()
    if found is None:
        raise RuntimeError("no QElectroTech found: set QET_BINARY to the qelectrotech program")
    return str(found)


def seven_zip():
    """A 7-Zip program: SEVENZIP, else 7z/7zz/7za on PATH, else the Windows default; None when none exists."""
    if os.environ.get("SEVENZIP"):
        return os.environ["SEVENZIP"]
    for name in ("7z", "7zz", "7za"):
        found = shutil.which(name)
        if found:
            return found
    for path in (r"C:\Program Files\7-Zip\7z.exe", r"C:\Program Files (x86)\7-Zip\7z.exe"):
        if os.path.isfile(path):
            return path
    return None


def user_collection():
    """The user collection ("custom://") the import writes its parts into: QET_CUSTOM_ELEMENTS_DIR (set by
    QElectroTech's import dialog), else the one qet-assistant.json names, else None."""
    env = os.environ.get("QET_CUSTOM_ELEMENTS_DIR", "").strip()
    if env:
        return os.path.abspath(env)
    try:
        found = mcp()._user_collections().get("custom")
    except Exception:          # no qet_mcp or no readable qet-assistant.json: the caller decides
        found = None
    return str(found) if found else None
