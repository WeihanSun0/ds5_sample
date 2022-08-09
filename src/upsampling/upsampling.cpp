#include "upsampling.h" 
#include <chrono>
#include <thread>
#include <math.h>
// #define SHOW_TIME

/**
 * @brief Construct a new upsampling::upsampling object
 * 
 */
upsampling::upsampling()
{
	this->m_flood_mask_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_flood_range_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_spot_mask_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_spot_range_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_flood_dmap_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_spot_dmap_ = cv::Mat::zeros(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
	this->m_flood_grid_ = cv::Mat::zeros(cv::Size(m_grid_width_, m_grid_height_), CV_32FC1);
	this->m_guide_edge_ = cv::Mat::ones(cv::Size(m_guide_width_, m_guide_height_), CV_32FC1);
}


/**
 * @brief Set the camera paramters
 * 
 * @param params camera paramters structure
 */
void upsampling::set_cam_paramters(const Camera_Params& params)
{
	this->m_cx_ = params.cx;
	this->m_cy_ = params.cy;
	this->m_fx_ = params.fx;
	this->m_fy_ = params.fy;
}

/**
 * @brief set upsampling paramters
 * 
 * @param params upsampling parameter structure 
 */
void upsampling::set_upsampling_parameters(const Upsampling_Params& params) 
{ 
	this->m_fgs_lambda_flood_ = params.fgs_lambda_flood; 
	this->m_fgs_sigma_color_flood_ = params.fgs_sigma_color_flood; 
	this->m_fgs_lambda_spot_ = params.fgs_lambda_spot; 
	this->m_fgs_sigma_color_spot_ = params.fgs_sigma_color_spot; 
	this->m_fgs_num_iter_flood_ = params.fgs_num_iter_flood;
	this->m_fgs_num_iter_spot_ = params.fgs_num_iter_spot;
	if (this->m_fgs_lambda_flood_ < 1) this->m_fgs_lambda_flood_ = 1;
	if (this->m_fgs_sigma_color_flood_ < 1) this->m_fgs_sigma_color_flood_ = 1;
	if (this->m_fgs_num_iter_flood_ < 1) this->m_fgs_num_iter_flood_ = 1;
	if (this->m_fgs_num_iter_flood_ > 5) this->m_fgs_num_iter_flood_ = 5;
	if (this->m_fgs_num_iter_spot_ < 1) this->m_fgs_num_iter_spot_ = 1;
	if (this->m_fgs_num_iter_spot_ > 5) this->m_fgs_num_iter_spot_ = 5;
};


/**
 * @brief get default upsampling parameters
 * 
 * @param params upsampling paramters structure 
 */
void upsampling::get_default_upsampling_parameters(Upsampling_Params& params)
{
	params.fgs_lambda_flood = this->m_fgs_lambda_flood_;
	params.fgs_sigma_color_flood = this->m_fgs_sigma_color_flood_;
	params.fgs_lambda_spot = this->m_fgs_lambda_spot_;
	params.fgs_sigma_color_spot = this->m_fgs_sigma_color_spot_;
	params.fgs_num_iter_flood = this->m_fgs_num_iter_flood_;
	params.fgs_num_iter_spot = this->m_fgs_num_iter_spot_;
}


/**
 * @brief set preprocessing paramters
 * 
 * @param params preprocessing paramters structure 
 */
void upsampling::set_preprocessing_parameters(const Preprocessing_Params& params)
{
	this->m_guide_edge_dilate_size_ = params.guide_edge_dilate_size;
	this->m_canny_thresh1_ = params.canny_thresh1;
	this->m_canny_thresh2_ = params.canny_thresh2;
	this->m_range_flood_ = params.range_flood;
	this->m_z_continuous_thresh_ = params.z_continuous_thresh;
	this->m_occlusion_thresh_ = params.occlusion_thresh;
	if (this->m_z_continuous_thresh_ == 1.0 && this->m_occlusion_thresh_ == 0.0)
		this->m_depth_edge_proc_on_ = false;
	else
		this->m_depth_edge_proc_on_ = true;
	if (this->m_canny_thresh1_ > this->m_canny_thresh2_)
		this->m_guide_edge_proc_on_ = false;
	else
		this->m_guide_edge_proc_on_ = true;
	// checkerror
	if (this->m_range_flood_ < 2) this->m_range_flood_ = 2;
	if (this->m_guide_edge_dilate_size_ < 1) this->m_guide_edge_dilate_size_ = 1;
};

/**
 * @brief get default preprocessing parameters
 * 
 * @param params preprocessing paramters structure
 */
void upsampling::get_default_preprocessing_parameters(Preprocessing_Params& params)
{
	params.guide_edge_dilate_size = this->m_guide_edge_dilate_size_; 
	params.canny_thresh1 = this->m_canny_thresh1_;
	params.canny_thresh2 = this->m_canny_thresh2_;
	params.range_flood = this->m_range_flood_;
	params.occlusion_thresh = this->m_occlusion_thresh_;
	params.z_continuous_thresh = this->m_z_continuous_thresh_;
};


/**
 * @brief clear buffers
 * 
 */
void upsampling::clear()
{
	this->m_flood_dmap_.setTo(0.0);
	this->m_flood_grid_.setTo(0.0);
	this->m_flood_mask_.setTo(0.0);
	this->m_flood_range_.setTo(0.0);
	this->m_spot_dmap_.setTo(0.0);
	this->m_spot_mask_.setTo(0.0);
	this->m_spot_range_.setTo(0.0);
	this->m_guide_edge_.setTo(1.0);
	this->m_flood_roi_ = cv::Rect(0, 0, this->m_guide_width_, this->m_guide_height_);
	this->m_spot_roi_ = cv::Rect(0, 0, this->m_guide_width_, this->m_guide_height_);
}

/**
 * @brief initialization 
 * 
 * @param dense 
 * @param conf 
 */
void upsampling::initialization(cv::Mat& dense, cv::Mat& conf)
{
	this->clear();
	if(dense.empty())
		dense = cv::Mat::zeros(cv::Size(this->m_guide_width_, this->m_guide_height_), CV_32FC1);
	else
		dense.setTo(0);
	
	if(conf.empty())
		conf = cv::Mat::zeros(cv::Size(this->m_guide_width_, this->m_guide_height_), CV_32FC1);
	else
		conf.setTo(0);
}

/**
 * @brief FGS filter processing
 * 
 * @param sparse: sparse depth 
 * @param mask: mask  
 * @param roi: ROI 
 * @param dense: output dense depth 
 * @param conf: output confidence 
 */
void upsampling::fgs_f(const cv::Mat & sparse, const cv::Mat& mask, const cv::Rect& roi, const float& lambda,
					cv::Mat& dense, cv::Mat& conf)
{
	cv::Mat matSparse, matMask;
	cv::Mat sparse_roi, mask_roi;
	this->m_fgs_filter_->filter(sparse(roi), matSparse);
	this->m_fgs_filter_->filter(mask(roi), matMask);
	dense(roi) = matSparse / matMask;
	conf(roi) = matMask * lambda * 10;
	conf.setTo(1.0, conf > 1.0);
}

/**
 * @brief Sobel Edge detection
 * 
 * @param src: source image 
 * @param tapsize_sobel: tap size 
 * @return cv::Mat: edge image 
 */
inline cv::Mat getSobelAbs(const cv::Mat& src, int tapsize_sobel)
{
	cv::Mat h_sobels, v_sobels;
	cv::Sobel(src, h_sobels, CV_32F, 1, 0, tapsize_sobel);
	cv::Sobel(src, v_sobels, CV_32F, 0, 1, tapsize_sobel);
	cv::Mat dst = (abs(h_sobels) + abs(v_sobels));
	return dst;
}

/**
 * @brief mark upsampling valid rect (square)
 * 
 * @param img: valid map 
 * @param u: x-axis position
 * @param v: y-axis position 
 * @param r: range 
 */
inline void mark_block(cv::Mat& img, int u, int v, int r)
{
	int start_u = u-r;
	int start_v = v-r;
	int end_u = u + r;
	int end_v = v + r;
	start_u = start_u >= 0 ? start_u : 0;
	start_v = start_v >= 0 ? start_v : 0;
	end_u = end_u < img.cols ? end_u : img.cols;
	end_v = end_v < img.rows ? end_v : img.rows;
	img(cv::Rect(start_u, start_v, end_u-start_u, end_v-start_v)) = 1.0;
}

void upsampling::flood_depth_proc_without_edge(const cv::Mat& pc_flood)
{
	// make depthmap
	int width = this->m_guide_width_;
	int height = this->m_guide_height_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	int minX = this->m_guide_width_, minY = this->m_guide_height_, maxX = 0, maxY = 0;
	int r = this->m_range_flood_;
	cv::Mat flood_dmap = this->m_flood_dmap_;
	cv::Mat flood_mask = this->m_flood_mask_;
	cv::Mat flood_range = this->m_flood_range_;
	pc_flood.forEach<cv::Vec3f>([&flood_dmap, &flood_mask, &flood_range, &minX, &minY, &maxX, &maxY, 
									r, cx, cy, fx, fy, width, height]
							(cv::Vec3f& p, const int * pos) -> void {
		float z = p[2];
		if (std::isnan(z) || z == 0.0) {

		} else {
			float x = p[0];
			float y = p[1];
			float uf = (x * fx / z) + cx;
			float vf = (y * fy / z) + cy;
			int u = static_cast<int>(std::round(uf));
			int v = static_cast<int>(std::round(vf));
			if (u >= 0 && u < width && v >= 0 && v < height) {
				flood_dmap.at<float>(v, u) = z;
				flood_mask.at<float>(v, u) = 1.0;
				mark_block(flood_range, u, v, r);
			}	
		}
	});
}

void upsampling::flood_depth_proc_with_edge(const cv::Mat& pc_flood)
{
	int width = this->m_guide_width_;
	int height = this->m_guide_height_;
	cv::Mat flood_dmap = this->m_flood_dmap_;
	cv::Mat flood_mask = this->m_flood_mask_;
	cv::Mat flood_range = this->m_flood_range_;
	int r = this->m_range_flood_;
	// u map
	cv::Mat u_map=cv::Mat::zeros(pc_flood.size(), CV_32FC1);
	cv::Mat pc_cpy;
	pc_flood.copyTo(pc_cpy);
    for (int y = 0; y < this->m_grid_height_; ++y) {
		// first pixel 
		int x = 0;
		float z = pc_cpy.at<cv::Vec3f>(y, x)[2];
		float uf = pc_cpy.at<cv::Vec3f>(y, x)[0] * this->m_fx_ / z + this->m_cx_;
		float vf = pc_cpy.at<cv::Vec3f>(y, x)[1] * this->m_fy_ / z + this->m_cy_;
		int u = static_cast<int>(std::round(uf));
		int v = static_cast<int>(std::round(vf));
		if (u >= 0 && u < width && v >= 0 && v < height) {
			flood_dmap.at<float>(v, u) = z;
			flood_mask.at<float>(v, u) = 1.0;
			mark_block(flood_range, u, v, r);
		}
		float z_left = z;
		float u_left = uf;	
		// loop to line end
        for (x = 1; x < this->m_grid_width_; ++x) {
            z = pc_flood.at<cv::Vec3f>(y, x)[2];
            uf = pc_flood.at<cv::Vec3f>(y, x)[0] * this->m_fx_ / z + this->m_cx_;
            vf = pc_flood.at<cv::Vec3f>(y, x)[1] * this->m_fy_ / z + this->m_cy_;
			float z_diff; // z difference
			float z_diff_per; // z difference percentage
			float u_diff; // u difference
			if (!isnan(uf) && isnan(u_left)) { // not occlusion
                u_diff = this->m_occlusion_thresh_ + 1;
            } else {
				u_diff = uf - u_left;
			}
			z_diff_per = FLT_EPSILON;
			if (!isnan(z)) {
				if (!isnan(z_left)) {
					z_diff = abs(z - z_left);
					z_diff_per = z_diff/z;//mm
				}
			}
			// justification
			if (u_diff < this->m_occlusion_thresh_ || z_diff_per > this->m_z_continuous_thresh_) { //remove
				;
			} else { // converto depthmap
				int u = static_cast<int>(std::round(uf));
				int v = static_cast<int>(std::round(vf));
				if (u >= 0 && u < width && v >= 0 && v < height) {
					flood_dmap.at<float>(v, u) = z;
					flood_mask.at<float>(v, u) = 1.0;
					mark_block(flood_range, u, v, r);
				}
			}
			z_left = z;
			u_left = uf;
        }
    }
}

void upsampling::flood_depth_proc(const cv::Mat& pc_flood)
{
	if (this->m_depth_edge_proc_on_) {
		this->flood_depth_proc_with_edge(pc_flood);
	} else {
		this->flood_depth_proc_without_edge(pc_flood);
	}
}

#if 0
void upsampling::flood_depth_proc(const cv::Mat& pc_flood)
{
	// create uv map
	cv::Mat u_map=cv::Mat::zeros(pc_flood.size(), CV_32FC1);
    for (int y = 0; y < this->m_grid_height_; ++y) {
        for (int x = 0; x < this->m_grid_width_; ++x) {
            float z = pc_flood.at<cv::Vec3f>(y, x)[2];
            float uf = pc_flood.at<cv::Vec3f>(y, x)[0] * this->m_fx_ / z + this->m_cx_;
            u_map.at<float>(y, x) = uf;
        }
    }
	// create occlusion map
	cv::Mat occ_map = cv::Mat::zeros(pc_flood.size(), CV_32FC1);
    cv::Mat z_continuous_map = cv::Mat::zeros(pc_flood.size(), CV_32FC1);
    for (int y = 0; y < this->m_grid_height_; ++y) {
        for (int x = 1; x < this->m_grid_width_; ++x) {
            // occlusion
            float u = u_map.at<float>(y, x);
            float u_left = u_map.at<float>(y, x-1);
            occ_map.at<float>(y, x) = u - u_left;
            // 隣がNaNの場合はthreshより大きな値いれておく
            if (!isnan(u) && isnan(u_left)) {
                occ_map.at<float>(y, x) = this->m_occlusion_thresh_ + 1;
            }
            //continuous
            float z = pc_flood.at<cv::Vec3f>(y, x)[2];
            float z_left = pc_flood.at<cv::Vec3f>(y, x - 1)[2];
            // zが連続的かジャンプしているか zに対する割合で確認
            if (!isnan(z) && !isnan(z_left)) {
                float diff = abs(z - z_left);
                z_continuous_map.at<float>(y, x) = diff/z;//mm
            }
            // 左隣がNaNの場合は最小値いれておく
            if (!isnan(z) && isnan(z_left)) {
                z_continuous_map.at<float>(y, x) = FLT_EPSILON;
            }
        }
    }
    cv::Mat oc_mask = cv::Mat::zeros(pc_flood.size(), CV_8UC1);
    oc_mask.setTo(1, occ_map > 0);
    oc_mask.setTo(0, occ_map < this->m_occlusion_thresh_);
    cv::Mat continu_mask = cv::Mat::zeros(pc_flood.size(), CV_8UC1);
    continu_mask.setTo(1, z_continuous_map > 0);
    continu_mask.setTo(0, z_continuous_map > this->m_z_continuous_thresh_);
    cv::Mat depth_filtered_mask;
    cv::bitwise_or(oc_mask, continu_mask, depth_filtered_mask);	


	cv::Mat pc_flood_cpy;
	pc_flood.copyTo(pc_flood_cpy);
	cv::Mat dpeth_filtered_mask_f = cv::Mat::zeros(pc_flood.size(), CV_32FC3);
	dpeth_filtered_mask_f.setTo((1.0, 1.0, 1.0), depth_filtered_mask > 0);
	cv::multiply(pc_flood_cpy, dpeth_filtered_mask_f, pc_flood_cpy);

	int width = this->m_guide_width_;
	int height = this->m_guide_height_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	int minX = this->m_guide_width_, minY = this->m_guide_height_, maxX = 0, maxY = 0;
	int r = this->m_range_flood_;
	cv::Mat flood_dmap = this->m_flood_dmap_;
	cv::Mat flood_mask = this->m_flood_mask_;
	cv::Mat flood_range = this->m_flood_range_;
	pc_flood_cpy.forEach<cv::Vec3f>([&flood_dmap, &flood_mask, &flood_range, &minX, &minY, &maxX, &maxY, 
									r, cx, cy, fx, fy, width, height]
							(cv::Vec3f& p, const int * pos) -> void {
		float z = p[2];
		if (std::isnan(z) || z == 0.0) {

		} else {
			float x = p[0];
			float y = p[1];
			float uf = (x * fx / z) + cx;
			float vf = (y * fy / z) + cy;
			int u = static_cast<int>(std::round(uf));
			int v = static_cast<int>(std::round(vf));
			if (u >= 0 && u < width && v >= 0 && v < height) {
				flood_dmap.at<float>(v, u) = z;
				flood_mask.at<float>(v, u) = 1.0;
				mark_block(flood_range, u, v, r);
				if (u < minX) minX = u;
				if (u > maxX) maxX = u;
				if (v < minY) minY = v;
				if (v > maxY) maxY = v;
			}	
		}
	});
}
#endif


/**
 * @brief create FGS filter
 * 
 * @param guide: guide image 
 */
void upsampling::flood_guide_proc2(const cv::Mat& guide)
{
	cv::Rect roi = this->m_flood_roi_;
	this->m_fgs_filter_ = cv::ximgproc::createFastGlobalSmootherFilter(guide(roi), 
							(double)this->m_fgs_lambda_flood_, (double)this->m_fgs_sigma_color_flood_, 
							(double)this->m_fgs_lambda_attenuation_, (double)this->m_fgs_num_iter_flood_);
}

/**
 * @brief guide image processing for flood
 * 
 * @param guide: guide image 
 */
void upsampling::flood_guide_proc(const cv::Mat& guide)
{
	if (this->m_guide_edge_proc_on_) {
		// guide edge
		cv::Mat imgEdgeGuide, imgBoarder;
		cv::Canny(guide, imgEdgeGuide, this->m_canny_thresh1_, this->m_canny_thresh2_, 3);
		cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, 
			cv::Size(this->m_guide_edge_dilate_size_, this->m_guide_edge_dilate_size_));
		cv::dilate(imgEdgeGuide, imgBoarder, kernel);
		this->m_guide_edge_.setTo(0.0, imgBoarder == 255);
	}
}

