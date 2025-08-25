#!/usr/bin/env python3
"""
Compresseur RLE pour yeah.mod - Style Amiga natif
"""

def rle_compress(data):
    """Compresse des données avec RLE style Amiga"""
    compressed = bytearray()
    i = 0
    
    while i < len(data):
        current = data[i]
        
        # Chercher une séquence répétée
        run_length = 1
        while (i + run_length < len(data) and 
               data[i + run_length] == current and 
               run_length < 127):
            run_length += 1
        
        if run_length >= 3:
            # RLE: 128+count, byte
            compressed.append(128 + run_length)
            compressed.append(current)
            i += run_length
        else:
            # Littéral: collecter les bytes non-répétitifs
            literal_start = i
            literal_len = 0
            
            while i < len(data) and literal_len < 127:
                current = data[i]
                # Vérifier si une séquence répétée arrive
                if (i + 2 < len(data) and 
                    data[i + 1] == current and 
                    data[i + 2] == current):
                    break
                i += 1
                literal_len += 1
            
            # Écrire le bloc littéral
            compressed.append(literal_len - 1)  # 0-126 pour 1-127 bytes
            compressed.extend(data[literal_start:literal_start + literal_len])
    
    # End marker
    compressed.append(128)
    return bytes(compressed)

def create_rle_file(source_file, output_file):
    """Crée un fichier RLE compressé"""
    
    # Lire le fichier source
    with open(source_file, 'rb') as f:
        data = f.read()
    
    print(f"Fichier original: {len(data)} bytes")
    
    # Compresser
    compressed_data = rle_compress(data)
    
    # Header RLE (8 bytes)
    header = bytearray()
    header.extend(b'RLE1')  # Magic
    # Taille originale (big endian)
    original_size = len(data)
    header.append((original_size >> 24) & 0xFF)
    header.append((original_size >> 16) & 0xFF)
    header.append((original_size >> 8) & 0xFF)
    header.append(original_size & 0xFF)
    
    # Écrire le fichier RLE
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(compressed_data)
    
    total_size = len(header) + len(compressed_data)
    ratio = (len(data) - total_size) / len(data) * 100
    
    print(f"Fichier compressé: {total_size} bytes")
    print(f"Gain: {ratio:.1f}%")
    
    return ratio

if __name__ == "__main__":
    ratio = create_rle_file("assets/yeah.mod", "assets/yeah.mod.rle")
    if ratio > 0:
        print(f"✅ Compression RLE réussie: {ratio:.1f}% de gain")
    else:
        print("❌ Compression RLE inefficace")