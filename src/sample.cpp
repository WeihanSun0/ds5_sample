/**
 * @file sample.cpp
 * @author Weihan.Sun (weihan.sun@sony.com)
 * @brief sample application
 * @version 0.1
 * @date 2022-07-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "dsviewer_interface.h"
#include "z2color.h"
#include "upsampling/upsampling.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
using namespace std;

#define SHOW_TIME 

const string rootPath = "../../";
const string strDataPath = rootPath + "dat/master_30cm_blackBG";
const string strParam = rootPath + "dat/camParam/camera_calib/param.txt";

float g_FPS = 0.0;

cv::Mat visualization(const cv::Mat& guide, const cv::Mat& dmapFlood, const cv::Mat& dmapSpot,
                        const cv::Mat& dense, const cv::Mat& conf, float conf_thresh, char mode)
{
    // input 
    double minVal, maxVal;
    cv::minMaxLoc(dmapFlood, &minVal, &maxVal);
    cv::Mat colorDmapFlood = z2colormap(dmapFlood, minVal, maxVal);
    cv::minMaxLoc(dmapSpot, &minVal, &maxVal);
    cv::Mat colorDmapSpot = z2colormap(dmapSpot, minVal, maxVal);

    cv::Mat imgMaskFlood = cv::Mat::zeros(dmapFlood.size(), dmapFlood.type());
    cv::Mat imgMaskSpot = cv::Mat::zeros(dmapSpot.size(), dmapSpot.type());
    imgMaskFlood.setTo(1.0, dmapFlood != 0.0);
    imgMaskSpot.setTo(1.0, dmapSpot != 0.0);
    cv::Mat imgGuideMapFlood = markSparseDepth(guide, colorDmapFlood, imgMaskFlood, 3);
    cv::Mat imgGuideMapSpot = markSparseDepth(guide, colorDmapSpot, imgMaskSpot, 3);
    // filtered by conf
    cv::Mat imgFiltered;
    dense.copyTo(imgFiltered);
    imgFiltered.setTo(std::nan(""), conf < conf_thresh);
    cv::minMaxLoc(imgFiltered, &minVal, &maxVal);
    cv::Mat colorFiltered = z2colormap(imgFiltered, minVal, maxVal);
    cv::Mat imgOverlapFiltered = overlap(colorFiltered, guide, 0.5);
    // output
    cv::minMaxLoc(dense, &minVal, &maxVal);
    cv::Mat colorDense = z2colormap(dense, minVal, maxVal);
    cv::Mat imgOverlapDense = overlap(colorDense, guide, 0.5);
        //
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    char szLabel[255];
    if (mode == '1' || mode == '3') {
        vecImgs.push_back(imgGuideMapFlood);
        sprintf(szLabel, "flood (FPS = %.2f)", g_FPS);
        vecLabel.push_back(szLabel);
    } else {
        vecImgs.push_back(imgGuideMapSpot);
        sprintf(szLabel, "spot (FPS = %.2f)", g_FPS);
        vecLabel.push_back(szLabel);
    }
    vecImgs.push_back(imgOverlapDense);
    vecLabel.push_back("dense");
    vecImgs.push_back(imgOverlapFiltered);
    vecLabel.push_back("filtered");
    return mergeImages(vecImgs, vecLabel, cv::Size(3, 1));
}

/**
 * @brief Create a new Window 
 * 
 * @param strWndName 
 * @param vecLabels 
 * @param vecValues 
 * @param vecRanges 
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
 * @param upsampling_params : global params 
 * @param preprocessing_params : global params 
 * @param iRange_flood 
 * @param iDilate_size 
 * @param iCanny_th1 
 * @param iCanny_th2 
 * @param iOcc_th 
 * @param iNeigbor_th 
 * @param iFgs_lambda_flood 
 * @param iFgs_sigma_flood 
 * @param iFgs_iter_num 
 * @param iConf 
 */
inline void convert_params_global2local(const Upsampling_Params& upsampling_params, const Preprocessing_Params& preprocessing_params,
                                int& iRange_flood, int& iDilate_size, int& iCanny_th1, int& iCanny_th2, int& iOcc_th, 
                                int& iNeigbor_th, int& iFgs_lambda_flood, int& iFgs_sigma_flood, int& iFgs_iter_num, int& iConf)
{
    iRange_flood = preprocessing_params.range_flood;
    iDilate_size = preprocessing_params.guide_edge_dilate_size;
    iCanny_th1 = preprocessing_params.canny_thresh1;
    iCanny_th2 = preprocessing_params.canny_thresh2;
    iFgs_lambda_flood = (int)upsampling_params.fgs_lambda_flood;
    iFgs_sigma_flood = (int)upsampling_params.fgs_sigma_color_flood;
    iOcc_th= (int)(preprocessing_params.occlusion_thresh*10);
    iNeigbor_th= (int)(preprocessing_params.z_continuous_thresh*100);
    iFgs_iter_num = upsampling_params.fgs_num_iter_flood;
    iConf = 5; 
}