/**
 * @brief preprocessing for flood
 * 
 * @param img_guide: input guide image
 * @param pc_flood: input flood pointcloud 
 */
void upsampling::flood_preprocessing(const cv::Mat& img_guide, const cv::Mat& pc_flood)
{
	std::thread th3([this, img_guide]()->void{
		this->flood_guide_proc2(img_guide); //create FGS filter
	});
	std::thread th1([this, pc_flood]()->void{
		this->flood_depth_proc(pc_flood); //depth edge processing and convert to depthmap
	});
	std::thread th2([this, img_guide]()->void{
		this->flood_guide_proc(img_guide); //guide image processing
	});
	th1.join();
	th2.join();
	th3.join();
	if (this->m_guide_edge_proc_on_) {
		// filtering depthmap by guide edge
		cv::multiply(this->m_flood_dmap_, this->m_guide_edge_, this->m_flood_dmap_);
		cv::multiply(this->m_flood_mask_, this->m_guide_edge_, this->m_flood_mask_);
	}
}

/**
 * @brief create FGS Filter for spot
 * 
 * @param guide: guide image 
 */
void upsampling::spot_guide_proc(const cv::Mat& guide)
{
	cv::Rect roi = this->m_spot_roi_;
	this->m_fgs_filter_ = cv::ximgproc::createFastGlobalSmootherFilter(guide(roi), 
							(double)this->m_fgs_lambda_spot_, (double)this->m_fgs_sigma_color_spot_, 
							(double)this->m_fgs_lambda_attenuation_, (double)this->m_fgs_num_iter_spot_);

}

