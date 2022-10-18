""" Convert file names saved by DSViewer to input file name of sample code
    File name save by DSViewer:
        <frame ID>-<timestamp>_<file type>.<ext name>
    Input file name of sample code:
        <frame ID>_<file type>.<ext name>
    NOTE Rev.01 : Remove timestamp regardless of the timestamps
    NOTE Rev.02 : Ensure the same timestamp for the same frame ID
"""

import os
import re
import sys
from shutil import copyfile


flood_suffix = "flood_depth_pc.exr"
spot_suffix = "spot_depth_pc.exr"
rgb_suffix = "rgb_gray_img.png"

file_dict = {} 
num_synchronized_frames = 0
input_path = ""
output_path = ""

class FILES_PER_FRAME:
    """input files per frame 
    """
    def __init__(self) -> None:
        self.rgb_file = ""
        self.flood_file = ""
        self.spot_file = ""
        self.type_code = 0 # rgb: 0x01 | flood: 0x02 | spot: 0x04
        self.fid = -1 # frame id
    
def make_report(report_file):
    """make a report

    Args:
        report_file (str): report file 

    Returns:
        Bool: True success, False failed
    """
    global input_path, output_path
    space_fid = 8
    space_timestamp = 20 
    space_rgb_file = 15 
    space_flood_file = 15 
    space_spot_file = 15 
    try:
        with open(report_file, "w") as fs:
            strID = f"Frame ID".rjust(space_fid, ' ')
            strTime = f"TimeStamp".rjust(space_timestamp, ' ')
            strRgb = f"has RGB file".rjust(space_rgb_file, ' ')
            strFlood = f"has Flood file".rjust(space_flood_file, ' ')
            strSpot = f"has Spot file".rjust(space_spot_file, ' ')
            fs.write(f"input_path, {input_path}\n")
            fs.write(f"output_path, {output_path}\n")
            fs.write(f"{strID},{strTime},{strRgb},{strFlood},{strSpot}\n")
            for ts in file_dict:
                type_code = file_dict[ts].type_code
                strID = f"{file_dict[ts].fid}".rjust(space_fid, ' ') if file_dict[ts].fid != -1 else " "*8
                strTime = f"{ts}".rjust(space_timestamp, ' ')
                strRgb = ("yes" if type_code & 0x01 != 0 else "no").rjust(space_rgb_file, ' ')
                strFlood = ("yes" if type_code & 0x02 != 0 else "no").rjust(space_flood_file, ' ')
                strSpot = ("yes" if type_code & 0x04 != 0 else "no").rjust(space_spot_file, ' ')
                fs.write(f"{strID},{strTime},{strRgb},{strFlood},{strSpot}\n")
        return True
    except:
        return False

def create_file_dict(input_path):
    """ scan the input path and make a file dict according to files' time stamps

    Args:
        input_path (_type_): file folder 

    Returns:
        length of frames, type code
    """
    global file_dict
    try:
        if not os.path.exists(input_path):
            print("Cannot find path of input as ", input_path)
            exit(1)
        fn_pattern = re.compile("(\d+)-(\d+)_(\w+_\w+_\w+.\w+)")
        input_type_code = 0x00 
        for fn in os.listdir(input_path):
            res = fn_pattern.match(fn)
            if res == None:
                continue
            time_stamp = int(res[2])
            if not file_dict.__contains__(time_stamp):
                file_dict[time_stamp] = FILES_PER_FRAME()

            if res[3] == rgb_suffix: # rgb
                file_dict[time_stamp].rgb_file = fn
                file_dict[time_stamp].type_code |= 0x01
                input_type_code |= 0x01
                continue
            elif res[3] == flood_suffix: # flood
                file_dict[time_stamp].flood_file = fn
                file_dict[time_stamp].type_code |= 0x02
                input_type_code |= 0x02
                continue
            elif res[3] == spot_suffix: # spot
                file_dict[time_stamp].spot_file = fn
                file_dict[time_stamp].type_code |= 0x04
                input_type_code |= 0x04
                continue
        return len(file_dict), input_type_code
    except:
        return 0, 0


def realign_frames(input_type_code):
    """realign files by timestamp and assign their ids

    Args:
        input_type_code (int): type code 

    Returns:
        bool : True success
    """
    global num_synchronized_frames, file_dict
    try:
        time_stamp_order = sorted(file_dict, key=lambda x: x in file_dict.keys())
        frame_count = 0
        for ts in time_stamp_order:
            if file_dict[ts].type_code == input_type_code:
                file_dict[ts].fid = frame_count
                frame_count += 1
        num_synchronized_frames = frame_count
        return frame_count 
    except:
        return -1

def copy_files(input_path, output_path):
    """copy files with frame ids

    Args:
        input_path (str): input path 
        output_path (str): output path 

    Returns:
        _type_: _description_
    """
    global num_synchronized_frames, file_dict 
    try:
        if not os.path.exists(output_path):
            os.mkdir(output_path)
        print("start converting")
        for ts in file_dict:
            if file_dict[ts].fid == -1:
                continue
            if file_dict[ts].type_code & 0x01 != 0: # rgb
                dst_file = f"{file_dict[ts].fid}".zfill(8) + f"_{rgb_suffix}"
                copyfile(f"{input_path}/{file_dict[ts].rgb_file}", f"{output_path}/{dst_file}")
            if file_dict[ts].type_code & 0x02 != 0: # flood
                dst_file = f"{file_dict[ts].fid}".zfill(8) + f"_{flood_suffix}"
                copyfile(f"{input_path}/{file_dict[ts].flood_file}", f"{output_path}/{dst_file}")
            if file_dict[ts].type_code & 0x04 != 0: # spot 
                dst_file = f"{file_dict[ts].fid}".zfill(8) + f"_{spot_suffix}"
                copyfile(f"{input_path}/{file_dict[ts].spot_file}", f"{output_path}/{dst_file}")
        return True
    except:
        return False


def main():
    global input_path, output_path
    #* create file dict
    num_frames, type_code = create_file_dict(input_path)
    if num_frames == 0 or type_code == 0:
        print("Input Error")
        exit(1)
    #* add frame id
    res = realign_frames(type_code)
    if res == False:
        print("Re-align error")
        exit(1)
    #* copy files
    num_realigned_frames = copy_files(input_path, output_path)
    if num_realigned_frames == -1:
        print("Copy file failed")
        exit(1)
    print("Finish conversion")
    #* make report
    make_report("./report.csv")
    print("For detail refer to report.csv")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("*** Convert files saved by DSViewer to Inputs of sample code ***")
        print("USAGE:")
        print("     python <script file> <path of saved files> <path of output>")
        exit(0)
    input_path = sys.argv[1] 
    output_path = sys.argv[2]
    # main processing
    main()
    
    
    
