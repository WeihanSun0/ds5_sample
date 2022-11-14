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
#include "common/dsviewer_interface.h"
#include "common/z2color.h"
#include "upsampling/upsampling.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include "playground.hpp"
#include "viewer.hpp"

using namespace std;

#define SHOW_TIME 
#define __TEST_GUIDE__
#define __TL10__

const string rootPath = "../../";
string strDataPath;
string strSavePath = ".";

#define FIX_COLORMAP
#ifdef FIX_COLORMAP
float g_min_value = 0.0;
float g_max_value = 1.0;
#endif

#ifdef __TL10__
const string strParam = rootPath + "dat/camParam/camera_calib/param_tl10.txt";
#else
const string strParam = rootPath + "dat/camParam/camera_calib/param_sun.txt";
#endif

float g_FPS = 0.0;

int iDilate_size, iCanny_th1, iCanny_th2;
// * DEBUG: to check guide image 
void guide_edge_extraction(const cv::Mat& guide, cv::Mat & imgEdgeGuide, cv::Mat & imgBoarder)
{
    cv::Canny(guide, imgEdgeGuide, iCanny_th1, iCanny_th2, 3);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, 
        cv::Size(iDilate_size, iDilate_size));
    cv::dilate(imgEdgeGuide, imgBoarder, kernel);
}

// ANCHOR

//* draw edge detection results
cv::Mat visualization_test(const cv::Mat& guide)
{
    cv::Mat imgGuideEdge, imgBoarder;
    guide_edge_extraction(guide, imgGuideEdge, imgBoarder);
    // cout << "old = " << imgGuideEdge.type() << endl;
    cv::Mat imgMarkedEdge = markEdge(guide, imgBoarder, cv::Vec3b(255, 0, 0));
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    vecImgs.push_back(imgMarkedEdge);
    vecLabel.push_back("guide");
    // vecImgs.push_back(imgGuideEdge);
    // vecLabel.push_back("edge");
    vecImgs.push_back(imgBoarder);
    vecLabel.push_back("boarder");
    return mergeImages(vecImgs, vecLabel, cv::Size(2,1));
}

//* try alternative edge detection
cv::Mat visualization_test3(const cv::Mat& guide)
{
    cv::Mat imgGuideEdge;
    imgGuideEdge = sobel_edge_detection(guide, 50, iDilate_size);
    // cout << "new = " << imgGuideEdge.type() << endl;
    cv::Mat imgMarkedEdge = markEdge(guide, imgGuideEdge, cv::Vec3b(255, 0, 0));
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    vecImgs.push_back(imgMarkedEdge);
    vecLabel.push_back("guide");
    vecImgs.push_back(imgGuideEdge);
    vecLabel.push_back("edge");
    return mergeImages(vecImgs, vecLabel, cv::Size(2,1));
}

cv::Mat visualization_test_edge(const cv::Mat& cur_guide, const cv::Mat& last_guide, const cv::Mat& cur_depth, const cv::Mat& last_depth)
{
    cv::Mat imgGuideEdge, imgDepthEdge;
    imgGuideEdge = EVS_guide_edge_detection(cur_guide, last_guide);
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    vecImgs.push_back(imgGuideEdge);
    vecLabel.push_back("guide edge");
    return mergeImages(vecImgs, vecLabel, cv::Size(1, 1)); 
}



//* debug: depth edge
cv::Mat visualization_depth_grid(const cv::Mat& pc_flood, const Camera_Params& params)
{
    int width = 80;
    int height = 60;
    int scale = 8;
    cv::Mat depthGrid = cv::Mat::zeros(cv::Size(width, height), CV_32FC1);
    float fx = params.fx;
    float fy = params.fy;
    float cx = params.cx;
    float cy = params.cy; 
    for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float z = pc_flood.at<cv::Vec3f>(y, x)[2];
            if (!isnan(z)) {
                depthGrid.at<float>(y, x) = z;
            }
        }
    }
    double minVal, maxVal;
    cv::minMaxLoc(depthGrid, &minVal, &maxVal);
    cv::Mat color_grid = z2colormap(depthGrid, minVal, maxVal);
    cv::Mat scaled_color_grid = enlarge_grid(color_grid, 8);
    cv::Mat edge = extract_depth_edge(pc_flood);
    cv::minMaxLoc(edge, &minVal, &maxVal);
    cv::Mat color_edge = z2colormap(edge, minVal, maxVal);
    cv::Mat scaled_color_edge = enlarge_grid(color_edge, 8);
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    char szLabel[255];
    vecImgs.push_back(scaled_color_grid);
    sprintf_s(szLabel, "grid");
    vecLabel.push_back(szLabel);
    // test
    vecImgs.push_back(scaled_color_edge);
    sprintf_s(szLabel, "edge");
    vecLabel.push_back(szLabel);
    return mergeImages(vecImgs, vecLabel, cv::Size(2, 1));
}

