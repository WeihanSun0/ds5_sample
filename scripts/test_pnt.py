from tkinter import N
from venv import create
from xmlrpc.client import MININT
import cv2 as cv
import numpy as np
import math


# flood_suffix = "flood_depth_pc.exr"
flood_suffix = "flood.csv"
spot_suffix = "spot_depth_pc.exr"
rgb_suffix = "rgb_gray_img.png"

data_path = "D:\\workspace2\\Upsampling\\DS5\\FB\\ds5_release\\dat\\test"

frame_id = "00000044" 
flood_file = f"{data_path}\\{frame_id}_{flood_suffix}"
spot_file = f"{data_path}\\{frame_id}_{spot_suffix}"
rgb_file = f"{data_path}\\{frame_id}_{rgb_suffix}"

param_file = "D:\\workspace2\\Upsampling\\DS5\\FB\\data\\20221019\\param.txt"

def load_params(filename):
    param = {}
    with open(filename, 'r') as f:
        strline = f.readline()
        while(strline != ''):
            strList = strline.split('\t')
            param[strList[0]] = float(strList[1].lstrip())
            strline = f.readline()
    return param

def pc2map(pc, params, img):
    fx = params["RGB_FOCAL_LENGTH_X"]
    fy = params["RGB_FOCAL_LENGTH_Y"]
    cx = params["RGB_CX"]
    cy = params["RGB_CY"]
    length, _ = pc.shape
    height, width = img.shape
    mat = np.zeros((height, width, 3), np.uint8)
    colorImg = cv.cvtColor(img, cv.COLOR_GRAY2BGR)
    minX, minY, maxX, maxY = width, height, 0, 0
    minR, maxR, minC, maxC = 0,0,0,0
    countX, countY = 0, 0
    prePnt = (0,0)
    for r in range(60):
        for c in range(80):
            i = 80 * r + c
            x, y, z = pc[i, :]
            if math.isnan(x):
                continue
            u = round(x * fx / z + cx)
            v = round(y * fy / z + cy)
            if v < 0 or u < 0 or v >= height or u >= width: 
                continue
            # mat[v, u, :] = (255, 255, 255)
            curPnt = (u, v)
            cv.line(colorImg, prePnt, curPnt, (0, 0, 255), 1)
            cv.circle(colorImg, curPnt, 3, (0, 255, 0), -1)
            prePnt = curPnt
            # (v-10)*12
            if minX > u: 
                minX = u
                minC = c
            if maxX < u:
                maxX = u
                maxC = c
            if minY > v:
                minY = v
                minR = r
            if maxY < v:
                maxY = v
                maxR = r
    print(f"minC:{minC}, maxC:{maxC}, minX:{minX}, maxX:{maxX}")
    print(f"minR:{minR}, maxR:{maxR}, minY:{minY}, maxY:{maxY}")
    return colorImg 

def createMatrix(pc, params, img):
    fx = params["RGB_FOCAL_LENGTH_X"]
    fy = params["RGB_FOCAL_LENGTH_Y"]
    cx = params["RGB_CX"]
    cy = params["RGB_CY"]
    length, _ = pc.shape
    height, width = img.shape
    mat = np.zeros((height, width, 3), np.uint8)
    colorImg = cv.cvtColor(img, cv.COLOR_GRAY2BGR)
    for r in range(60):
        for c in range(80):
            # estimate pnt
            u_est = 20 + 12 * c
            v_est =  12 * (r - 7)
            flag = 0
            if v_est < 0:
                v_est = 0
            # calc real pnt
            i = 80 * r + c
            x, y, z = pc[i, :]
            if math.isnan(x):
                u = u_est
                v = v_est
            else:
                u = round(x * fx / z + cx)
                v = round(y * fy / z + cy)
                flag = 1
                if v < 0 or u < 0 or v >= height or u >= width: 
                    u = u_est
                    v = v_est
                    flag = 2
            if flag == 0:
                cv.circle(colorImg, (u, v), 3, (0, 255, 0), -1) 
            elif flag ==  1: 
                cv.circle(colorImg, (u, v), 3, (0, 0, 255), -1) 
            else:
                cv.circle(colorImg, (u, v), 3, (255, 0, 0), -1) 
                
    return colorImg


if __name__ == '__main__':
    flood_pc = np.loadtxt(flood_file, delimiter=',') 
    rgb_img = cv.imread(rgb_file, -1)
    params = load_params(param_file)
    # mat = pc2map(flood_pc, params, rgb_img)
    mat = createMatrix(flood_pc, params, rgb_img)
    cv.imshow('mat', mat)
    cv.waitKey(0)
