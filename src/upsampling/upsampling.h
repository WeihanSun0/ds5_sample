#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/ximgproc.hpp>

typedef struct Upsampling_Params{
	float fgs_lambda_flood; // 0.1~100
	float fgs_sigma_color_flood; // 1~20
	float fgs_lambda_attenuation;
	float fgs_lambda_spot; // 1~1000
	float fgs_sigma_color_spot; // 1~20
	int fgs_num_iter_flood; //1~5
	int fgs_num_iter_spot; //1
} Upsampling_Params;



typedef struct Preprocessing_Params{
	float z_continuous_thresh; // (m) z threshold for continuous region 
	float occlusion_thresh; // threshold for justification of occlusion
	int guide_edge_dilate_size; // dilate size of guide image edge
	int canny_thresh1; // canny threshold 1
	int canny_thresh2; // canny threshold 2
	int range_flood; // flood upsampling range 
} Preprocessing_Params;

typedef struct Camera_Params{
	Camera_Params(float cx_, float cy_, float fx_, float fy_): cx(cx_), cy(cy_), fx(fx_), fy(fy_){};
	float cx;
	float cy;
	float fx;
	float fy;
} Camera_Params;

class upsampling
{
public:
	// number is method for upsampling
	upsampling();

	~upsampling() {};
	// set camera(RGB) parameters for xyz2depthmap
	void set_cam_paramters(const Camera_Params& params);
	// set upsampling processing paramters 
	void set_upsampling_parameters(const Upsampling_Params& params); 
	// get default upsampling processing paramters
	void get_default_upsampling_parameters(Upsampling_Params& params);
	// set preprocessing paramters 
	void set_preprocessing_parameters(const Preprocessing_Params& params);
	// get default preprocessing paramters
	void get_default_preprocessing_parameters(Preprocessing_Params& params);	
	// main processing interface
	bool run(const cv::Mat& rgb, const cv::Mat& flood_pc, const cv::Mat& spot_pc, cv::Mat& dense, cv::Mat& conf);
	// for show depthmap
	cv::Mat get_flood_depthMap() {return this->m_flood_dmap_;};
	cv::Mat get_spot_depthMap() {return this->m_spot_dmap_;};
	// convert depth map to point cloud
	void depth2pc(const cv::Mat& depth, cv::Mat& pc);
private:
	void clear(); // clear temperary variables
	void initialization(cv::Mat& dense, cv::Mat& conf); // initialization
	void run_flood(const cv::Mat& img_guide, const cv::Mat& pc_flood, cv::Mat& dense, cv::Mat& conf); // processing for flood
	void run_spot(const cv::Mat& img_guide, const cv::Mat& pc_spot, cv::Mat& dense, cv::Mat& conf); // processing for spot
	void fgs_f(const cv::Mat & sparse, const cv::Mat& mask, const cv::Rect& roi, const float& lambda, 
					cv::Mat& dense, cv::Mat& conf);
	void spot_guide_proc(const cv::Mat& guide); // guide image processing for spot
	void spot_depth_proc(const cv::Mat& pc_spot); // depth processing for flood
	void spot_preprocessing(const cv::Mat& guide, const cv::Mat& pc_spot); // preprocessing for spot
	void flood_guide_proc(const cv::Mat& guide); // guide image processing for flood
	void flood_guide_proc2(const cv::Mat& guide); // guide image processing 2 for flood
	void flood_depth_proc(const cv::Mat& pc_flood); // depth processing for flood
	void flood_depth_proc_with_edge_fixed(const cv::Mat& pc_flood);
	void flood_depth_proc_without_edge(const cv::Mat& pc_flood);
	void flood_preprocessing(const cv::Mat& guide, const cv::Mat& pc_flood); // preprocessing for flood
private:
	const int m_guide_width_ = 960; // guide image width 
	const int m_guide_height_ = 540; // guide image height
	const int m_grid_width_ = 80; // flood depth grid width
	const int m_grid_height_ = 60; // flood depth grid height
	int m_mode_ = 0; // 0: no processing, 1: only flood, 2: only spot, 3: both flood and spot
	// temperary data
	cv::Mat m_flood_mask_; // 32FC1 
	cv::Mat m_flood_range_; // 32FC1 
	cv::Mat m_spot_mask_; // 32FC1 
	cv::Mat m_spot_range_; // 32FCC1 
	cv::Mat m_flood_dmap_; // 32FC1 depth map resolution same as guide
	cv::Mat m_spot_dmap_; // 32FC1 depth map resuolution same as guide 
	cv::Mat m_flood_grid_; // 32FC1
	cv::Mat m_flood_edge_; // 32FC1
	cv::Mat m_guide_edge_; // 8UC1 0 or 255
	cv::Rect m_flood_roi_; // ROI for flood
	cv::Rect m_spot_roi_; // ROI for spot
	// upsampling main processing paramters
	float m_fgs_lambda_flood_ = 220.f; // 0.1~1000
	float m_fgs_sigma_color_flood_ = 4.f; // 0~256
	float m_fgs_lambda_attenuation_ = 0.25f;
	float m_fgs_lambda_spot_ = 700.f; 
	float m_fgs_sigma_color_spot_ = 5.f; // 1~20
	int m_fgs_num_iter_flood_ = 1; //1~5
	int m_fgs_num_iter_spot_ = 2;
	// flood preprocessing paramters
	int m_guide_edge_dilate_size_ = 4; // (1~10) dilate size of guide image edge
	float m_z_continuous_thresh_ = 0.1f; // (0~1) z threshold for continuous region 
	float m_occlusion_thresh_ = 10.5f; // (0~20.0)  threshold for justification of occlusion (max pixels between neigbors)
	int m_canny_thresh1_ = 40; // 0~255
	int m_canny_thresh2_ = 170; // 0~255
	int m_range_flood_ = 20; // 2~40
	// camera paramters
	float m_fx_ = 135.51f;
	float m_fy_ = 135.51f;
	float m_cx_ = 159.81f;
	float m_cy_ = 120.41f;
	// processing flag
	bool m_depth_edge_proc_on_ = true;
	bool m_guide_edge_proc_on_ = true;
	cv::Ptr<cv::ximgproc::FastGlobalSmootherFilter> m_fgs_filter_;
};

