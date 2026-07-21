#include <LoadDataManager.hpp>


LoadDataManager::LoadDataManager(const std::string& base_path, unsigned int num_workers)
{
	_base_path = base_path;		// Copy base path and store it in instance property

	for (int i = 0; i < num_workers; i++)
	{
		_workers.emplace_back(&LoadDataManager::workerLoop, this);
	}
}

LoadDataManager::~LoadDataManager() {
	// If LoadDataManager instance deleted notify all threads to stop
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stop = true;
    }
    _cv.notify_all();

    for (std::thread& worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void LoadDataManager::workerLoop()
{
	// Open binary file once in each thread
	std::ifstream file(_base_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[DATA MANAGER | ERROR] Unable to open binary file : " << _base_path << std::endl;
		return;
    }		

	while (true)
	{
		data_task_t task;

		{
			std::unique_lock<std::mutex> lock(_mutex);		// Lock mutex so only one thread can access to queue

			// Sleep thread if no task or no stoped
			_cv.wait(lock, [this] {		// conditional_variables releases mutex if thread sleeps
					return _stop || !_task_queue.empty();
				});
			
			// Stop thread if stop flag and all tasks completed
			if (_stop && _task_queue.empty())
			{
				// Close file and exit the thread
				file.close();
				return;
			}
			
			// Get task from queue
			task = std::move(_task_queue.front());
			_task_queue.pop();

		}
		
		// Add disk data to buffer
		file.seekg(task.byte_offset, std::ios::beg);	// Move file read pointer to byte offset where points are stored
		file.read(task.vector, task.num_points * sizeof(point_data_t));
	
		// Update node state so it knows buffer is filled
		task.state->store(PointBufferState::READY_TO_LOAD_VRAM, std::memory_order_release);
	}

}



void LoadDataManager::AddLoadVectorTask(char* vector, uint64_t points, uint64_t byte_offset, std::atomic<PointBufferState>& state)
{
	state.store(PointBufferState::LOADING, std::memory_order_release);	// Update buffer state to Loading
	// Create task struct
	data_task_t task;
	task.vector = vector;
	task.num_points = points;
	task.byte_offset = byte_offset;
	task.state = &state;
	
	// Add task to the worker's queue using mutex (since workers threads can be accessing the queue)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_task_queue.push(task);
	}
	// Wake up one thread to process the task
	_cv.notify_one();
}