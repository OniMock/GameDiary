#!/usr/bin/env python3
import os
import sys
import re
from PIL import Image

# Constants for PSP GU format (from pspgu.h)
GU_PSM_8888 = 3

class ImageReader:
    def __init__(self, filepath):
        self.filepath = filepath
        self.width = 0
        self.height = 0
        self.pixels = [] # List of lists (rows) of uint32

    def parse(self):
        try:
            with Image.open(self.filepath) as img:
                img = img.convert("RGBA")
                self.width, self.height = img.size

                if self.width > 512 or self.height > 512:
                    return False, f"Image too large: {self.width}x{self.height} (max 512x512)"

                # Extract pixel data
                raw_data = img.getdata()
                self.pixels = []
                for y in range(self.height):
                    row = []
                    for x in range(self.width):
                        r, g, b, a = raw_data[y * self.width + x]
                        # PSP Format: 0xAABBGGRR (Little Endian MIPS)
                        val = (a << 24) | (b << 16) | (g << 8) | r
                        row.append(val)
                    self.pixels.append(row)

                return True, ""
        except Exception as e:
            return False, str(e)

class CodeGenerator:
    def __init__(self, output_dir, header_dir):
        self.output_dir = output_dir
        self.header_dir = header_dir
        self.assets = []

    def sanitize_name(self, filename):
        name = os.path.splitext(os.path.basename(filename))[0]
        ext = os.path.splitext(filename)[1][1:].upper()
        # Prefix with GD_IMG_ and convert to uppercase, replace special chars with _
        clean = re.sub(r'[^a-zA-Z0-9]', '_', name).upper()
        return f"GD_IMG_{clean}_{ext}"

    def get_pot(self, n):
        x = 1
        while x < n:
            x <<= 1
        return x

    def add_asset(self, filename, width, height, pixels):
        symbol = self.sanitize_name(filename)

        # Calculate Power of Two dimensions
        pot_w = self.get_pot(width)
        pot_height = self.get_pot(height)

        # In PSP, the texture buffer width (stride) must be a multiple of 16
        # and for POT textures, it's usually = pot_w
        stride = pot_w

        # Initialize padded data with 0 (transparent)
        # We produce pot_w * pot_height uint32 pixels
        data_size = pot_w * pot_height
        padded_pixels = [0] * data_size

        # Fill the original pixels into the top-left of the POT buffer
        for y in range(height):
            for x in range(width):
                padded_pixels[y * pot_w + x] = pixels[y][x]

        self.assets.append({
            'symbol': symbol,
            'name': os.path.basename(filename),
            'width': width,
            'height': height,
            'pot_width': pot_w,
            'pot_height': pot_height,
            'stride': stride,
            'size': data_size * 4,
            'data': padded_pixels
        })
        print(f"[OK] {os.path.basename(filename)} -> {symbol} ({width}x{height} -> POT {pot_w}x{pot_height}, stride {stride})")

    def write_header(self):
        h_path = os.path.join(self.header_dir, "image_resources.h")
        with open(h_path, 'w') as f:
            f.write("#ifndef GAMEDIARY_IMAGE_RESOURCES_H\n")
            f.write("#define GAMEDIARY_IMAGE_RESOURCES_H\n\n")
            f.write("#include <stdint.h>\n\n")
            f.write("// PSP GU format constants if not using pspgu.h\n")
            f.write("//#ifndef GU_PSM_8888\n")
            f.write("//#define GU_PSM_8888 3\n")
            f.write("//#endif\n\n")
            f.write("#include <pspgu.h>\n\n")
            f.write("typedef struct {\n")
            f.write("    uint16_t width;      // Original width\n")
            f.write("    uint16_t height;     // Original height\n")
            f.write("    uint16_t pot_width;  // Next power of two width (for GPU)\n")
            f.write("    uint16_t pot_height; // Next power of two height (for GPU)\n")
            f.write("    uint16_t stride;     // Texture buffer width (multiple of 16)\n")
            f.write("    uint16_t format;\n")
            f.write("    uint32_t size;       // Total size in bytes (pot_width * pot_height * 4)\n")
            f.write("    const uint32_t* data;\n")
            f.write("} ImageResource;\n\n")
            f.write("#define GD_IMAGE_SIZE(img) ((img)->size)\n")
            f.write("#define GD_IMAGE_BYTES(img) ((img)->size)\n\n")

            # Sort assets by symbol for consistent generation
            self.assets.sort(key=lambda x: x['symbol'])
            for asset in self.assets:
                f.write(f"extern const ImageResource {asset['symbol']};\n")

            f.write("\n#endif // GAMEDIARY_IMAGE_RESOURCES_H\n")

    def write_source(self):
        c_path = os.path.join(self.output_dir, "image_resources.c")
        with open(c_path, 'w') as f:
            f.write("#include \"app/render/image_resources.h\"\n\n")

            # Sort assets by symbol for consistent generation
            self.assets.sort(key=lambda x: x['symbol'])
            for asset in self.assets:
                f.write(f"// Data for {asset['symbol']} ({asset['name']})\n")
                f.write("__attribute__((aligned(16)))\n")
                f.write(f"static const uint32_t {asset['symbol']}_DATA[] = {{\n")

                # Format pixels in a nice grid (8 columns)
                for i, pixel in enumerate(asset['data']):
                    if i % 8 == 0:
                        f.write("    ")
                    f.write(f"0x{pixel:08X}, ")
                    if (i + 1) % 8 == 0:
                        f.write("\n")

                if len(asset['data']) % 8 != 0:
                    f.write("\n")
                f.write("};\n\n")

                f.write(f"const ImageResource {asset['symbol']} = {{\n")
                f.write(f"    .width = {asset['width']},\n")
                f.write(f"    .height = {asset['height']},\n")
                f.write(f"    .pot_width = {asset['pot_width']},\n")
                f.write(f"    .pot_height = {asset['pot_height']},\n")
                f.write(f"    .stride = {asset['stride']},\n")
                f.write(f"    .format = GU_PSM_8888,\n")
                f.write(f"    .size = {asset['size']},\n")
                f.write(f"    .data = {asset['symbol']}_DATA\n")
                f.write("};\n\n")

def main():
    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    assets_dir = os.path.join(root_dir, "assets", "image")
    output_src_dir = os.path.join(root_dir, "src", "app", "render")
    output_inc_dir = os.path.join(root_dir, "include", "app", "render")

    if not os.path.exists(assets_dir):
        print(f"Error: assets/image directory not found: {assets_dir}")
        sys.exit(1)

    gen = CodeGenerator(output_src_dir, output_inc_dir)

    any_img = False
    valid_extensions = ('.bmp', '.png', '.jpg', '.jpeg', '.tga')

    for filename in sorted(os.listdir(assets_dir)):
        if filename.lower().endswith(valid_extensions):
            any_img = True
            filepath = os.path.join(assets_dir, filename)
            reader = ImageReader(filepath)
            success, error = reader.parse()
            if success:
                gen.add_asset(filename, reader.width, reader.height, reader.pixels)
            else:
                print(f"[ERROR] {filename}: {error}")

    if any_img:
        gen.write_header()
        gen.write_source()
        print("\nResources generated successfully.")
    else:
        print("No compatible image files found to process.")

if __name__ == "__main__":
    main()
