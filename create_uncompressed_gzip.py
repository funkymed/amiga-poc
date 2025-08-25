#!/usr/bin/env python3
"""
Crée un fichier gzip avec blocs non compressés pour compatibilité Amiga
"""

import struct
import zlib
import sys

def create_uncompressed_gzip(input_file, output_file):
    """Crée un gzip avec blocs DEFLATE non compressés"""
    
    with open(input_file, 'rb') as f:
        data = f.read()
    
    # Header gzip
    gzip_header = struct.pack('<BBBBIBB', 
        0x1f, 0x8b,  # Magic
        8,           # Method (DEFLATE)
        0,           # Flags
        0,           # MTIME
        0,           # XFL
        255          # OS (unknown)
    )
    
    # Crée des blocs DEFLATE non compressés
    compressed_data = bytearray()
    pos = 0
    
    while pos < len(data):
        # Taille du bloc (max 65535 bytes)
        block_size = min(65535, len(data) - pos)
        is_final = (pos + block_size >= len(data))
        
        # En-tête du bloc: 1 bit final + 2 bits type (00 = non compressé)
        # Tous les bits restants à 0, alignement sur byte boundary
        block_header = 1 if is_final else 0  # Seulement bit final
        compressed_data.append(block_header)
        
        # Pour les blocs non compressés, on doit aligner sur byte boundary
        # Donc on ajoute des 0 pour compléter le byte si nécessaire
        
        # Longueur du bloc (little endian)
        compressed_data.extend(struct.pack('<H', block_size))
        # Complément à 1 de la longueur (NOT de block_size)
        compressed_data.extend(struct.pack('<H', (~block_size) & 0xFFFF))
        
        # Données du bloc
        compressed_data.extend(data[pos:pos + block_size])
        pos += block_size
    
    # CRC32 et taille originale
    crc32 = zlib.crc32(data) & 0xffffffff
    gzip_trailer = struct.pack('<II', crc32, len(data))
    
    # Écrit le fichier final
    with open(output_file, 'wb') as f:
        f.write(gzip_header)
        f.write(compressed_data)
        f.write(gzip_trailer)
    
    print(f"Créé: {output_file}")
    print(f"Original: {len(data)} bytes")
    print(f"Gzip non compressé: {len(gzip_header) + len(compressed_data) + len(gzip_trailer)} bytes")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python create_uncompressed_gzip.py <input> <output.gz>")
        sys.exit(1)
    
    create_uncompressed_gzip(sys.argv[1], sys.argv[2])