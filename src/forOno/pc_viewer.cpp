
#include "common/dsviewer_interface.h"
#include "common/z2color.h"
#include "upsampling/upsampling.h"
#include <opencv2/opencv.hpp>
#include "viewer.hpp"
#include <string>
using namespace std;

const string rootPath = "../../";
string strDataPath;
string strSavePath = ".";

const string strParam = rootPath + "dat/camParam/camera_calib/param_ono.txt";

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
        cout << "*** point cloud viewer ***" << endl;
        cout << "USAGE:" << endl;
        cout << "   <exe> <input data path> <start_fid> <end_fid>" << endl;
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

   // images
    cv::Mat dense; // output
    cv::Mat pc_filtered;
    // viewer pc
    myViz viz;	
    int fixedFrame = 0; // 0: no fixed, 1: fixed
    int frameShift = 0;
    int curr_frame_idx = start_frame_idx;
    while(1) {
        // file names
        cout << "frame = " << curr_frame_idx << " shift = " << frameShift << endl;
        char szFN[255];
        sprintf_s(szFN, "%s/%08d_rgb_gray_img.png", strDataPath.c_str(), curr_frame_idx);
        string strGuide = string(szFN);
        sprintf_s(szFN, "%s/%08d_flood_pc.exr", strDataPath.c_str(), curr_frame_idx + frameShift);
        string strFloodPc = string(szFN);
        sprintf_s(szFN, "%s/%08d_dense_pc.exr", strDataPath.c_str(), curr_frame_idx + frameShift);
        string strDensePc = string(szFN);
        sprintf_s(szFN, "%s/%08d_dense_dmap.tiff", strDataPath.c_str(), curr_frame_idx + frameShift);
        string strDenseDmap= string(szFN);
        // read dat
        cv::Mat imgGuide = cv::imread(strGuide, -1);
        cv::Mat pcFlood = cv::imread(strFloodPc, -1);
        cv::Mat pcDense = cv::imread(strDensePc, -1);
        cv::Mat dmapDense = cv::imread(strDenseDmap, -1);
        // cout << imgGuide.size() << endl;
        // cout << pcFlood.size() << endl;
        // cout << pcDense.size() << endl;
        // cout << dmapDense.size() << endl;
        if (!imgGuide.empty()) {
            cv::imshow("dense", dmapDense);
            viz.set(pcFlood, cv::viz::Color::green(), 5);
            viz.set(pcDense, imgGuide, 3);
            viz.show(true);
            char c= cv::waitKey(30);
            switch (c)
            {
            case 'q': // quit
                exit(0);
            case 'p': // pause
                fixedFrame = 1 - fixedFrame;
                break;
            default:
                break;
            }
        }
        if (fixedFrame == 0) { // not paused
            curr_frame_idx += 1;
        }
        if(curr_frame_idx > end_frame_idx) // repeat
            curr_frame_idx = start_frame_idx;
        if(curr_frame_idx < 0) // loop
            curr_frame_idx = end_frame_idx;
    }
    exit(0);
}