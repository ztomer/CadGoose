from PIL import Image
import numpy as np

img = Image.open('test_out.png').convert('L')
arr = np.array(img)
# find the text pixels
y_indices, x_indices = np.where(arr < 128)
if len(y_indices) > 0:
    min_y, max_y = y_indices.min(), y_indices.max()
    min_x, max_x = x_indices.min(), x_indices.max()
    
    # print a small 10x10 ascii representation of the first letter
    for y in range(min_y, min_y+20):
        line = ''
        for x in range(min_x, min_x+20):
            if arr[y, x] < 128:
                line += '#'
            else:
                line += '.'
        print(line)