inline void convert_params_local2global(Upsampling_Params& upsampling_params, Preprocessing_Params& preprocessing_params,
                                int iRange_flood, int iDilate_size, int iCanny_th1, int iCanny_th2, int iOcc_th, 
                                int iNeigbor_th, int iFgs_lambda_flood, int iFgs_sigma_flood, int iFgs_iter_num)
{
    preprocessing_params.range_flood = iRange_flood;  
    preprocessing_params.guide_edge_dilate_size = iDilate_size; 
    preprocessing_params.canny_thresh1 = iCanny_th1;
    preprocessing_params.canny_thresh2 = iCanny_th2;
    preprocessing_params.z_continuous_thresh = (float)iNeigbor_th/100.f;
    preprocessing_params.occlusion_thresh = (float)iOcc_th/10.f;
    upsampling_params.fgs_lambda_flood = (float)iFgs_lambda_flood/10.f;
    upsampling_params.fgs_sigma_color_flood = (float)iFgs_sigma_flood;
    upsampling_params.fgs_num_iter_flood = iFgs_iter_num ;
}

int main(int argc, char* argv[])
{
    // read camera parameters
    map<string, float> params;
    if(!read_param(strParam, params)) {
        cout << "open param failed" <<endl;
        exit(0);
    }
    float cx, cy, fx, fy;
    get_rgb_params(params, cx, cy, fx, fy);

    // upsampling
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
    int iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf;
    convert_params_global2local(upsampling_params, preprocessing_params, 
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf);
    
    string strWndName = "DS5 sample";
    vector<string>vecTrackbarLabels = {"range", "dilate size", "canny th1", "canny th2", "occ th", "neigbor th", "fgs lambda", "fgs sigma", "iter num", "conf"};
    vector<int>vecTrackbarValueRanges = {50, 20, 255, 255, 200, 100, 1000, 20, 5, 10000};
    vector<int*>vecTrackbarValues = {&iRange_flood, &iDilate_size, &iCanny_th1, &iCanny_th2, &iOcc_th, &iNeigbor_th, &iFgs_lambda_flood, &iFgs_sigma_flood, &iFgs_iter_num, &iConf};
    createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
    // images
    cv::Mat dense, conf, imgShow;

    char mode = '1'; // 1: flood 2: spot 3: flood + spot
#ifdef SHOW_TIME
    chrono::system_clock::time_point t_start, t_end; 
#endif
    int frame_num = 82;
    int max_frame_num = 100;
    int fixedFrame = 1;
    while(1) {
        // file names
        cout << "frame = " << frame_num << endl;
        char szFN[255];
        sprintf(szFN, "%s/%08d_rgb_gray_img.png", strDataPath.c_str(), frame_num);
        string strGuide = string(szFN);
        sprintf(szFN, "%s/%08d_spot_depth_pc.exr", strDataPath.c_str(), frame_num);
        string strSpotPc = string(szFN);
        sprintf(szFN, "%s/%08d_flood_depth_pc.exr", strDataPath.c_str(), frame_num);
        string strFloodPc = string(szFN);
        // read dat
        cv::Mat imgGuide = cv::imread(strGuide, -1);
        cv::Mat pcFlood = cv::imread(strFloodPc, -1);
        cv::Mat pcSpot = cv::imread(strSpotPc, -1);
        // convert and set parameters
        convert_params_local2global(upsampling_params, preprocessing_params, 
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num);
        dc.set_upsampling_parameters(upsampling_params); 
        dc.set_preprocessing_parameters(preprocessing_params);
#ifdef SHOW_TIME
        t_start = chrono::system_clock::now();
#endif 
        // upsampling processing  
        if (mode == '1') {
            dc.run(imgGuide, pcFlood, cv::Mat(), dense, conf);
        } else if (mode == '2') {
            dc.run(imgGuide, cv::Mat(), pcSpot, dense, conf);
        } else if (mode == '3') {
            dc.run(imgGuide, pcFlood, pcSpot, dense, conf);
        } else if (mode == 'q') {
            break;
        } else {

        }
#ifdef SHOW_TIME
        t_end = chrono::system_clock::now();
        double elapsed = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
        cout << "\033[31;43mUpsampling total time = " << elapsed << " [us]\033[0m" << endl;
        g_FPS = (float)1000/elapsed*1000;
#endif
        // visualization
        cv::Mat dmapFlood = dc.get_flood_depthMap();
        cv::Mat dmapSpot = dc.get_spot_depthMap();
        imgShow = visualization(imgGuide, dmapFlood, dmapSpot, dense, conf, (float)iConf/10000.f, mode);
        cv::imshow(strWndName, imgShow);
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
            sprintf(szFN, "%s/%08d_dense_dmap.tiff", strDataPath.c_str(), frame_num);
            cv::imwrite(szFN, dense);
            sprintf(szFN, "%s/%08d_conf.tiff", strDataPath.c_str(), frame_num);
            cv::imwrite(szFN, conf);
            break;
        case 'r': // reset parameters
            convert_params_global2local(default_upsampling_params, default_preprocessing_params, 
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf);
            // update window
            cv::destroyWindow(strWndName);
            createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
            break;
        case 'p': // pause
            fixedFrame = 1 - fixedFrame;
        default:
            break;
        }
        if (fixedFrame == 0)
            frame_num += 1;
        if(frame_num >= max_frame_num)
            frame_num = 0;
    }
    exit(0);
}