/**
 * @brief depth processing for spot
 * 
 * @param pc_spot point cloud of spot 
 */
void upsampling::spot_depth_proc(const cv::Mat& pc_spot)
{
	cv::Mat dmap = this->m_spot_dmap_;
	cv::Mat mask = this->m_spot_mask_;
	int width = this->m_guide_width_;
	int height = this->m_guide_height_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;
	float fx = this->m_fx_;
	float fy = this->m_fy_;

	pc_spot.forEach<cv::Vec3f>([&dmap, &mask, width, height, cx, cy, fx, fy]
								(cv::Vec3f& p, const int* pos) -> void{
		float z = p[2];
		float uf = (p[0] * fx / z) + cx;
		float vf = (p[1] * fy / z) + cy;
		int u = static_cast<int>(std::round(uf));
		int v = static_cast<int>(std::round(vf));
		if (u >= 0 && u < width && v >= 0 && v < height) {
			dmap.at<float>(v, u) = z;
			mask.at<float>(v, u) = 1.0;
		}
	});	
}

/**
 * @brief preprocessing for spot
 * 
 * @param guide: guide image 
 * @param pc_spot: point cloud of spot 
 */
void upsampling::spot_preprocessing(const cv::Mat& guide, const cv::Mat& pc_spot)
{
	std::thread th1([this, guide]()->void{
		this->spot_guide_proc(guide); //create FGS filter
	});
	std::thread th2([this, pc_spot]()->void{
		this->spot_depth_proc(pc_spot);
	});
	th1.join();
	th2.join();
}

