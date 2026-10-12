from PyQt5.QtCore import Qt, QMetaEnum

me = QMetaEnum.fromType(Qt.Alignment)
val = me.keyToValue("AlignRight")
print(f"AlignRight value: {val}")
