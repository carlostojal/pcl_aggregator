/*
* Copyright (c) 2023 Carlos Tojal.
* All rights reserved.
*
* Manage the different PointCloud managers and lists, keeping the latest and removing the ones
* older than the max defined age.
*/

#include "PointCloudsManager.h"

PointCloudsManager::PointCloudsManager(size_t n_sources, time_t max_age) {
		this->n_sources = n_sources;
		// allocate the array
		this->allocCloudManagers();

		this->max_age = max_age;
}

PointCloudsManager::~PointCloudsManager() {
	// delete each of the instances first
	#pragma omp parallel for
	for(size_t i = 0; i < this->n_sources; i++) {
		delete this->cloudManagers[i];
	}
	// free the array
	free(this->cloudManagers);

	// free the list
	delete this->clouds;
}

size_t PointCloudsManager::getNClouds() {
	return this->n_sources;
}

void PointCloudsManager::allocCloudManagers() {
	if((this->cloudManagers = (StreamManager**) malloc(this->n_sources * sizeof(StreamManager*))) == NULL) {
		std::cerr << "Error allocating point cloud managers: " << strerror(errno) << std::endl;
		return;
	}
	// initialize all instances to nullptr
	# pragma omp parallel for
	for(size_t i = 0; i < this->n_sources; i++) {
		this->cloudManagers[i] = nullptr;
	}
}

void PointCloudsManager::clean() {
	// get the current timestamp to calculate pointclouds age
	time_t cur_timestamp;
	if((cur_timestamp = time(NULL)) < 0) {
		std::cerr << "Error getting current timestamp: " << strerror(errno) << std::endl;
		return;
	}
	#pragma omp parallel for
	for(size_t i = 0; i < this->n_sources; i++) {
		if(this->cloudManagers[i] != nullptr) {
			if(cur_timestamp - this->cloudManagers[i]->getTimestamp() > max_age) {
				this->clouds->remove(this->cloudManagers[i]->getCloud()); // remove from the final list
				delete this->cloudManagers[i];
				this->cloudManagers[i] = nullptr;
			}
		}
	}
}

size_t PointCloudsManager::topicNameToIndex(std::string topicName) {
	// the pointcloud topic names must be "pointcloud0", "pointcloud1", etc.
	// so, we can use the number after "pointcloud" as index on the array
	std::string cloudNumber = topicName.substr(10);
	size_t index = atol(cloudNumber.c_str());

	return index;
}

bool PointCloudsManager::align(pcl::PointCloud<pcl::PointXYZ>::Ptr merged, pcl::PointCloud<pcl::PointXYZ>::Ptr input, Eigen::Matrix<float, 4, 4> *transform) {
	// align the pointclouds
	pcl::IterativeClosestPoint<pcl::PointXYZ, pcl::PointXYZ> icp;
	icp.setInputSource(input);
	icp.setInputTarget(merged); // "input" will align to "merged"
	icp.align(*merged); // combine the aligned pointclouds on the "merged" instance

	*transform = icp.getFinalTransformation(); // get the transformation matrix

	if(!icp.hasConverged())
		merged += input; // if alignment was not possible, just add the pointclouds

	return icp.hasConverged(); // return true if alignment was possible
}

void PointCloudsManager::addCloud(pcl::PointCloud<pcl::PointXYZ> *cloud, std::string topicName) {
		

		size_t index = this->topicNameToIndex(topicName);

		// set the pointcloud as the latest of this source
		// check if it was ever defined
		if(this->cloudManagers[index] == nullptr) {
			this->cloudManagers[index] = new StreamManager();
		} else {
			this->cloudManagers[index]->addCloud(cloud);
		}

		// clean the old pointclouds
		// doing this only after insertion avoids instance immediate destruction and recreation upon updating
		this->clean();

		// this point cloud is very recent, so it should be kept by now
		this->clouds->insertOnTail(cloud);
}

void PointCloudsManager::setTransform(Eigen::Affine3d *transformEigen, std::string topicName) {

	size_t index = this->topicNameToIndex(topicName);

	// check if it was ever defined
	if(this->cloudManagers[index] == nullptr) {
		this->cloudManagers[index] = new StreamManager();
	} else {
		this->cloudManagers[index]->setTransform(transformEigen);
	}
}

// this is the filtered raw list returned from the manager
PointCloudList *PointCloudsManager::getClouds() {
	return this->clouds;
}