/**
 * @file sample.cpp
 * @author Weihan.Sun (weihan.sun@sony.com)
 * @brief sample application
 * @version 2.0 
 * @date 2022-12-12
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "common/dsviewer_interface.h"
#include "common/z2color.h"
#include "upsampling/upsampling.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include <ctime>

using namespace std;

#define SHOW_TIME 
#define __TEST_GUIDE__

const string rootPath = "../../";
string strDataPath; // from argument
string strSavePath = "."; // image save path

#define FIX_COLORMAP // normalize colormap by fixed min/max values
#ifdef FIX_COLORMAP
float g_min_value = 0.0;
float g_max_value = 1.0; // 1 meter
#endif

const string strParam = rootPath + "dat/camParam/camera_calib/param_tl10.txt";

float g_FPS = 0.0;

/**
 * @brief show input depthmap overlapped with guide & upsampling results
 * 
 * @param guide : guide image
 * @param dmapFlood : flood depthmap
 * @param dmapSpot : spot depthmap
 * @param filtered_dense : dense depthmap filtered according to confidence value
 * @param mode 1: flood 2: spot 3:spot+flood
 * @return cv::Mat 
 */
cv::Mat visualization(const cv::Mat& guide, const cv::Mat& dmapFlood, const cv::Mat& dmapSpot,
                        const cv::Mat& filtered_dense, char mode)
{
    // input 
    double minVal, maxVal;
#ifdef FIX_COLORMAP
    cv::Mat colorDmapFlood = z2colormap(dmapFlood, g_min_value, g_max_value);
    cv::Mat colorDmapSpot = z2colormap(dmapSpot, g_min_value, g_max_value);
#else
    cv::minMaxLoc(dmapFlood, &minVal, &maxVal);
    cv::Mat colorDmapFlood = z2colormap(dmapFlood, minVal, maxVal);
    cv::minMaxLoc(dmapSpot, &minVal, &maxVal);
    cv::Mat colorDmapSpot = z2colormap(dmapSpot, minVal, maxVal);
#endif

    cv::Mat imgMaskFlood = cv::Mat::zeros(dmapFlood.size(), dmapFlood.type());
    cv::Mat imgMaskSpot = cv::Mat::zeros(dmapSpot.size(), dmapSpot.type());
    imgMaskFlood.setTo(1.0, dmapFlood != 0.0);
    imgMaskSpot.setTo(1.0, dmapSpot != 0.0);
    cv::Mat imgGuideMapFlood = markSparseDepth(guide, colorDmapFlood, imgMaskFlood, 3);
    cv::Mat imgGuideMapSpot = markSparseDepth(guide, colorDmapSpot, imgMaskSpot, 3);
    // filtered by conf
#ifdef FIX_COLORMAP
    minVal = g_min_value;
    maxVal = g_max_value;
#else
    cv::minMaxLoc(imgFiltered, &minVal, &maxVal);
#endif
    cv::Mat colorFiltered = z2colormap(filtered_dense, minVal, maxVal);
    cv::Mat imgOverlapFiltered = overlap(colorFiltered, guide, 0.5);
    // output
#ifndef FIX_COLORMAP
    cv::minMaxLoc(dense, &minVal, &maxVal);
#endif
    //
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    char szLabel[255];
    if (mode == '1' || mode == '3') {
        vecImgs.push_back(imgGuideMapFlood);
        sprintf_s(szLabel, "flood (FPS = %.2f)", g_FPS);
        vecLabel.push_back(szLabel);
    } else {
        vecImgs.push_back(imgGuideMapSpot);
        sprintf_s(szLabel, "spot (FPS = %.2f)", g_FPS);
        vecLabel.push_back(szLabel);
    }
    vecImgs.push_back(imgOverlapFiltered);
    vecLabel.push_back("filtered");
    return mergeImages(vecImgs, vecLabel, cv::Size(2, 1));
}

/**
 * @brief Create a new Window 
 * 
 * @param strWndName : window name 
 * @param vecLabels : label vector
 * @param vecValues : value vector
 * @param vecRanges : range vector
 */
