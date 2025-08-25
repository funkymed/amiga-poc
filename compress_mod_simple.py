#!/usr/bin/env python3
"""
Compresseur LZ77 simplifié pour MOD Amiga - compatible C
"""

import os
import sys

def lz77_compress_simple(input_data):
    """Compression LZ77 simplifiée - compatible avec le décompresseur C"""
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
    
    while pos < len(input_data):
        best_length = 0
        best_distance = 0
        
        # Recherche dans une fenêtre de 4096 bytes
        start = max(0, pos - 4096)
        for i in range(start, pos):
            length = 0
            # Trouve la longueur de correspondance maximale
            while (length < 18 and 
                   pos + length < len(input_data) and
                   input_data[i + length] == input_data[pos + length]):
                length += 1
            
            if length >= 3 and length > best_length:
                best_length = length
                best_distance = pos - i
        
        if best_length >= 3:
            # Encode une référence: 2 bytes
            # 1er byte: 0x80 | ((distance >> 4) & 0x7F)
            # 2nd byte: ((distance & 0xF) << 4) | ((length - 3) & 0xF)
            byte1 = 0x80 | ((best_distance >> 4) & 0x7F)
            byte2 = ((best_distance & 0xF) << 4) | ((best_length - 3) & 0xF)
            compressed.append(byte1)
            compressed.append(byte2)
            pos += best_length
        else:
            # Encode un littéral: 1 byte (bit 7 = 0)
            compressed.append(input_data[pos] & 0x7F)
            pos += 1
    
    # Met à jour la taille compressée
    compressed_size = len(compressed) - 12
    compressed[size_pos:size_pos+4] = compressed_size.to_bytes(4, 'big')
    
    return bytes(compressed)

def lz77_decompress_simple(compressed_data):
    """Décompression LZ77 simplifiée"""
    if len(compressed_data) < 12:
        raise ValueError("Données trop courtes")
    
    # Vérifie l'header
    if compressed_data[:4] != b'LZ77':
        raise ValueError("Header LZ77 invalide")
    
    # Lit les tailles
    original_size = int.from_bytes(compressed_data[4:8], 'big')
    
    decompressed = bytearray()
    pos = 12  # Début des données
    
    while pos < len(compressed_data) and len(decompressed) < original_size:
        if pos >= len(compressed_data):
            break
            
        first_byte = compressed_data[pos]
        pos += 1
        
        if first_byte & 0x80:  # Référence arrière
            if pos >= len(compressed_data):
                break
            second_byte = compressed_data[pos]
            pos += 1
            
            # Décode la référence
            distance = ((first_byte & 0x7F) << 4) | ((second_byte >> 4) & 0xF)
            length = (second_byte & 0xF) + 3
            
            if distance == 0 or len(decompressed) < distance:
                break
            
            # Copie les données depuis la position précédente
            for i in range(length):
                if len(decompressed) < original_size:
                    decompressed.append(decompressed[-distance])
                else:
                    break
        else:
            # Littéral
            if len(decompressed) < original_size:
                decompressed.append(first_byte)
    
    return bytes(decompressed)

def compress_file(input_filename, output_filename):
    """Compresse un fichier"""
    with open(input_filename, 'rb') as f:
        data = f.read()
    
    compressed = lz77_compress_simple(data)
    
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
    test_data = b"HELLO WORLD! " * 1000 + b"TEST DATA " * 500
    
    compressed = lz77_compress_simple(test_data)
    decompressed = lz77_decompress_simple(compressed)
    
    print(f"Test - Original: {len(test_data)} bytes")
    print(f"Test - Compressé: {len(compressed)} bytes")
    print(f"Test - Ratio: {(1 - len(compressed)/len(test_data))*100:.1f}%")
    print(f"Test - Décompression OK: {test_data == decompressed}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python compress_mod_simple.py <input.mod> <output.mod.lz77>")
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