#pragma once
#include<iostream>
#include<fstream>
#include<string>
#include<map>
#include<opencv2/opencv.hpp>
/**
 * @brief Get the parameters of RGB camera
 * 
 * @param params : recived by call read_param function 
 * @param cx 
 * @param cy 
 * @param fx 
 * @param fy 
 * @return true : succeed
 * @return false : failed 
 */
bool get_rgb_params(std::map<std::string, float>& params, float& cx, float& cy, float& fx, float& fy)
{
    cx = params[std::string("RGB_CX")];
    cy = params[std::string("RGB_CY")];
    fx = params[std::string("RGB_FOCAL_LENGTH_X")];
    fy = params[std::string("RGB_FOCAL_LENGTH_Y")];
    if (cx == 0.0 || cy == 0.0 || fx == 0.0 || fy == 0.0) 
        return false;
    return true;
}

/**
 * @brief Read parameters from params.txt (viewer's paramter file)
 * 
 * @param strFile : parameter file name 
 * @param params : return parameters 
 * @return true : succeed 
 * @return false : failed
 */
bool read_param(const std::string& strFile, std::map<std::string, float>& params)
{
    std::fstream fs;
    fs.open(strFile.c_str(), std::ios::in);
    if (!fs.is_open())
        return false;
    std::string strline, strKey, strVal;
    
    while(std::getline(fs, strline)) {
        strline.erase(0, strline.find_first_not_of(" "));
        if (strcmp(strline.substr(0, 2).c_str(), "//") && !strline.empty()) {
            size_t pos = strline.find_first_of("\t");
            if (pos == -1)
                pos = strline.find_first_of(" ");
            strKey = strline.substr(0, pos);
            strVal = strline.substr(pos+1, strline.length()-pos-1); 
            strVal.erase(0, strVal.find_first_not_of("\t"));
            params[strKey] = (float)atof(strVal.c_str());
        }
    }
    return true;
}