void createWindow(const string& strWndName, const vector<string>& vecLabels, const vector<int*>& vecValues, const vector<int>& vecRanges)
{
    cv::namedWindow(strWndName);
    for (int i = 0; i < vecLabels.size(); ++i) {
        cv::createTrackbar(vecLabels[i], strWndName, vecValues[i], vecRanges[i]);     
    }
}


/**
 * @brief convert upsampling class parameters (global) to trackbar parameters (local)
 * 
 * @param upsampling_params : global parameters
 * @param preprocessing_params : global parameters
 * @param iRange_flood : local parameters
 * @param iOcc_th : local paramters
 * @param iNeigbor_th : local parameters
 * @param iFgs_lambda_flood : local parameters
 * @param iFgs_sigma_flood : local parameters
 * @param iFgs_iter_num : local parameters
 * @param iConf : local parameters
 * @param iDepth_diff_thresh : local parameters
 * @param iGuide_diff_thresh : local parameters
 * @param iMin_diff_count : local parameters
 */
inline void convert_params_global2local(const Upsampling_Params& upsampling_params, const Preprocessing_Params& preprocessing_params,
                                int& iRange_flood, int& iOcc_th, 
                                int& iNeigbor_th, int& iFgs_lambda_flood, int& iFgs_sigma_flood, int& iFgs_iter_num, int& iConf,
                                int& iDepth_diff_thresh, int& iGuide_diff_thresh, int& iMin_diff_count)
{
    iRange_flood = preprocessing_params.range_flood;
    iFgs_lambda_flood = (int)upsampling_params.fgs_lambda_flood;
    iFgs_sigma_flood = (int)upsampling_params.fgs_sigma_color_flood;
    iOcc_th= (int)(preprocessing_params.occlusion_thresh*10);
    iNeigbor_th= (int)(preprocessing_params.z_continuous_thresh*100);
    iFgs_iter_num = upsampling_params.fgs_num_iter_flood;
    iDepth_diff_thresh = (int)(preprocessing_params.depth_diff_thresh*100);
    iGuide_diff_thresh = (int)preprocessing_params.guide_diff_thresh;
    iMin_diff_count = preprocessing_params.min_diff_count;
    // * reset default parameters here
    iFgs_lambda_flood = 200;
    iFgs_sigma_flood = 6;
    iConf = 1000; 
    iDepth_diff_thresh = 6;
    iGuide_diff_thresh = 45;
    iMin_diff_count = 16;
}

/**
 * @brief convert trackbar parameters (local) to upsampling cleass parameters (global)
 * 
 * @param upsampling_params : global parameters
 * @param preprocessing_params : global parameters
 * @param iRange_flood : local parameter
 * @param iOcc_th : local parameter
 * @param iNeigbor_th : local parameter
 * @param iFgs_lambda_flood : local parameter
 * @param iFgs_sigma_flood : local parameter
 * @param iFgs_iter_num : local parameter
 * @param iDepth_diff_thresh : local parameter
 * @param iGuide_diff_thresh : local parameter
 * @param iMin_diff_count : local parameter
 */
inline void convert_params_local2global(Upsampling_Params& upsampling_params, Preprocessing_Params& preprocessing_params,
                                int iRange_flood, int iOcc_th, 
                                int iNeigbor_th, int iFgs_lambda_flood, int iFgs_sigma_flood, int iFgs_iter_num,
                                int iDepth_diff_thresh, int iGuide_diff_thresh, int iMin_diff_count)
{
    preprocessing_params.range_flood = iRange_flood;  
    preprocessing_params.z_continuous_thresh = (float)iNeigbor_th/100.f;
    preprocessing_params.occlusion_thresh = (float)iOcc_th/10.f;
    preprocessing_params.depth_diff_thresh = (float)iDepth_diff_thresh/100.f;
    preprocessing_params.guide_diff_thresh = (float)iGuide_diff_thresh;
    preprocessing_params.min_diff_count = iMin_diff_count;
    upsampling_params.fgs_lambda_flood = (float)iFgs_lambda_flood/10.f;
    upsampling_params.fgs_sigma_color_flood = (float)iFgs_sigma_flood;
    upsampling_params.fgs_num_iter_flood = iFgs_iter_num ;
}


