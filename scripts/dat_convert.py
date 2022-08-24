""" Convert file names saved by DSViewer to input file name of sample code
    File name save by DSViewer:
        <frame ID>-<timestamp>_<file type>.<ext name>
    Input file name of sample code:
        <frame ID>_<file type>.<ext name>
"""

import os
import re
import sys
from shutil import copyfile

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("*** Convert files saved by DSViewer to Inputs of sample code ***")
        print("USAGE:")
        print("     python <script file> <path of saved files> <path of output>")
        exit(0)
    input_path = sys.argv[1] 
    output_path = sys.argv[2]

    if not os.path.exists(input_path):
        print("Cannot find path of input as ", input_path)
        exit(1)
    
    if not os.path.exists(output_path):
        os.mkdir(output_path)
    
    fn_pattern = re.compile("(\d+)-(\d+)_(\w+_\w+_\w+.\w+)")
    file_count = 0
    frame_count = {}
    for fn in os.listdir(input_path):
        res = fn_pattern.match(fn)
        if res == None:
            continue
        fn_new = f"{res[1]}_{res[3]}"
        copyfile(f"{input_path}\\{fn}", f"{output_path}\\{fn_new}")
        print(fn_new + " - converted.")
        frame_count[res[1]] = 1
        file_count += 1
    print(f"FINISHED. Total {len(frame_count)} frames({file_count} files) converted")
    