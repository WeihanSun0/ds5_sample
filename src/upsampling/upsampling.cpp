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
	this->m_range_flood_ = params.range_flood;
	this->m_z_continuous_thresh_ = params.z_continuous_thresh;
	this->m_occlusion_thresh_ = params.occlusion_thresh;
	this->m_depth_diff_thresh_ = params.depth_diff_thresh;
	this->m_guide_diff_thresh_ = params.guide_diff_thresh;
	this->m_min_diff_count= params.min_diff_count;
	if (this->m_z_continuous_thresh_ == 1.0 && this->m_occlusion_thresh_ == 0.0)
		this->m_depth_edge_proc_on_ = false;
	else
		this->m_depth_edge_proc_on_ = true;
};

/**
 * @brief get default preprocessing parameters
 * 
 * @param params preprocessing paramters structure
 */
void upsampling::get_default_preprocessing_parameters(Preprocessing_Params& params)
{
	params.range_flood = this->m_range_flood_;
	params.occlusion_thresh = this->m_occlusion_thresh_;
	params.z_continuous_thresh = this->m_z_continuous_thresh_;
	params.depth_diff_thresh = this->m_depth_diff_thresh_;
	params.guide_diff_thresh = this->m_guide_diff_thresh_;
	params.min_diff_count = this->m_min_diff_count;
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
 * @param dense : dense depthmap 
 * @param conf : confidence depthmap
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

/**
 * @brief depth preproocessing without edge processing
 * 
 * @param pc_flood 
 */
void upsampling::flood_depth_proc_without_edge(const cv::Mat& pc_flood)
{
	this->pc2flood_dmap(pc_flood);
}

/**
 * @brief Get the rect average value 
 * 
 * @param img : image
 * @param cx : rect center x
 * @param cy : rect center yt
 * @param r : rect half boarder
 * @return float : average value
 */
inline float get_rect_avg_val(const cv::Mat & img, int cx, int cy, int r)
{
	float sumV = 0.0f;
	int count = 0;
	for (int j = -r; j < r+1; ++j) {
		for (int i = -r; i < r+1; ++i) {
			int x = cx + i;
			int y = cy + j;
			if (x < 0 || x >= img.cols || y < 0 || y >= img.rows) 
				sumV += 255.0;
			else
				sumV += img.at<uchar>(y, x);
			count +=1;
		}
	}
	return sumV/count;
}

/**
 * @brief filtering parallax devation error points
 * 
 * @param pc_in : point cloud input
 * @param pc_out : point cloud output
 */
void upsampling::filter_parallax_devation_points(const cv::Mat& pc_in, cv::Mat& pc_out)
{
	int width = this->m_guide_width_;
	int height = this->m_guide_height_;
	cv::Mat flood_dmap = this->m_flood_dmap_; // final output
	cv::Mat flood_mask = this->m_flood_mask_; // final output
	cv::Mat flood_range = this->m_flood_range_; // final output
	int range = this->m_range_flood_;
	float inval = 100.0f;
	// copy one for check error
	pc_out = cv::Mat::ones(pc_in.size(), CV_32FC3) * inval;
	/* cv::Mat flood_edge_dmap = this->m_flood_edge_dmap_; // for mark edge points */
	
	// declaration
	float z, uf, vf;
	float z_left, u_left; // save left u, z
	float z_diff, u_diff, z_diff_per; // difference 
	int u, v;
	for (int y = 0; y < this->m_grid_height_; ++y) {
		z_left = static_cast<float>(std::nan(""));
		u_left = -1;
		for (int x = 0; x < this->m_grid_width_; ++x) {
			z = pc_in.at<cv::Vec3f>(y, x)[2];
			if (isnan(z)) { //remove 
				continue;
			}
			uf = pc_in.at<cv::Vec3f>(y, x)[0] * this->m_fx_ / z + this->m_cx_;
			vf = pc_in.at<cv::Vec3f>(y, x)[1] * this->m_fy_ / z + this->m_cy_;
			u = static_cast<int>(std::round(uf));
			v = static_cast<int>(std::round(vf));
			if (u >= 0 && u < width && v >= 0 && v < height) { 
				if (isnan(z_left)) { // new left
					z_left = z;
					u_left = uf;
				} else {
					z_diff = abs(z - z_left);
					z_diff_per = z_diff / z;
					u_diff = uf - u_left;
					if (u_diff < this->m_occlusion_thresh_ && z_diff_per > this->m_z_continuous_thresh_) {
						//remove
						continue;
					} else {
						if (u_left < uf) {
							u_left = uf;
							z_left = z;
						}
					}
				}
				//* insert for output
				pc_out.at<cv::Vec3f>(y, x) = pc_in.at<cv::Vec3f>(y, x);
			}
		}
	}
}

/**
 * @brief convert flood point cloud to depthmap 
 * 
 * @param pc : point cloud input
 */
void upsampling::pc2flood_dmap(const cv::Mat& pc)
{
	float cx = this->m_cx_;
	float cy = this->m_cy_;
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	cv::Mat dmap = this->m_flood_dmap_;
	cv::Mat mask = this->m_flood_mask_;
	cv::Mat range =  this->m_flood_range_;	
	float inval = 100.0f;
	for (int j = 0; j < pc.rows; ++j) {
		for (int i = 0; i < pc.cols; ++i) {
			cv::Vec3f vals = pc.at<cv::Vec3f>(j, i);
			float x = vals[0];
			float y =  vals[1];
			float z = vals[2];
			if (z == inval) continue;
			int u = static_cast<int>(round(x * fx / z + cx));
			int v = static_cast<int>(round(y * fy / z + cy));
			if (u >= 0 && u < this->m_guide_width_ && v >=0 && v < this->m_guide_height_) {
				dmap.at<float>(v, u) = z;
				mask.at<float>(v, u) = 1.0;
				mark_block(range, u, v, this->m_range_flood_);
			}
		}
	}
}

/**
 * @brief error edge points detection based on the guide image 
 * 
 * @param guide : guide image
 * @param z_map : flood z map (80 x 60)
 * @param edge_mask : input mask for edge points
 * @param err_mask1 : input mask for error points last iteration
 * @param err_mask2 : output mask for error points this iteration
 * @param num_neigbors : number of neigbors are taken as reference points. 8 or 24 now
 * @param region_size : region size of guide is applied for average value calculation
 * @param depth_thresh : threshold of depth to justify different or not
 * @param guide_thresh : threshold of guide to justify different or not
 * @param min_diff_count : minimum different count for not an error
 */
inline void error_points_detection(const cv::Mat& guide, const cv::Mat& z_map, const cv::Mat& edge_mask,
									cv::Mat& err_mask1, cv::Mat& err_mask2, int num_neigbors, int region_size = 1,
									float depth_thresh = 0.1f, float guide_thresh = 40.0f, int min_diff_count = -1)
{
	int delta;
	const float inval = 100.0f;
	if (num_neigbors == 8) { // stage 1: absolute error
		delta = 1;
	} else { // stage 2: relative error
		delta = 2;
	}
	float z, z_ref, val, val_ref;
	int u, v, u_ref, v_ref;
	bool is_depth_diff, is_guide_diff;
	for (int r = 1; r < z_map.rows-1; ++r) {
        for (int c = 1; c < z_map.cols-1; ++c) {
			if (edge_mask.at<uchar>(r, c) == 0) { //* not edge
				continue;
			}
			if (err_mask1.at<uchar>(r, c) == 1) { //* already masked as errors
				err_mask2.at<uchar>(r, c) = 1;
				continue;
			}
			// * comparation of guide and depth 
			cv::Vec3f pnt = z_map.at<cv::Vec3f>(r, c);
			u = static_cast<int>(pnt[0]);
			v = static_cast<int>(pnt[1]);
			z = pnt[2];
			val = get_rect_avg_val(guide, u, v, region_size);
			int count_diff = 0;
			for (int j = -delta; j <= delta; ++j) { //* match local feature
				for (int i = -delta; i <= delta; ++i) {
					if (i == 0 && j == 0) // ref = org
						continue;
					if (err_mask1.at<uchar>(r+j, c+i) == 1) { // ref is err
						continue;
					}
					is_depth_diff = false;
					is_guide_diff = false;	
					cv::Vec3f pnt_ref = z_map.at<cv::Vec3f>(r+j, c+i);
					z_ref = pnt_ref[2];
					if (z_ref == inval) {
						u_ref = u + 12*i;
						v_ref = v + 12*j;
					} else {
						u_ref = static_cast<int>(pnt_ref[0]);
						v_ref = static_cast<int>(pnt_ref[1]);
					}
					val_ref = get_rect_avg_val(guide, u_ref, v_ref, region_size);
					if (z_ref == inval || fabs(z_ref - z) > depth_thresh) {
						is_depth_diff = true;	
					}
					if (fabs(val_ref -val) > guide_thresh)	{
						is_guide_diff = true;
					}
					if (is_guide_diff^is_depth_diff) { //* different
						count_diff += 1;
					} else {
						count_diff -= 1 ;
					}
				}
			}
			if (count_diff > min_diff_count) { // error
				err_mask2.at<uchar>(r, c) = 1;
			}
		}
	}
}

/**
 * @brief extract depth edge
 * 
 * @param z_map : input z map
 * @param edge_mask : output edge point mask
 */
void upsampling::extract_depth_edge(const cv::Mat& z_map, cv::Mat& edge_mask)
{
	float inval = 100.0;
	float depth_thresh = this->m_depth_diff_thresh_;
	int num_diff_for_edge = 0;
	if (edge_mask.empty()) 
		edge_mask = cv::Mat::zeros(z_map.size(), CV_8UC1);
	for (int r = 1; r < z_map.rows-1; ++r) {
		for (int c = 1; c < z_map.cols-1; ++c) {
			if (z_map.at<cv::Vec3f>(r, c)[2] == inval)
				continue;
			float fGrad = 0.0f;
			int count = 0;
			fGrad = fabs(z_map.at<cv::Vec3f>(r, c-1)[2] - z_map.at<cv::Vec3f>(r, c+1)[2]);
			if (fGrad > depth_thresh)
				count += 1;
			fGrad = fabs(z_map.at<cv::Vec3f>(r-1, c)[2] - z_map.at<cv::Vec3f>(r+1, c)[2]);
			if (fGrad > depth_thresh)
				count += 1;
			fGrad = fabs(z_map.at<cv::Vec3f>(r-1, c-1)[2] - z_map.at<cv::Vec3f>(r+1, c+1)[2]);
			if (fGrad > depth_thresh) 
				count += 1;
			fGrad = fabs(z_map.at<cv::Vec3f>(r-1, c+1)[2] - z_map.at<cv::Vec3f>(r+1, c-1)[2]);
			if (fGrad > depth_thresh) 
				count += 1;
			if (count > num_diff_for_edge) { // * edge point
				edge_mask.at<uchar>(r, c) = 1;
			}
		}
	}
}

/**
 * @brief filter error edge points
 * 
 * @param img_guide : guide image
 * @param pc_in : input point cloud
 * @param pc_out : output point cloud
 */
void upsampling::filter_error_edge_points(const cv::Mat& img_guide, const cv::Mat& pc_in, cv::Mat& pc_out)
{
#define USE_REG_AVG 1
#ifdef USE_REG_AVG
	int region_size = 1;
#endif
	float depth_thresh = this->m_depth_diff_thresh_;
	float guide_thresh = this->m_guide_diff_thresh_;
	float inval = 100.0f;
	int num_diff_for_edge = 0;
	int min_diff_num = this->m_min_diff_count;
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;
	int guide_width = this->m_guide_width_;
	int guide_height = this->m_guide_height_;
	pc_in.copyTo(pc_out);
	cv::Mat z_map = cv::Mat::ones(pc_in.size(), CV_32FC3) * inval; //* (u, v, z) map
	cv::Mat edge_mask = cv::Mat::zeros(pc_in.size(), CV_8UC1); // * edge point mask
	cv::Mat err_mask0 = cv::Mat::zeros(pc_in.size(), CV_8UC1); // * error mask of stage 0
	cv::Mat err_mask1 = cv::Mat::zeros(pc_in.size(), CV_8UC1); // * error mask of stage 1
	cv::Mat err_mask2 = cv::Mat::zeros(pc_in.size(), CV_8UC1); // * error mask of stage 2
	cv::Mat err_mask3 = cv::Mat::zeros(pc_in.size(), CV_8UC1); // * error mask of stage 2
	//* create z map
	pc_in.forEach<cv::Vec3f>([&z_map, fx, fy, cx, cy, guide_width, guide_height](cv::Vec3f& pixel, const int pos[]) -> void {
        int c = pos[1];
        int r = pos[0];
		float x = pixel[0];
		float y = pixel[1];
        float z = pixel[2];
		float u = std::round((x * fx)/z + cx);
		float v = std::round((y * fy)/z + cy);
		if (u >= 0 && u < guide_width && v >= 0 && v < guide_height) // not use points outside image
			z_map.at<cv::Vec3f>(r, c) = cv::Vec3f(u, v, z);
    });
	//* stage 0: edge detection 
	this->extract_depth_edge(z_map, edge_mask);

	// * stage 1: absolute error detection
	error_points_detection(img_guide, z_map, edge_mask, err_mask0, err_mask1, 8, 1, 
							depth_thresh, guide_thresh, 0);
	// * stage 2: relative error detection
	error_points_detection(img_guide, z_map, edge_mask, err_mask1, err_mask2, 24, 1,
							depth_thresh, guide_thresh, min_diff_num - 24);
	// * filtered error points
	cv::Mat mask = err_mask2;
	pc_in.forEach<cv::Vec3f>([&pc_out, &mask](cv::Vec3f& p, const int pos[]) -> void{
		int c = pos[1];
		int r = pos[0];
		if (mask.at<uchar>(r, c) == 1) { // remove
			pc_out.at<cv::Vec3f>(r, c) = cv::Vec3f(static_cast<float>(std::nan("")), static_cast<float>(std::nan("")), static_cast<float>(std::nan("")));
		}
	});
}

/**
 * @brief flood depth preprocessing with edge
 * 
 * @param pc_flood : input flood point cloud
 * @param img_guide : input guide image
 */
void upsampling::flood_depth_proc_with_edge(const cv::Mat& pc_flood, const cv::Mat& img_guide)
{
	cv::Mat pc_filtered_parallax, pc_filtered_edge_err;
	this->filter_parallax_devation_points(pc_flood, pc_filtered_parallax);
	if (this->m_depth_diff_thresh_ == 0.0f || this->m_guide_diff_thresh_ == 0.0f) { // no edge error filtering
		this->pc2flood_dmap(pc_filtered_parallax);
		return;
	}
	this->filter_error_edge_points(img_guide, pc_filtered_parallax, pc_filtered_edge_err);
	this->pc2flood_dmap(pc_filtered_edge_err);
}


/**
 * @brief depth preprocessing
 * 
 * @param pc_flood : flood point cloud
 * @param img_guide : guide image
 */
void upsampling::flood_depth_proc(const cv::Mat& pc_flood, const cv::Mat& img_guide)
{
	if (this->m_use_preprocessing) {
		this->flood_depth_proc_with_edge(pc_flood, img_guide);
	} else {
		this->flood_depth_proc_without_edge(pc_flood);
	}
}


/**
 * @brief create FGS filter
 * 
 * @param guide: guide image 
 */
void upsampling::flood_guide_proc(const cv::Mat& guide)
{
	cv::Rect roi = this->m_flood_roi_;
	this->m_fgs_filter_ = cv::ximgproc::createFastGlobalSmootherFilter(guide(roi), 
							(double)this->m_fgs_lambda_flood_, (double)this->m_fgs_sigma_color_flood_, 
							(double)this->m_fgs_lambda_attenuation_, this->m_fgs_num_iter_flood_);
}

/**
 * @brief preprocessing for flood
 * 
 * @param img_guide: input guide image
 * @param pc_flood: input flood pointcloud 
 */
void upsampling::flood_preprocessing(const cv::Mat& img_guide, const cv::Mat& pc_flood)
{
	/* img_guide.copyTo(this->m_guide); */
	std::thread th3([this, img_guide]()->void{
		this->flood_guide_proc(img_guide); //create FGS filter
	});
	std::thread th1([this, pc_flood, img_guide]()->void {
		this->flood_depth_proc(pc_flood, img_guide); //depth edge processing and convert to depthmap 
	});
	th1.join();
	th3.join();
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
							(double)this->m_fgs_lambda_attenuation_, this->m_fgs_num_iter_spot_);

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

/**
 * @brief point cloud to depthmap
 * 
 * @param pc : input point cloud
 * @param depth : output depth image
 */
void upsampling::pc2depthmap(const cv::Mat& pc, cv::Mat& depth)
{
	depth.create(cv::Size(this->m_guide_width_, this->m_guide_height_), CV_32FC1);
	float fx = this->m_fx_;
	float fy = this->m_fy_;
	float cx = this->m_cx_;
	float cy = this->m_cy_;

	for (int r = 0; r < pc.rows; ++r) {
		for (int c = 0; c < pc.cols; ++c) {
			float x = pc.at<cv::Vec3f>(r, c)[0];
			float y = pc.at<cv::Vec3f>(r, c)[1];
			float z = pc.at<cv::Vec3f>(r, c)[2];
			if (z == 0.0) 
				continue;
			float u = (x * fx)/z + cx;
			float v = (y * fy)/z + cy;
			int iu = static_cast<int>(round(u));
			int iv = static_cast<int>(round(v));
			if (iu >= 0 && iu < this->m_guide_width_ && iv >= 0 && iv < this->m_guide_height_) {
				depth.at<float>(iv, iu) = z;
			}
		}
	}
}

/**
 * @brief filter dense depthmap by confidence
 * 
 * @param dense : input dense depthmap
 * @param conf : confidence map
 * @param filtered : output filtered depthmap
 * @param threshold : threshold
 */
void upsampling::filter_by_confidence(const cv::Mat& dense, const cv::Mat& conf, cv::Mat& filtered, float threshold)
{
	dense.copyTo(filtered);
	filtered.setTo(std::nan(""), conf < threshold);
}