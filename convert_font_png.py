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
        
        # Keep original image size - no forced resizing
        print(f"Using image size: {img.size}")
        
        # Convert to RGB to check exact color values
        if img.mode != 'RGB':
            img_rgb = img.convert('RGB')
        else:
            img_rgb = img
        
        # Get RGB pixel data
        rgb_pixels = list(img_rgb.getdata())
        
        # Simple 2-color font conversion: black=0 (transparent), non-black=1 (white)
        indexed_pixels = []
        for rgb in rgb_pixels:
            r, g, b = rgb
            # For grayscale/font images: black pixels = transparent (0), others = white (1)
            if r == 0 and g == 0 and b == 0:  # Pure black = transparent
                indexed_pixels.append(0)
            else:  # Any non-black pixel = white text
                indexed_pixels.append(1)
        
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
    input_file = "assets/16X16-F5.png"
    output_file = "assets/16X16-F5.raw"
    
    if not os.path.exists(input_file):
        print(f"Input file {input_file} not found!")
        sys.exit(1)
    
    success = convert_png_to_amiga_font(input_file, output_file)
    
    if success:
        print("Font conversion completed successfully!")
    else:
        print("Font conversion failed!")
        sys.exit(1)