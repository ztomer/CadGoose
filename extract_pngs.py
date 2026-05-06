import sys

def extract_pngs(filename):
    with open(filename, 'rb') as f:
        data = f.read()

    magic = b'\x89PNG\r\n\x1a\n'
    idx = 0
    count = 0

    while True:
        idx = data.find(magic, idx)
        if idx == -1:
            break
        
        # Try to find the IEND chunk
        iend_magic = b'IEND\xaeB`\x82'
        iend_idx = data.find(iend_magic, idx)
        
        if iend_idx != -1:
            png_data = data[idx:iend_idx + 8]
            out_name = f'leaf_{count}.png'
            with open(out_name, 'wb') as out_f:
                out_f.write(png_data)
            print(f"Extracted {out_name}")
            count += 1
            idx = iend_idx + 8
        else:
            idx += 8

if __name__ == '__main__':
    extract_pngs(sys.argv[1])