/**
 * @brief Visualization function
 * 
 * @param guide : guide image 
 * @param dmapFlood : input depthmap of flood 
 * @param dmapSpot : input depthmap of spot 
 * @param dense : upsampling result dense depthmap 
 * @param conf : upsampling result confidence map 
 * @param conf_thresh : confidence threshold for filtering 
 * @param mode : current show mode 
 * @return cv::Mat : image for show 
 */
cv::Mat visualization(const cv::Mat& guide, const cv::Mat& dmapFlood, const cv::Mat& dmapSpot,
                        const cv::Mat& dense, const cv::Mat& conf, float conf_thresh, char mode)
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
    cv::Mat imgFiltered;
    dense.copyTo(imgFiltered);
    imgFiltered.setTo(std::nan(""), conf < conf_thresh);
#ifdef FIX_COLORMAP
    minVal = g_min_value;
    maxVal = g_max_value;
#else
    cv::minMaxLoc(imgFiltered, &minVal, &maxVal);
#endif
    cv::Mat colorFiltered = z2colormap(imgFiltered, minVal, maxVal);
    cv::Mat imgOverlapFiltered = overlap(colorFiltered, guide, 0.5);
    // output
#ifndef FIX_COLORMAP
    cv::minMaxLoc(dense, &minVal, &maxVal);
#endif
    cv::Mat colorDense = z2colormap(dense, minVal, maxVal);
    cv::Mat imgOverlapDense = overlap(colorDense, guide, 0.5);
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
    // vecImgs.push_back(imgOverlapDense);
    // vecLabel.push_back("dense");
    vecImgs.push_back(imgOverlapFiltered);
    vecLabel.push_back("filtered");
    return mergeImages(vecImgs, vecLabel, cv::Size(2, 1));
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
 * @param upsampling_params : global parameters
 * @param preprocessing_params : global parameters 
 * @param iRange_flood : trackbar parameters 
 * @param iDilate_size  : trackbar parameters 
 * @param iCanny_th1  : trackbar parameters 
 * @param iCanny_th2  : trackbar parameters 
 * @param iOcc_th  : trackbar parameters 
 * @param iNeigbor_th  : trackbar parameters 
 * @param iFgs_lambda_flood  : trackbar parameters 
 * @param iFgs_sigma_flood  : trackbar parameters 
 * @param iFgs_iter_num  : trackbar parameters 
 * @param iConf  : trackbar parameters 
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
    iConf = 400; 
}

/**
 * @brief convert trackbar parameters (local) to upsampling cleass parameters (global)
 * 
 * @param upsampling_params : global parameters 
 * @param preprocessing_params  : global parameters 
 * @param iRange_flood  : local parameters 
 * @param iDilate_size   : local parameters 
 * @param iCanny_th1   : local parameters 
 * @param iCanny_th2   : local parameters 
 * @param iOcc_th   : local parameters 
 * @param iNeigbor_th   : local parameters 
 * @param iFgs_lambda_flood   : local parameters 
 * @param iFgs_sigma_flood   : local parameters 
 * @param iFgs_iter_num   : local parameters 
 */
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

// * check no preprocessing results
inline void disable_preprocessing(int& iCanny_th1, int& iCanny_th2, int& iOcc_th, int& iNeigbor_th)
{
    iCanny_th1 = 250;
    iCanny_th2 = 10;
    iOcc_th = 0;
    iNeigbor_th = 100;
}

// * save pc for python to avoid exr file read
void save_csv(const cv::Mat& img, const string& strFileName)
{
    fstream fs;
    fs.open(strFileName, ios::out);
    for (int r = 0; r < img.rows; r++) {
        for (int c = 0; c < img.cols; c++) {
            cv::Vec3f v3f = img.at<cv::Vec3f>(r,c);
            fs << v3f[0] << "," << v3f[1] << "," << v3f[2] << endl;
        }
    }
    fs.close();
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
    convert_params_global2local(upsampling_params, preprocessing_params, 
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf);
    
    // GUI setting
    string strWndName = "DS5 sample";
    // paramter trackbars setting
    vector<string>vecTrackbarLabels = {"range", "dilate size", "canny th1", "canny th2", "occ th", "neigbor th", "fgs lambda", "fgs sigma", "iter num", "conf"};
    vector<int>vecTrackbarValueRanges = {40, 20, 255, 255, 200, 100, 1000, 20, 5, 10000};
    vector<int*>vecTrackbarValues = {&iRange_flood, &iDilate_size, &iCanny_th1, &iCanny_th2, &iOcc_th, &iNeigbor_th, &iFgs_lambda_flood, &iFgs_sigma_flood, &iFgs_iter_num, &iConf};
    createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
    // images
    cv::Mat dmapFlood, dmapSpot; // input
    cv::Mat dense, conf, imgShow; // output
    cv::Mat dmap_filtered, pc_filtered;
    // viewer pc
    myViz viz;	
    int showViz = 0; // 1: show point cloud
    int remove_depth_edge = 0;
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
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num);
        dc.set_upsampling_parameters(upsampling_params); 
        dc.set_preprocessing_parameters(preprocessing_params);