/**
 * @brief full processing for flood 
 * 
 * @param img_guide: image guide 
 * @param pc_flood: point cloud of flood 
 * @param dense: output dense depthmap 
 * @param conf: output confidence 
 */
void upsampling::run_flood(const cv::Mat& img_guide, const cv::Mat& pc_flood, cv::Mat& dense, cv::Mat& conf)
{
#ifdef SHOW_TIME
	std::chrono::system_clock::time_point t_start, t_end;
	double elapsed;
	t_start = std::chrono::system_clock::now();
#endif
	// preprocessing
	this->flood_preprocessing(img_guide, pc_flood);
#ifdef SHOW_TIME
	t_end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
	std::cout << "xyz2depthmap time = " << elapsed << " [us]" << std::endl;
#endif

	// upsampling
#ifdef SHOW_TIME
	t_start = std::chrono::system_clock::now();
#endif
	// cv::Rect roi(0, 0, this->guide_width, this->guide_height);
	this->fgs_f(this->m_flood_dmap_, this->m_flood_mask_, this->m_flood_roi_, this->m_fgs_lambda_flood_,
				dense, conf);
#ifdef SHOW_TIME
	t_end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
	std::cout << "FGS processing time = " << elapsed << " [us]" << std::endl;
#endif
	// fill invalid regions
	dense.setTo(std::nan(""), this->m_flood_range_ == 0.0);
	conf.setTo(std::nan(""), this->m_flood_range_ == 0.0);
}

