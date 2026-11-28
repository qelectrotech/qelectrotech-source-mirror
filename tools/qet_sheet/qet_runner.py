"""QElectroTech run headless through the functions of the MCP server
(misc/qet-mcp/qet_mcp.py): new project, edit, layout check, PDF export.

The importers never write a .qet themselves: they describe the drawing as
qet_edit operations, and QElectroTech's scripting API makes the project.

Environment (the import dialog of QElectroTech sets the first two):
  QET_BINARY                the QElectroTech to run
  QET_COMMON_ELEMENTS_DIR,
  QET_CUSTOM_ELEMENTS_DIR,
  QET_COMPANY_ELEMENTS_DIR  the collections the elements come from
  QET_MCP                   qet_mcp.py, when it is not found next to this file
"""
import importlib.util
import json
import os
import tempfile

from . import symbols as S

HERE = os.path.dirname(os.path.abspath(__file__))


def _mcp_file():
    explicit = os.environ.get("QET_MCP", "").strip()
    candidates = [explicit] if explicit else []
    # source tree: tools/qet_sheet -> misc/qet-mcp; installation:
    # <data>/import/qet_sheet -> <data>/mcp
    candidates += [os.path.join(HERE, "..", "..", "misc", "qet-mcp", "qet_mcp.py"),
                   os.path.join(HERE, "..", "..", "mcp", "qet_mcp.py")]
    for c in candidates:
        if c and os.path.isfile(c):
            return os.path.normpath(c)
    raise FileNotFoundError("qet_mcp.py not found; set QET_MCP to misc/qet-mcp/qet_mcp.py")


_mcp = None


def mcp():
    """The qet_mcp module, loaded once."""
    global _mcp
    if _mcp is None:
        spec = importlib.util.spec_from_file_location("qet_mcp", _mcp_file())
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        _mcp = module
    return _mcp


def _prepare():
    """The importer drives the QElectroTech it was started from (or the one
    QET_BINARY names) through a script; it needs scripting for that run
    only, and the collections the operations name."""
    os.environ["QET_ENABLE_SCRIPTING"] = "1"
    folders = {}
    if S.CUSTOM:
        folders["elements_custom"] = S.CUSTOM
    if S.COMPANY:
        folders["elements_company"] = S.COMPANY
    if folders and not os.environ.get("QET_MCP_INFO_FILE"):
        # qet_mcp gives the sandboxed QElectroTech the user and company
        # collections that qet-assistant.json names: name exactly these
        fd, path = tempfile.mkstemp(prefix="qet-import-", suffix=".json")
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump({"folders": folders}, f)
        os.environ["QET_MCP_INFO_FILE"] = path


def binary():
    b = mcp().resolve_binary()
    if b is None:
        raise FileNotFoundError("no QElectroTech found: set QET_BINARY")
    return str(b)


def _elements():
    return S.COMMON or None


def project_new(output, title, folios, author="", timeout=600):
    _prepare()
    return mcp().tool_project_new(binary(), output, title=title, folios=folios, author=author,
                                  overwrite=True, elements_dir=_elements(), timeout=timeout)


def edit(project, operations, output, timeout=3000):
    _prepare()
    return mcp().tool_edit(binary(), project, operations, output, elements_dir=_elements(),
                           timeout=timeout)


def layout_check(project, limit=50, timeout=1800):
    _prepare()
    return mcp().tool_layout_check(binary(), project, limit=limit, elements_dir=_elements(),
                                   timeout=timeout)


def export_pdf(project, output, timeout=1800):
    return mcp().tool_export(binary(), project, "pdf", output, timeout=timeout)
