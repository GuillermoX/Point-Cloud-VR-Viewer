#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "PointBufferState.hpp"
#include "LASReader.hpp"

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

#define NUM_WORKERS 3
#define MAX_TASKS_PENDING 5

struct data_task_t
{
	char* vector;
	uint64_t num_points;
	uint64_t byte_offset;
	std::atomic<PointBufferState>* state;
};

/**
 * @brief Singleton class that manages disk data, importing data from binary file to octree buffers
 * @note It creates working threads to load data from disk to buffer
 */
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
	/**
	 * @brief Initializes the LoadDataManager singleton instance
	 * @param base_path: Point cloud binary file base path
	 * @param num_threads: Number of threads to create
	 */
	static void Init(const std::string& base_path, unsigned int num_threads)
	{
		static bool instance_created = false;	// static variable is only created once
		if (!instance_created)
		{
			getInstancePtr(&base_path, num_threads);
			instance_created = true;
		}
	}

	/**
	 * @brief Static method to get LoadDataManager singleton instance
	 */
	static LoadDataManager& GetInstance()
	{	
		return getInstancePtr(nullptr, 1);	// Ignore params since won't create new instances
	}

	/**
	 * @brief Helper to handle LoadDataManager instance securely
	 */
    static LoadDataManager& getInstancePtr(const std::string* base_path, unsigned int num_threads) {
		// Static variable that will be created onces
        static LoadDataManager instance(base_path ? *base_path : "", num_threads);
        return instance;
    }
	
	/**
	 * @brief Adds a task to the LoadDataManager
	 * @param vector: Buffer where the points will be stored (IMPORTANT: Don't destroy buffer until state is different than EMPTY)
	 * @param points: Number of points to be loaded
	 * @param byte_offset: Offset where the first point is inside point data binary file
	 * @param state: state of the buffer (will be set as READY_TO_LOAD_VRAM) after buffer is filled
	 */
	void AddLoadVectorTask(char* vector, uint64_t points, uint64_t byte_offset, std::atomic<PointBufferState>& state);

	/**
	 * @brief Determines if a task can be added depending of the current number of tasks
	 * @return true -> Task can be added, false -> task can't be added
	 */
	bool CanAddTask();

};

#endif