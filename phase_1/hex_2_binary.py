def hex_to_binary(input_file, output_file, width=96, height=96):
    """
    Convert hex text file from ESP32 camera output to binary RGB565 file
    
    Args:
        input_file: Path to the text file containing hex data
        output_file: Path to output binary file
        width: Image width in pixels
        height: Image height in pixels
    """
    
    try:
        with open(input_file, 'r') as f:
            content = f.read()
        
        print(f"Reading from: {input_file}")
        print(f"File size: {len(content)} characters")
        
        # Method 1: Extract between DATA_START and DATA_END markers
        start_marker = "DATA_START"
        end_marker = "DATA_END"
        
        start_idx = content.find(start_marker)
        end_idx = content.find(end_marker)
        
        if start_idx != -1 and end_idx != -1:
            print("Found DATA_START and DATA_END markers")
            hex_data = content[start_idx + len(start_marker):end_idx]
        else:
            print("Markers not found, processing entire file")
            hex_data = content
        
        # Clean the hex data - remove everything except hex characters
        hex_clean = ""
        for char in hex_data:
            if char in '0123456789ABCDEFabcdef':
                hex_clean += char
        
        print(f"Extracted hex characters: {len(hex_clean)}")
        print(f"First 32 hex chars: {hex_clean[:32]}")
        
        # Check if we have even number of hex characters
        if len(hex_clean) % 2 != 0:
            print("Warning: Odd number of hex characters, removing last character")
            hex_clean = hex_clean[:-1]
        
        # Convert hex string to binary data
        try:
            binary_data = bytes.fromhex(hex_clean)
        except ValueError as e:
            print(f"Error converting hex to binary: {e}")
            return False
        
        # Write binary data to file
        with open(output_file, 'wb') as f:
            f.write(binary_data)
        
        # Verify the conversion
        expected_bytes = width * height * 2  # 2 bytes per pixel for RGB565
        actual_bytes = len(binary_data)
        
        print(f"\nConversion Results:")
        print(f"Expected bytes: {expected_bytes}")
        print(f"Actual bytes: {actual_bytes}")
        print(f"Image dimensions: {width}x{height}")
        print(f"Output file: {output_file}")
        
        if actual_bytes == expected_bytes:
            print("✓ Perfect match! Conversion successful.")
        else:
            print(f"⚠ Size mismatch. Difference: {actual_bytes - expected_bytes} bytes")
            if actual_bytes > expected_bytes:
                print("File is larger than expected - might contain extra data")
            else:
                print("File is smaller than expected - might be incomplete")
        
        return True
        
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def analyze_hex_file(input_file):
    """
    Analyze the hex file to understand its structure
    """
    try:
        with open(input_file, 'r') as f:
            content = f.read()
        
        print("=== File Analysis ===")
        print(f"Total characters: {len(content)}")
        
        # Count hex characters
        hex_chars = sum(1 for c in content if c in '0123456789ABCDEFabcdef')
        print(f"Hex characters: {hex_chars}")
        print(f"Potential bytes: {hex_chars // 2}")
        
        # Look for markers
        if "DATA_START" in content:
            print("✓ Found DATA_START marker")
        if "DATA_END" in content:
            print("✓ Found DATA_END marker")
        
        # Show first few lines
        lines = content.split('\n')[:10]
        print("\nFirst 10 lines:")
        for i, line in enumerate(lines):
            print(f"{i+1:2d}: {line[:80]}{'...' if len(line) > 80 else ''}")
        
    except Exception as e:
        print(f"Error analyzing file: {e}")

# Main usage
if __name__ == "__main__":
    input_file = "raw_rgb565.txt"
    output_file = "image.rgb565"
    
    # Image dimensions (change these to match your camera settings)
    width = 96
    height = 96
    
    print("RGB565 Hex to Binary Converter")
    print("=" * 40)
    
    # First analyze the file
    analyze_hex_file(input_file)
    
    print("\n" + "=" * 40)
    
    # Convert to binary
    success = hex_to_binary(input_file, output_file, width, height)
    
    if success:
        print(f"\n✓ Conversion completed successfully!")
        print(f"You can now open '{output_file}' in your RGB565 raw image viewer")
        print(f"Settings: {width}x{height}, RGB565 format, Little Endian")
    else:
        print("\n✗ Conversion failed!")