/**
 * @brief full processing for spot
 * 
 * @param img_guide: image guide 
 * @param pc_spot: point cloud of spot 
 * @param dense: output dense depthmap 
 * @param conf: output confidence 
 */
void upsampling::run_spot(const cv::Mat& img_guide, const cv::Mat& pc_spot, cv::Mat& dense, cv::Mat& conf)
{
#ifdef SHOW_TIME
	std::chrono::system_clock::time_point t_start, t_end;
	double elapsed;
	t_start = std::chrono::system_clock::now();
#endif
	this->spot_preprocessing(img_guide, pc_spot);

#ifdef SHOW_TIME
	t_end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
	std::cout << "spot preprocessing time = " << elapsed << " [ms]" << std::endl;
#endif

#ifdef SHOW_TIME
	t_start = std::chrono::system_clock::now();
#endif
	// upsampling
	fgs_f(this->m_spot_dmap_, this->m_spot_mask_, this->m_spot_roi_, this->m_fgs_lambda_spot_, 
		dense, conf);
#ifdef SHOW_TIME
		t_end = std::chrono::system_clock::now();
		elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
		std::cout << "FGS spot processing time = " << elapsed << " [ms]" << std::endl;
#endif
#if 0 // skipped for full region results
	// fill invalid regions
	dense.setTo(std::nan(""), this->m_spot_range == 0.0);
	conf.setTo(std::nan(""), this->m_spot_range == 0.0);
#endif
}

