#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "PointBufferState.hpp"
#include "LASReader.hpp"

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

#define NUM_WORKERS 3

struct data_task_t
{
	char* vector;
	uint64_t num_points;
	uint64_t byte_offset;
	std::atomic<PointBufferState>* state;
};

// Singleton class that manages disk data and import's it to octree nodes
class LoadDataManager
{
private:

	// Instance properties
	// Data loading properties
	std::vector<std::thread> _workers;
	std::queue<data_task_t> _task_queue;
	
	std::mutex _mutex;
	std::condition_variable _cv;
	bool _stop = false;

	std::string _base_path;

	// Avoid creating new instances manually
	LoadDataManager(const std::string& base_path, unsigned int num_workers);	// Make constructor private
	~LoadDataManager();
	LoadDataManager(const LoadDataManager&) = delete;	// Delete copy constructor
	LoadDataManager& operator=(const LoadDataManager&) = delete;	// Delete asignation operator

	void workerLoop();


public:
	// Static method to initialize LoadDataManager singleton instance
	static void Init(const std::string& base_path, unsigned int num_threads)
	{
		static bool instance_created = false;
		if (!instance_created)
		{
			getInstancePtr(&base_path, num_threads);
		}
	}

	// Static method to get LoadDataManager instance
	static LoadDataManager& GetInstance()
	{	
		return getInstancePtr(nullptr, 1);	// Ignore params since won't create new instances
	}

	// Helper to handle LoadDataManager intance securily
    static LoadDataManager& getInstancePtr(const std::string* base_path, unsigned int num_threads) {
		// Static variable that will be created onces
        static LoadDataManager instance(base_path ? *base_path : "", num_threads);
        return instance;
    }

	void AddLoadVectorTask(char* vector, uint64_t points, uint64_t byte_offset, std::atomic<PointBufferState>& state);

};

#endif