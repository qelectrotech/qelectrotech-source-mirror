# python script to trace the alignment
class DynamicElementTextItem:
    def __init__(self):
        self.text = ""
        self.x = 0
        self.m_block_alignment = False
        
    def setPlainText(self, text):
        old_width = len(self.text) * 10
        new_width = len(text) * 10
        diff = new_width - old_width
        
        if not self.m_block_alignment:
            self.x -= diff
            
        self.text = text
        print(f"setPlainText('{text}') -> x={self.x}")
        
    def setPos(self, x):
        self.x = x
        print(f"setPos({x}) -> x={self.x}")

print("Original:")
item = DynamicElementTextItem()
item.setPos(-67)
item.setPlainText("L1")
# user links it
item.setPlainText("2-B13 / L1")

print("\nPaste without correctTextPos:")
pasted = DynamicElementTextItem()
# fromXml
pasted.setPlainText("2-B13 / L1")
pasted.setPos(-67)

print("\nPaste with correctTextPos (old behavior):")
pasted2 = DynamicElementTextItem()
# correctTextPos saved x=-67, text="/ L1"
pasted2.setPlainText("/ L1")
pasted2.setPos(-67)
# user links it
pasted2.setPlainText("2-B13 / L1")
