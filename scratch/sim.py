import sys

class TextItem:
    def __init__(self, x, text, align):
        self.x = x
        self.text = text
        self.align = align
        self.width = len(text) * 10
        self.alignment_rect = [0, 0, self.width, 10]
        
    def set_text(self, new_text, ignore_alignment=False):
        old_right = self.x + self.alignment_rect[2]
        self.text = new_text
        self.width = len(new_text) * 10
        new_right = self.x + self.width
        
        if not ignore_alignment:
            if self.align == "right":
                diff = new_right - old_right
                self.x -= diff
            self.alignment_rect = [0, 0, self.width, 10]
            
    def copy(self):
        return TextItem(self.x, self.text, self.align)

print("--- Original ---")
item = TextItem(-20, "L3", "right") # user places it
item.alignment_rect = [0, 0, item.width, 10]
print(f"User Placed: text={item.text}, x={item.x}, right={item.x+item.width}")
item.set_text("L3 / 1-A15")
print(f"Linked: text={item.text}, x={item.x}, right={item.x+item.width}")

print("\n--- Paste WITHOUT patch (Double Shift) ---")
pasted1 = item.copy()
print(f"Pasted: text={pasted1.text}, x={pasted1.x}, right={pasted1.x+pasted1.width}")
pasted1.set_text("L3", ignore_alignment=True) # fromXml ignores alignment
print(f"fromXml: text={pasted1.text}, x={pasted1.x}, right={pasted1.x+pasted1.width}")
# WAIT! prepareAlignment runs BEFORE the timer updates the text!
# so alignment_rect is updated to the bounding rect of "L3"!
pasted1.alignment_rect = [0, 0, pasted1.width, 10]
pasted1.set_text("L3 / 2-A15")
print(f"Timer: text={pasted1.text}, x={pasted1.x}, right={pasted1.x+pasted1.width}")

print("\n--- Paste WITH patch (Ignore Alignment) ---")
pasted2 = item.copy()
print(f"Pasted: text={pasted2.text}, x={pasted2.x}, right={pasted2.x+pasted2.width}")
pasted2.set_text("L3", ignore_alignment=True) 
print(f"fromXml: text={pasted2.text}, x={pasted2.x}, right={pasted2.x+pasted2.width}")
# timer fires, but ignore_alignment=True
pasted2.set_text("L3 / 2-A15", ignore_alignment=True)
print(f"Timer: text={pasted2.text}, x={pasted2.x}, right={pasted2.x+pasted2.width}")
