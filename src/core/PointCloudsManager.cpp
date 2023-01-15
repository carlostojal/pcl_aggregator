#include "PointCloudsManager.h"

PointCloudsManager::PointCloudsManager(size_t n_clouds, time_t max_age) {
		this->n_clouds = n_clouds;
		// allocate the array
		this->allocCloudManagers();

		this->max_age = max_age;
}

PointCloudsManager::~PointCloudsManager() {
	// delete each of the instances first
	#pragma omp parallel for
	for(size_t i = 0; i < this->n_clouds; i++) {
		delete this->cloudManagers[i];
	}
	// free the array
	free(this->cloudManagers);
}

size_t PointCloudsManager::getNClouds() {
	return this->n_clouds;
}

void PointCloudsManager::allocCloudManagers() {
	if((this->cloudManagers = (PCLStamped**) malloc(this->n_clouds * sizeof(PCLStamped*))) == NULL) {
		std::cerr << "Error allocating point cloud managers: " << strerror(errno) << std::endl;
		return;
	}
	// initialize all instances to nullptr
	# pragma omp parallel for
	for(size_t i = 0; i < this->n_clouds; i++) {
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
	for(size_t i = 0; i < this->n_clouds; i++) {
		if(this->cloudManagers[i] != nullptr) {
			if(cur_timestamp - this->cloudManagers[i]->getTimestamp() > max_age) {
				delete this->cloudManagers[i];
				this->cloudManagers[i] = nullptr;
			}
		}
	}
}

void PointCloudsManager::addCloud(pcl::PointCloud<pcl::PointXYZ> *cloud, std::string topicName) {
		// the pointcloud topic names must be "pointcloud0", "pointcloud1", etc.
		// so, we can use the number after "pointcloud" as index on the array
		std::string cloudNumber = topicName.substr(10);
		size_t index = atol(cloudNumber.c_str());

		// set the pointcloud as the latest of this source
		// check if it was ever defined
		if(this->cloudManagers[index] == nullptr) {
			this->cloudManagers[index] = new PCLStamped(cloud);
		} else {
			this->cloudManagers[index]->addCloud(cloud);
		}
}

