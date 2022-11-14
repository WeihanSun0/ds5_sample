#include <opencv2/opencv.hpp>
#include <chrono>
#include "common/z2color.h"
using namespace std;


cv::Mat sobel_edge_detection_8U(const cv::Mat& src, int tapsize_sobel)
{
	cv::Mat h_sobels, v_sobels;
	cv::Sobel(src, h_sobels, CV_32F, 1, 0, tapsize_sobel);
	cv::Sobel(src, v_sobels, CV_32F, 0, 1, tapsize_sobel);
	cv::Mat dst = 0.5 * (abs(h_sobels) + abs(v_sobels));
    // double minVal, maxVal;
    // cv::minMaxLoc(dst, &minVal, &maxVal);
    // cv::Mat dst8u;
    // cv::threshold(dst, dst8u, 1, maxVal, cv::THRESH_BINARY);
    // cout << maxVal << ":" << dst8u.depth() << endl;
	return dst;
}

cv::Mat normalized_to_8U(const cv::Mat src)
{
    double minVal, maxVal;
    cv::minMaxLoc(src, &minVal, &maxVal);
    cv::Mat dst = (src - minVal);
    dst.convertTo(dst, CV_8U, 255./(maxVal - minVal));
    return dst;
}

cv::Mat enlarge_grid(const cv::Mat& grid, int scale = 8)
{
    int width = grid.cols;
    int height = grid.rows;
    cv::Mat scaledGrid = cv::Mat::zeros(cv::Size(width*scale, height*scale), CV_8UC3);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            for (int j = 0; j < scale; ++j) {
                for (int i = 0; i < scale; ++i) {
                    cv::Vec3b color = grid.at<cv::Vec3b>(y, x);
                    scaledGrid.at<cv::Vec3b>(y*scale+j, x*scale+i) = color;
                }
            }
        }
    }
    return scaledGrid;
}


cv::Mat sobel_edge_detection(const cv::Mat & guide, int threshold, int boarder_size)
{
    cv::Mat edge = sobel_edge_detection_8U(guide, 3);
    cv::Mat normalized_edge = normalized_to_8U(edge);
    cv::Mat binary_edge, imgBoarder;
    cv::threshold(edge, binary_edge, threshold, 255, cv::THRESH_BINARY);
    //32F 2 8U
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, 
        cv::Size(boarder_size, boarder_size));
    cv::dilate(binary_edge, imgBoarder, kernel);
    cv::Mat imgBoarder_8U;
    imgBoarder.convertTo(imgBoarder_8U, CV_8UC1);
    return imgBoarder_8U;
}

cv::Mat EVS_guide_edge_detection(const cv::Mat & cur_img, const cv::Mat & last_img)
{
    cv::Mat imgEdgeS = cv::Mat::zeros(cur_img.size(), CV_16S); // -255 -> 255
    cv::Mat imgEdge = cv::Mat::zeros(cur_img.size(), CV_8UC3);
    cv::Mat cur_imgS, last_imgS;
    if (last_img.empty()) 
        return imgEdge;
    cv::subtract(cur_img, last_img, imgEdgeS);
    // imgEdgeS = cur_img - last_img;
    short threshold = 400;
    imgEdgeS.forEach<short>([&imgEdge, &threshold](short& pixel, const int pos[]) -> void {
        int x = pos[1];
        int y = pos[0];
		if (pixel < -threshold) { // minor edge
			imgEdge.at<cv::Vec3b>(y, x)[0] = 0;
			imgEdge.at<cv::Vec3b>(y, x)[1] = 0; 
			imgEdge.at<cv::Vec3b>(y, x)[2] = 255;
		} else if (pixel > threshold) {
			imgEdge.at<cv::Vec3b>(y, x)[0] = 0;
			imgEdge.at<cv::Vec3b>(y, x)[1] = 255; 
			imgEdge.at<cv::Vec3b>(y, x)[2] = 0;
        }
	});
    return imgEdge;
}

cv::Mat extract_depth_edge(const cv::Mat & flood)
{
    cv::Mat edges, flood_tmp;
    flood_tmp = cv::Mat::zeros(flood.size(), CV_32FC1);
    edges = cv::Mat::zeros(flood.size(), CV_8UC1);
    float inval = 10.0f;
    flood.forEach<cv::Vec3f>([&flood_tmp, &inval](cv::Vec3f& pixel, const int pos[]) -> void {
        int x = pos[1];
        int y = pos[0];
        float z = pixel[2];
        if (isnan(z)) {
            flood_tmp.at<float>(y, x) = inval;
        } else {
            flood_tmp.at<float>(y, x) = z;
        }
    });
    float threshold = 0.1f;
    for (int r = 1; r < flood_tmp.rows-1; ++r) {
        for (int c = 1; c < flood_tmp.cols-1; ++c) {
            if (flood_tmp.at<float>(r, c) == inval)
                continue;
            float maxGrad = 0.0f, fGrad = 0.0f;
            fGrad = fabs(flood_tmp.at<float>(r, c-1) - flood_tmp.at<float>(r, c+1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c) - flood_tmp.at<float>(r+1, c));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c-1) - flood_tmp.at<float>(r+1, c+1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c+1) - flood_tmp.at<float>(r+1, c-1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            // set edge
            if (maxGrad > threshold)
                edges.at<uchar>(r, c) = 255; 
        }
    }
    return edges;
}

cv::Mat remove_depth_edge_func(const cv::Mat& flood)
{
    cv::Mat res, flood_tmp;
    flood.copyTo(res);
    flood_tmp = cv::Mat::zeros(flood.size(), CV_32FC1);
    float inval = 10.0f;
    flood.forEach<cv::Vec3f>([&flood_tmp, &inval](cv::Vec3f& pixel, const int pos[]) -> void {
        int x = pos[1];
        int y = pos[0];
        float z = pixel[2];
        if (isnan(z)) {
            flood_tmp.at<float>(y, x) = inval;
        } else {
            flood_tmp.at<float>(y, x) = z;
        }
    });
    float threshold = 0.1f;
    for (int r = 1; r < flood_tmp.rows-1; ++r) {
        for (int c = 1; c < flood_tmp.cols-1; ++c) {
            if (flood_tmp.at<float>(r, c) == inval)
                continue;
            float maxGrad = 0.0f, fGrad = 0.0f;
            fGrad = fabs(flood_tmp.at<float>(r, c-1) - flood_tmp.at<float>(r, c+1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c) - flood_tmp.at<float>(r+1, c));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c-1) - flood_tmp.at<float>(r+1, c+1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            fGrad = fabs(flood_tmp.at<float>(r-1, c+1) - flood_tmp.at<float>(r+1, c-1));
            if (fGrad > maxGrad) maxGrad = fGrad;
            // set edge
            if (maxGrad > threshold) {
                res.at<cv::Vec3f>(r, c)[0] = std::nan(""); 
                res.at<cv::Vec3f>(r, c)[1] = std::nan(""); 
                res.at<cv::Vec3f>(r, c)[2] = std::nan(""); 
            }
        }
    }
    return res;
}

cv::Mat get_filtered_dense(const cv::Mat & dense, const cv::Mat & conf, float conf_thresh)
{
    cv::Mat imgFiltered;
    dense.copyTo(imgFiltered);
    imgFiltered.setTo(std::nan(""), conf < conf_thresh);
    return imgFiltered;
}