/**
 * @brief Upsampling main processing
 * 
 * @param img_guide : guide image 
 * @param pc_flood : flood point cloud 
 * @param pc_spot : spot point cloud 
 * @param dense : upsampling result dense depthmap 
 * @param conf : confidence map 
 * @return true 
 * @return false 
 */
bool upsampling::run(const cv::Mat& img_guide, const cv::Mat& pc_flood, const cv::Mat& pc_spot, 
						cv::Mat& dense, cv::Mat& conf)
{
#ifdef SHOW_TIME
	std::chrono::system_clock::time_point t_start, t_end;
	double elapsed;
#endif
	if (img_guide.empty()) // no guide
		return false;
	this->initialization(dense, conf);
	// set mode
	m_mode_ = 0;
	if (!pc_flood.empty()) { // flood
		m_mode_ = 1;
	}
	if (!pc_spot.empty()) { // spot
		if(m_mode_ == 1)
			m_mode_ = 3;
		else
			m_mode_ = 2;
	}

	if (m_mode_ == 0) { // invalid
		dense.setTo(std::nan(""));
		conf.setTo(std::nan(""));
		return false;
	}
	if (m_mode_ == 1) { // flood only
		this->run_flood(img_guide, pc_flood, dense, conf);
		return true;
	}
	if (m_mode_ == 2) { // spot only
		this->run_spot(img_guide, pc_spot, dense, conf);
		return true;
	}
	if (m_mode_ == 3) { // flood + spot
		cv::Mat denseSpot = cv::Mat::zeros(dense.size(), dense.type());
		cv::Mat confSpot = cv::Mat::zeros(conf.size(), conf.type());
		this->run_flood(img_guide, pc_flood, dense, conf);
		this->run_spot(img_guide, pc_spot, denseSpot, confSpot);
		// merge
		denseSpot.copyTo(dense, this->m_flood_range_ == 0);
		confSpot.copyTo(conf, this->m_spot_range_ == 0);
		return true;
	}
	return false;
}

/**
 * @brief convert depthmap to point cloud
 * 
 * @param depth : input depthmap 
 * @param pc : output point cloud 
 */
void upsampling::depth2pc(const cv::Mat& depth, cv::Mat& pc)
{
	pc.create(depth.size(), CV_32FC3);
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;

	for (int y = 0; y < depth.rows; ++y) {
		for (int x = 0; x < depth.cols; ++x) {
			float z = depth.at<float>(y, x);
			pc.at<cv::Vec3f>(y, x)[0] = (x - cx) * z / fx;
			pc.at<cv::Vec3f>(y, x)[1] = (y - cy) * z / fy;
			pc.at<cv::Vec3f>(y, x)[2] = z;
		}
	}
}