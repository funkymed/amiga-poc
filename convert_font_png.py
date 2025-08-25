#!/usr/bin/env python3
"""
Convert PNG font bitmap to raw Amiga format
"""
from PIL import Image
import sys
import os

def convert_png_to_amiga_font(input_png, output_raw):
    """Convert PNG to raw bitmap data for Amiga"""
    try:
        # Open the PNG image
        img = Image.open(input_png)
        print(f"Original image mode: {img.mode}, size: {img.size}")
        
        # Ensure the image is 320x200 as expected
        if img.size != (320, 200):
            print(f"Resizing image from {img.size} to (320, 200)")
            img = img.resize((320, 200), Image.NEAREST)
        
        # Convert to RGB to check exact color values
        if img.mode != 'RGB':
            img_rgb = img.convert('RGB')
        else:
            img_rgb = img
        
        # Get RGB pixel data
        rgb_pixels = list(img_rgb.getdata())
        
        # Also get palette data if available for color mapping
        if img.mode == 'P':
            print("Image is in palette mode, preserving original colors")
            palette_pixels = list(img.getdata())
        else:
            # Convert to palette mode to get color indices
            img_palette = img.convert('P', palette=Image.ADAPTIVE, colors=8)
            palette_pixels = list(img_palette.getdata())
        
        # Map colors: only pure black RGB(0,0,0) becomes transparent
        indexed_pixels = []
        for i, rgb in enumerate(rgb_pixels):
            r, g, b = rgb
            if r == 0 and g == 0 and b == 0:  # Pure black RGB(0,0,0) = transparent
                indexed_pixels.append(0)   # Transparent
            else:
                # Use palette index, ensuring it's within Amiga range (1-15)
                palette_index = palette_pixels[i]
                if palette_index == 0:  # If palette says black but RGB isn't pure black
                    palette_index = 1   # Make it color 1 instead of transparent
                color_index = palette_index if palette_index <= 15 else (palette_index % 15) + 1
                indexed_pixels.append(color_index)
        
        # Write indexed pixel data
        with open(output_raw, 'wb') as f:
            for pixel in indexed_pixels:
                f.write(bytes([pixel]))
        
        print(f"Converted {input_png} to {output_raw}")
        print(f"Image size: {img.size}")
        print(f"Data size: {len(indexed_pixels)} bytes")
        print(f"Color distribution:")
        for i in range(16):
            count = indexed_pixels.count(i)
            if count > 0:
                print(f"  Color {i}: {count} pixels")
        
        return True
        
    except Exception as e:
        print(f"Error converting {input_png}: {e}")
        return False

if __name__ == "__main__":
    input_file = "assets/font1.png"
    output_file = "assets/font1.raw"
    
    if not os.path.exists(input_file):
        print(f"Input file {input_file} not found!")
        sys.exit(1)
    
    success = convert_png_to_amiga_font(input_file, output_file)
    
    if success:
        print("Font conversion completed successfully!")
    else:
        print("Font conversion failed!")
        sys.exit(1)