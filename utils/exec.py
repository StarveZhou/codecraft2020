from time import time
import os

if __name__ == '__main__':
    start = time()
    os.system('./test')
    end = time()
    print("time: " + str(end - start))