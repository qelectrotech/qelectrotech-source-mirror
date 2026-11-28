"""Drawing library of the QElectroTech importers (tools/*_import).

An importer lays out sheets on QET's grid and describes them as qet_edit
operations; Plan.generate() has QElectroTech make the project from them:

    from qet_sheet import Plan
    plan = Plan("Machine", folder)
    s = plan.sheet("Power supply", "", "", 1)
    s.rail("24V")
    ...
    plan.generate("machine.qet")
"""
from .sheet import Plan, Sheet, grid
from . import symbols

__all__ = ["Plan", "Sheet", "grid", "symbols"]
