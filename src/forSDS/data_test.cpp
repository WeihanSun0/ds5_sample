/**
 * @file data_test.cpp
 * @author Weihan.Sun (weihan.sun@sony.com)
 * @brief test data for SDS
 * @version 0.1
 * @date 2022-11-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "common/dsviewer_interface.h"
#include "common/z2color.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace std;

#define SHOW_TIME 
#define __TEST_GUIDE__
#define __TL10__

const string rootPath = "../../";
string strDataPath;

#ifdef __TL10__
const string strParam = rootPath + "dat/camParam/camera_calib/param_tl10.txt";
#else
const string strParam = rootPath + "dat/camParam/camera_calib/param_sun.txt";
#endif

float g_FPS = 0.0;

int iDilate_size, iCanny_th1, iCanny_th2;

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
cv::Mat visualization(const cv::Mat& guide, const cv::Mat& dmapFlood, const cv::Mat& dmapSpot)
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
   
        //
    vector<cv::Mat> vecImgs;
    vector<string> vecLabel;
    char szLabel[255];
    vecImgs.push_back(imgGuideMapFlood);
    sprintf_s(szLabel, "flood");
    vecLabel.push_back(szLabel);
    vecImgs.push_back(imgGuideMapSpot);
    sprintf_s(szLabel, "spot");
    vecLabel.push_back(szLabel);
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
        cout << "*** DS5 data refine for SDS ***" << endl;
        cout << "USAGE:" << endl;
        cout << "   <exe> <data path> <start idx> <end_idx>" << endl;
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

    // GUI setting
    string strWndName = "DS5 sample";
    // images
    cv::Mat dmapFlood, dmapSpot; // input
    cv::Mat imgShow; // output

    char mode = '3'; // 1: flood 2: spot 3: flood + spot q: exit
#ifdef SHOW_TIME
    chrono::system_clock::time_point t_start, t_end; 
#endif
    int fixedFrame = 0; // 0: no fixed, 1: fixed
    int curr_frame_idx = start_frame_idx;

    // disable preprocessing
    while(1) {
        if (mode == 'q') {
            break;
        }
        // file names
        cout << "frame = " << curr_frame_idx << endl;
        char szFN[255];
        sprintf_s(szFN, "%s/%08d_guide.png", strDataPath.c_str(), curr_frame_idx);
        string strGuide = string(szFN);
        sprintf_s(szFN, "%s/%08d_spot_dmap.tiff", strDataPath.c_str(), curr_frame_idx);
        string strSpotDmap = string(szFN);
        sprintf_s(szFN, "%s/%08d_flood_dmap.tiff", strDataPath.c_str(), curr_frame_idx);
        string strFloodDmap = string(szFN);
        // read dat
        cv::Mat imgGuide = cv::imread(strGuide, -1);
        cv::Mat dmapFlood = cv::imread(strFloodDmap, -1);
        cv::Mat dmapSpot = cv::imread(strSpotDmap, -1);
                        
        cout << "read finished"  << endl;
        cout << imgGuide.size() << endl;
        // visualization
        imgShow = visualization(imgGuide, dmapFlood, dmapSpot);
        cv::imshow(strWndName, imgShow);
        char c = cv::waitKey(100);

        switch (c)
        {
        case 'q': 
            mode = 'q';
            break;
        case 'p': // pause
            fixedFrame = 1 - fixedFrame;
            break;
        default:
            break;
        }
        if (fixedFrame == 0)
            curr_frame_idx += 1;
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