/**
 * @brief Main function of sample code
 * 
 * @param argc : argument number (4 is required)
 * @param argv : arguments
 * @return int 
 */
int main(int argc, char* argv[])
{
    // argument check 
    if (argc != 4) {
        cout << "*** DS5 Upsampling sample application ***" << endl;
        cout << "USAGE:" << endl;
        cout << "   <exe> <input data path> <start frame ID> <end frame ID>" << endl;
        exit(0);
    }
    // assignments
    strDataPath = string(argv[1]);
    int start_frame_idx = atol(argv[2]);
    int end_frame_idx = atol(argv[3]);
    int autosave = 0;
    // read camera parameters
    map<string, float> params;
    if(!read_param(strParam, params)) {
        cout << "open param failed" <<endl;
        exit(0);
    }
    float cx, cy, fx, fy;
    get_rgb_params(params, cx, cy, fx, fy);

    // frame shift for debug
    int frameShift = 0;
    // use new depth for debug
    int use_new_depth = 0;
    // upsampling instance 
    upsampling dc;
    // set camera parameters
    Camera_Params cam_params(cx, cy, fx, fy);
    dc.set_cam_paramters(cam_params);
    // get default parameters
    Upsampling_Params upsampling_params;
    dc.get_default_upsampling_parameters(upsampling_params);
    Preprocessing_Params preprocessing_params;
    dc.get_default_preprocessing_parameters(preprocessing_params);
    Upsampling_Params default_upsampling_params = upsampling_params;
    Preprocessing_Params default_preprocessing_params = preprocessing_params;
    // convert to trackbar parameters
    int iRange_flood, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf;
    int iDepth_diff_thresh, iGuide_diff_thresh, iMin_diff_count;
    convert_params_global2local(upsampling_params, preprocessing_params, 
                            iRange_flood, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf,
                            iDepth_diff_thresh, iGuide_diff_thresh, iMin_diff_count);
    
    // GUI setting
    string strWndName = "DS5 sample";
    // paramter trackbars setting
    vector<string>vecTrackbarLabels = {"depth diff Thr", "guide diff Thr", "min diff count", "range", "occ th", "neigbor th", "fgs lambda", "fgs sigma", "iter num", "conf"};
    vector<int>vecTrackbarValueRanges = {100, 255, 24, 40, 200, 100, 1000, 20, 5, 10000};
    vector<int*>vecTrackbarValues = {&iDepth_diff_thresh, &iGuide_diff_thresh, &iMin_diff_count, &iRange_flood, &iOcc_th, &iNeigbor_th, &iFgs_lambda_flood, &iFgs_sigma_flood, &iFgs_iter_num, &iConf};
    createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
    // images
    cv::Mat dmapFlood, dmapSpot; // input
    cv::Mat dense, conf, imgShow; // output
    cv::Mat dmap_filtered, pc_filtered;
    // viewer pc
    char mode = '1'; // 1: flood 2: spot 3: flood + spot q: exit
#ifdef SHOW_TIME
    chrono::system_clock::time_point t_start, t_end; 
#endif
    int fixedFrame = 0; // 0: no fixed, 1: fixed
    int curr_frame_idx = start_frame_idx;
    cv::Mat last_guide, last_depth;
    while(1) {
        // file names
        cout << "frame = " << curr_frame_idx << " shift = " << frameShift << endl;
        if (use_new_depth != 0) {
            cout << "use new depth = " << use_new_depth << endl;
        }
        if (curr_frame_idx == 0) {
            last_guide.release();
            last_depth.release();
        }
        char szFN[255];
        sprintf_s(szFN, "%s/%08d_rgb_gray_img.png", strDataPath.c_str(), curr_frame_idx);
        string strGuide = string(szFN);
        sprintf_s(szFN, "%s/%08d_spot_depth_pc.exr", strDataPath.c_str(), curr_frame_idx + frameShift);
        string strSpotPc = string(szFN);
        sprintf_s(szFN, "%s/%08d_flood_depth_pc.exr", strDataPath.c_str(), curr_frame_idx + frameShift);
        string strFloodPc = string(szFN);
        // read dat
        cv::Mat imgGuide = cv::imread(strGuide, -1);
        cv::Mat pcFlood = cv::imread(strFloodPc, -1);
        cv::Mat pcSpot = cv::imread(strSpotPc, -1);
        // convert and set parameters
        convert_params_local2global(upsampling_params, preprocessing_params, 
                            iRange_flood, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num,
                            iDepth_diff_thresh, iGuide_diff_thresh, iMin_diff_count);
        dc.set_upsampling_parameters(upsampling_params); 
        dc.set_preprocessing_parameters(preprocessing_params);
#ifdef SHOW_TIME
        t_start = chrono::system_clock::now();
#endif 
        // upsampling processing  
        bool res = false; // upsampling success or not
        if (mode == '1') {
            // res = dc.run(imgGuide, pcFlood, cv::Mat(), dense, conf);
            res = dc.run(imgGuide, pcFlood, cv::Mat(), dense, conf);
        } else if (mode == '2') {
            res = dc.run(imgGuide, cv::Mat(), pcSpot, dense, conf);
        } else if (mode == '3') {
            // res = dc.run(imgGuide, pcFlood, pcSpot, dense, conf);
            res = dc.run(imgGuide, pcFlood, pcSpot, dense, conf);
        } else if (mode == 'q') {
            break;
        } else {
            ;
        }
        
#ifdef SHOW_TIME
        t_end = chrono::system_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
        cout << "\033[31;43mUpsampling total time = " << elapsed << " [us]\033[0m" << endl;
        g_FPS = (float)1000.f/(float)elapsed*1000.f;
#endif
        
        if(res) {
            // visualization
            dmapFlood = dc.get_flood_depthMap();
            dmapSpot = dc.get_spot_depthMap();
            dc.filter_by_confidence(dense, conf, dmap_filtered, (float)iConf/10000.0f);
            imgShow = visualization(imgGuide, dmapFlood, dmapSpot, dmap_filtered, mode);
            cv::imshow(strWndName, imgShow);
        }

        char c= cv::waitKey(30);
        switch (c)
        {
        case '1': // input flood only
            mode = '1';
            break;
        case '2': // input spot only
            mode = '2';
            break;
        case '3': // input both
            mode = '3';
            break;
        case 'q': // quit
            mode = 'q';
            break;
        case 's': // save dense and conf
            if (res) {
                sprintf_s(szFN, "%s/%08d_dense_dmap.tiff", strSavePath.c_str(), curr_frame_idx);
                cv::imwrite(szFN, dense);
                sprintf_s(szFN, "%s/%08d_conf.tiff", strSavePath.c_str(), curr_frame_idx);
                cv::imwrite(szFN, conf);
                sprintf_s(szFN, "%s/%08d_color.png", strSavePath.c_str(), curr_frame_idx);
                cv::imwrite(szFN, imgShow);
            }
            break;
        case 'r': // reset parameters to default
            convert_params_global2local(default_upsampling_params, default_preprocessing_params, 
                            iRange_flood, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf,
                            iDepth_diff_thresh, iGuide_diff_thresh, iMin_diff_count);
            // update window
            cv::destroyWindow(strWndName);
            createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
            break;
        case 'd': // disable preprocessing
            dc.use_proprocessing(!dc.use_proprocessing());
             // update window
            cv::destroyWindow(strWndName);
            createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
            break;
        case 'p': // pause
            fixedFrame = 1 - fixedFrame;
            break;
        case '.': // 1 frame forward
            curr_frame_idx += 1;
            break;
        case ',': // 1 frame backward
            curr_frame_idx -= 1;
            break;
        case '-':
            frameShift -= 1;
            break;
        case '=':
            frameShift += 1;
            break;
        default:
            break;
        }
       if (fixedFrame == 0) { // not paused
            curr_frame_idx += 1;
            imgGuide.copyTo(last_guide);
            pcFlood.copyTo(last_depth);
        }
        if(curr_frame_idx > end_frame_idx) // repeat
            curr_frame_idx = start_frame_idx;
        if(curr_frame_idx < 0) // loop
            curr_frame_idx = end_frame_idx;
        
        dmapFlood.release();
        imgGuide.release();
        dmapSpot.release();
    }
    exit(0);
}