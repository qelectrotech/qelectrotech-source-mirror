import math

def simulate_qt_transform():
    # Parent is rotated 180
    # Text item is rotated 180 relative to parent (so it is upright in scene)
    text_rot = 180
    
    # Original state
    width1 = 15
    # User wants right edge at -12 in PARENT coords.
    # Text is rotated 180.
    # In parent coords, text starts at pos_x and goes LEFT by width.
    # So right edge in parent coords is pos_x!
    # Left edge in parent coords is pos_x - width.
    # If right edge is -12, then pos_x must be -12!
    pos_x = -12
    
    print(f"Original pos_x: {pos_x}")
    print(f"Parent right edge: {pos_x}")
    print(f"Parent left edge: {pos_x - width1}")
    
    # Now width becomes 45
    width2 = 45
    # If pos_x stays -12, right edge stays -12!
    print(f"\nIf pos_x stays {pos_x}:")
    print(f"Parent right edge: {pos_x}")
    print(f"Parent left edge: {pos_x - width2}")
    
    # Let's see what finishAlignment does!
    # m_alignment_rect is [0, 0, 15, 10] (local coords)
    x = 15 # right edge of local alignment rect
    xa = 45 # right edge of new local bounding rect
    
    # transform maps local to parent
    # rot 180 means x -> -x
    px = -x
    pax = -xa
    
    diff_x = pax - px # -45 - (-15) = -30
    
    new_pos_x = pos_x - diff_x # -12 - (-30) = 18
    
    print(f"\nAfter finishAlignment:")
    print(f"new_pos_x: {new_pos_x}")
    print(f"Parent right edge: {new_pos_x}")
    print(f"Parent left edge: {new_pos_x - width2}")

simulate_qt_transform()
