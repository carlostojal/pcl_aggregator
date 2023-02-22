/*
* Copyright (c) 2023 Carlos Tojal.
* All rights reserved.
*
* Manage the different PointCloud managers and lists, keeping the latest and removing the ones
* older than the max defined age.
*/

#ifndef POINTCLOUDS_MANAGER_H_
#define POINTCLOUDS_MANAGER_H_

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/registration/icp.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/visualization/cloud_viewer.h>
#include <eigen3/Eigen/Dense>
#include "PointCloudList.h"
#include "StreamManager.h"
#include <vector>
#include <memory>

#define FILTER_VOXEL_SIZE 0.1f

// this class manages the stored point clouds
// keep an array of point clouds, the last one for each source
class PointCloudsManager {

	private:
		size_t n_sources;
		time_t max_age;
		// the array of instances below functions almost as a hashtable. details explained on "addCloud"
		std::vector<std::shared_ptr<StreamManager>> cloudManagers; // array of point cloud managers - fixed size = n_sources
		pcl::PointCloud<pcl::PointXYZRGB>::Ptr mergedCloud = nullptr; // the merged cloud
		bool mergedCloudDownsampled = false; // prevent double downsampling
		void allocCloudManagers();
		void clean(); // remove clouds older than "maxAge"
		static size_t topicNameToIndex(const std::string& topicName);

		bool appendToMerged(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr& input);
		void clearMergedCloud();
		void downsampleMergedCloud();

	
	public:
		PointCloudsManager(size_t n_sources, time_t max_age);
		~PointCloudsManager();
		size_t getNClouds();
		void addCloud(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud, const std::string& topicName);
		void setTransform(const Eigen::Affine3d& transformEigen, const std::string& topicName);

		pcl::PointCloud<pcl::PointXYZRGB> getMergedCloud();
};

#endif
