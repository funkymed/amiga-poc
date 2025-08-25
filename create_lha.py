#!/usr/bin/env python3
"""
Créateur de fichier LHA simple pour yeah.mod
Utilise un format LHA basique avec compression store (pas de compression)
"""

import struct
import os

def create_simple_lha(source_file, output_file):
    """Crée un fichier LHA simple (store, pas de compression)"""
    
    # Lire le fichier source
    with open(source_file, 'rb') as f:
        data = f.read()
    
    filename = os.path.basename(source_file)
    
    # Header LHA basique (format -lh0- = store)
    header_size = 22
    method = b'-lh0-'  # Store (pas de compression)
    packed_size = len(data)
    original_size = len(data)
    filename_len = len(filename)
    
    # Construire le header
    header = bytearray()
    header.append(header_size)  # Header size
    header.append(0)           # Header checksum (simplifiée)
    header.extend(method)      # Method
    header.extend(struct.pack('<L', packed_size))    # Packed size (little endian)
    header.extend(struct.pack('<L', original_size))  # Original size
    header.extend(struct.pack('<H', 0))              # Time (simplifié)
    header.extend(struct.pack('<H', 0))              # Date (simplifié)  
    header.extend(struct.pack('<H', 0))              # CRC (simplifié)
    header.append(0x20)        # OS ID (generic)
    header.append(filename_len) # Filename length
    header.extend(filename.encode('ascii'))  # Filename
    
    # Calculer le checksum simple
    checksum = sum(header[2:]) & 0xFF
    header[1] = checksum
    
    # Écrire le fichier LHA
    with open(output_file, 'wb') as f:
        f.write(header)
        f.write(data)  # Données non compressées (-lh0-)
    
    print(f"Fichier LHA créé: {output_file}")
    print(f"Taille originale: {original_size} bytes")
    print(f"Taille LHA: {len(header) + len(data)} bytes")

if __name__ == "__main__":
    create_simple_lha("assets/yeah.mod", "assets/yeah.mod.lzh")