#ifdef SHOW_TIME
        t_start = chrono::system_clock::now();
#endif 
        //! test remove depth edge
        cv::Mat new_pcFlood;
        if (remove_depth_edge == 1) {
            new_pcFlood = remove_depth_edge_func(pcFlood);
        } else {
            pcFlood.copyTo(new_pcFlood);
        }
        // upsampling processing  
        bool res = false; // upsampling success or not
        if (mode == '1') {
            // res = dc.run(imgGuide, pcFlood, cv::Mat(), dense, conf);
            res = dc.run(imgGuide, new_pcFlood, cv::Mat(), dense, conf);
        } else if (mode == '2') {
            res = dc.run(imgGuide, cv::Mat(), pcSpot, dense, conf);
        } else if (mode == '3') {
            // res = dc.run(imgGuide, pcFlood, pcSpot, dense, conf);
            res = dc.run(imgGuide, new_pcFlood, pcSpot, dense, conf);
        } else if (mode == 'q') {
            break;
        } else {
            ;
        }
        
#ifdef SHOW_TIME
        t_end = chrono::system_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(t_end - t_start).count();
#ifdef __TL10__
        cout << "\033[31;43mUpsampling total time = " << elapsed << " [us]\033[0m" << " __TL10__"  << endl;
#else        
        cout << "\033[31;43mUpsampling total time = " << elapsed << " [us]\033[0m" << " __SUN__"  << endl;
#endif
        g_FPS = (float)1000.f/(float)elapsed*1000.f;
#endif
        if(res) {
            // visualization
            dmapFlood = dc.get_flood_depthMap();
            dmapSpot = dc.get_spot_depthMap();
            imgShow = visualization(imgGuide, dmapFlood, dmapSpot, dense, conf, (float)iConf/10000.f, mode);
            cv::imshow(strWndName, imgShow);
            dmap_filtered = get_filtered_dense(dense, conf, (float)iConf/10000.f);
#ifdef __TEST_GUIDE__
            // cv::Mat imgTestGuide = visualization_test(imgGuide);
            // cv::imshow("original guide edge", imgTestGuide);
            cv::imshow("filtered", dmap_filtered);
            cv::Mat imgTestDepth = visualization_depth_grid(pcFlood, cam_params);
            cv::imshow("depth edge", imgTestDepth);
            // cv::Mat imgTestGuideNewEdge = visualization_test3(imgGuide);
            // cv::imshow("new guide edge", imgTestGuideNewEdge);
            // cv::Mat imgEVSGuideEdge = visualization_test_edge(imgGuide, last_guide, pcFlood, last_depth);
            // cv::imshow("evs guide edge", imgEVSGuideEdge);
#endif
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
                // sprintf_s(szFN, "%s/%08d_flood.csv", strSavePath.c_str(), curr_frame_idx);
                // save_csv(pcFlood, szFN);
            }
            break;
        case 'r': // reset parameters to default
            convert_params_global2local(default_upsampling_params, default_preprocessing_params, 
                            iRange_flood, iDilate_size, iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th, iFgs_lambda_flood, iFgs_sigma_flood, iFgs_iter_num, iConf);
            // update window
            cv::destroyWindow(strWndName);
            createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
            break;
        case 'd': // disable preprocessing
            disable_preprocessing(iCanny_th1, iCanny_th2, iOcc_th, iNeigbor_th);
             // update window
            cv::destroyWindow(strWndName);
            createWindow(strWndName, vecTrackbarLabels, vecTrackbarValues, vecTrackbarValueRanges);
            break;
        case 'n': // remove depth edge
            remove_depth_edge = 1 - remove_depth_edge;
            break;
        case 'g': // * debug: fix bug in depth edge process
            if (use_new_depth == 2)
                use_new_depth = 0;
            else 
                use_new_depth = 2;
            dc.set_debug_flag(use_new_depth);
            break;
        case 'f': // * debug: feeding
            if (use_new_depth == 3)
                use_new_depth = 0;
            else 
                use_new_depth = 3;
            dc.set_debug_flag(use_new_depth);
            break;
        case 'p': // pause
            fixedFrame = 1 - fixedFrame;
            break;
        case 'v': // view point cloud
            showViz = 1 - showViz;
        case '.': // forward
            curr_frame_idx += 1;
            break;
        case ',': // backward
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
        if (showViz == 1) {
            viz.set(pcFlood, cv::viz::Color::green(), 5);
            dc.depth2pc(dmap_filtered, pc_filtered);
            viz.set(pc_filtered, imgGuide, 3);
            viz.show(true);
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