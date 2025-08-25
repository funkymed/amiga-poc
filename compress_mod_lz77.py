#!/usr/bin/env python3
"""
Compresseur LZ77 pour fichiers MOD Amiga
Compatible avec l'implémentation C Amiga
"""

import os
import sys

def find_longest_match(data, pos, window_size=4096, min_match=3, max_match=18):
    """Trouve la plus longue correspondance dans la fenêtre de recherche"""
    best_length = 0
    best_distance = 0
    start = max(0, pos - window_size)
    
    for i in range(start, pos):
        length = 0
        while (length < max_match and 
               pos + length < len(data) and 
               data[i + length] == data[pos + length]):
            length += 1
        
        if length >= min_match and length > best_length:
            best_length = length
            best_distance = pos - i
    
    return best_distance, best_length

def lz77_compress(input_data):
    """Compresse les données avec l'algorithme LZ77"""
    compressed = bytearray()
    pos = 0
    
    # Header LZ77
    compressed.extend(b'LZ77')
    
    # Taille originale (big endian)
    original_size = len(input_data)
    compressed.extend(original_size.to_bytes(4, 'big'))
    
    # Espace réservé pour la taille compressée
    size_pos = len(compressed)
    compressed.extend(b'\x00\x00\x00\x00')
    
    data_start = len(compressed)
    
    while pos < len(input_data):
        distance, length = find_longest_match(input_data, pos)
        
        if length >= 3:  # Référence arrière
            # Format: 1 bit (1) + 12 bits distance + 4 bits (longueur-3)
            reference = 0x8000 | ((distance & 0xFFF) << 4) | ((length - 3) & 0xF)
            compressed.extend(reference.to_bytes(2, 'big'))
            pos += length
        else:  # Littéral
            # Format: 1 bit (0) + 7 bits valeur
            compressed.append(input_data[pos] & 0x7F)
            pos += 1
    
    # Met à jour la taille compressée dans l'header
    compressed_size = len(compressed) - 12  # Sans l'header
    compressed[size_pos:size_pos+4] = compressed_size.to_bytes(4, 'big')
    
    return bytes(compressed)

def lz77_decompress(compressed_data):
    """Décompresse les données LZ77"""
    if len(compressed_data) < 12:
        raise ValueError("Données trop courtes")
    
    # Vérifie l'header
    if compressed_data[:4] != b'LZ77':
        raise ValueError("Header LZ77 invalide")
    
    # Lit les tailles
    original_size = int.from_bytes(compressed_data[4:8], 'big')
    compressed_size = int.from_bytes(compressed_data[8:12], 'big')
    
    decompressed = bytearray()
    pos = 12  # Début des données
    
    while pos < len(compressed_data) and len(decompressed) < original_size:
        if pos >= len(compressed_data):
            break
            
        first_byte = compressed_data[pos]
        pos += 1
        
        if first_byte & 0x80:  # Référence arrière (2 bytes)
            if pos >= len(compressed_data):
                break
            second_byte = compressed_data[pos]
            pos += 1
            
            reference = (first_byte << 8) | second_byte
            distance = (reference >> 4) & 0xFFF
            length = (reference & 0xF) + 3
            
            if distance == 0 or len(decompressed) < distance:
                break
            
            # Copie depuis la position précédente
            for _ in range(length):
                if len(decompressed) < original_size:
                    decompressed.append(decompressed[-distance])
                else:
                    break
        else:  # Littéral
            if len(decompressed) < original_size:
                decompressed.append(first_byte)
    
    return bytes(decompressed)

def compress_file(input_filename, output_filename):
    """Compresse un fichier"""
    with open(input_filename, 'rb') as f:
        data = f.read()
    
    compressed = lz77_compress(data)
    
    with open(output_filename, 'wb') as f:
        f.write(compressed)
    
    original_size = len(data)
    compressed_size = len(compressed)
    ratio = (1 - compressed_size / original_size) * 100
    
    print(f"Fichier original: {original_size:,} bytes")
    print(f"Fichier compressé: {compressed_size:,} bytes")
    print(f"Compression: {ratio:.1f}%")
    
    return compressed_size

def test_compression():
    """Test de compression/décompression"""
    test_data = b"AAABBBCCCDDDEEEFFFGGGHHHIIIJJJKKKLLLMMMNNNOOOPPPQQQRRRSSSTTTUUUVVVWWWXXXYYYZZZ" * 100
    
    compressed = lz77_compress(test_data)
    decompressed = lz77_decompress(compressed)
    
    print(f"Test - Original: {len(test_data)} bytes")
    print(f"Test - Compressé: {len(compressed)} bytes")
    print(f"Test - Ratio: {(1 - len(compressed)/len(test_data))*100:.1f}%")
    print(f"Test - Décompression OK: {test_data == decompressed}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compress_mod_lz77.py <input.mod> <output.mod.lz77>")
        print("Test compression:")
        test_compression()
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    if not os.path.exists(input_file):
        print(f"Erreur: fichier {input_file} introuvable")
        sys.exit(1)
    
    try:
        compress_file(input_file, output_file)
        print(f"Compression réussie: {output_file}")
    except Exception as e:
        print(f"Erreur: {e}")
        sys.exit(1)