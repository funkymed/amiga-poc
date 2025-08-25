#!/usr/bin/env python3
"""
Convert PNG font bitmap to raw Amiga format
Automatically detects image dimensions and color count
Usage: python3 convert_font_png.py <input.png> [output.raw]
"""
from PIL import Image
import sys
import os
import argparse

def analyze_image_colors(img):
    """Analyze image to detect unique colors"""
    # Convert to RGB to analyze colors
    if img.mode != 'RGB':
        img_rgb = img.convert('RGB')
    else:
        img_rgb = img
    
    # Get all unique colors
    unique_colors = set(img_rgb.getdata())
    print(f"Found {len(unique_colors)} unique colors: {list(unique_colors)}")
    
    return unique_colors, img_rgb

def convert_png_to_amiga_font(input_png, output_raw):
    """Convert PNG to raw bitmap data for Amiga"""
    try:
        # Open the PNG image
        img = Image.open(input_png)
        print(f"Original image mode: {img.mode}, size: {img.size}")
        
        # Analyze colors in the image
        unique_colors, img_rgb = analyze_image_colors(img)
        
        # Get RGB pixel data
        rgb_pixels = list(img_rgb.getdata())
        
        # Detect conversion strategy based on color count
        if len(unique_colors) == 2:
            print("Detected 2-color image (likely black/white font)")
            # Simple 2-color font conversion: black=0 (transparent), non-black=1 (white)
            indexed_pixels = []
            for rgb in rgb_pixels:
                r, g, b = rgb
                if r == 0 and g == 0 and b == 0:  # Pure black = transparent
                    indexed_pixels.append(0)
                else:  # Any non-black pixel = white text
                    indexed_pixels.append(1)
                    
        elif len(unique_colors) <= 16:
            print(f"Detected {len(unique_colors)}-color image (using palette mode)")
            # Multi-color conversion with palette
            # Convert to palette mode to get color indices
            img_palette = img.convert('P', palette=Image.ADAPTIVE, colors=min(16, len(unique_colors)))
            palette_pixels = list(img_palette.getdata())
            
            indexed_pixels = []
            for i, rgb in enumerate(rgb_pixels):
                r, g, b = rgb
                if r == 0 and g == 0 and b == 0:  # Pure black = transparent
                    indexed_pixels.append(0)
                else:
                    # Use palette index, ensuring it's within Amiga range (1-15)
                    palette_index = palette_pixels[i]
                    if palette_index == 0:  # If palette says black but RGB isn't pure black
                        palette_index = 1   # Make it color 1 instead of transparent
                    color_index = palette_index if palette_index <= 15 else (palette_index % 15) + 1
                    indexed_pixels.append(color_index)
        else:
            print(f"Warning: Image has {len(unique_colors)} colors, reducing to 16 colors")
            # Too many colors, reduce to 16
            img_palette = img.convert('P', palette=Image.ADAPTIVE, colors=16)
            palette_pixels = list(img_palette.getdata())
            
            indexed_pixels = []
            for i, rgb in enumerate(rgb_pixels):
                r, g, b = rgb
                if r == 0 and g == 0 and b == 0:  # Pure black = transparent
                    indexed_pixels.append(0)
                else:
                    palette_index = palette_pixels[i]
                    if palette_index == 0:
                        palette_index = 1
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

def main():
    parser = argparse.ArgumentParser(
        description='Convert PNG font bitmap to raw Amiga format',
        epilog='Example: python3 convert_font_png.py assets/font.png'
    )
    parser.add_argument('input_png', help='Input PNG file')
    parser.add_argument('output_raw', nargs='?', help='Output RAW file (default: input.png -> input.raw)')
    
    args = parser.parse_args()
    
    # Generate output filename if not provided
    if args.output_raw is None:
        base_name = os.path.splitext(args.input_png)[0]
        args.output_raw = f"{base_name}.raw"
    
    # Check if input file exists
    if not os.path.exists(args.input_png):
        print(f"Error: Input file '{args.input_png}' not found!")
        sys.exit(1)
    
    print(f"Converting: {args.input_png} -> {args.output_raw}")
    
    success = convert_png_to_amiga_font(args.input_png, args.output_raw)
    
    if success:
        print("Font conversion completed successfully!")
    else:
        print("Font conversion failed!")
        sys.exit(1)

if __name__ == "__main__":
    main()