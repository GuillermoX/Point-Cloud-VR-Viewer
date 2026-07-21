// ------------------ Support enum for DataManager and Octree ---------------------
// (Separate header to avoid circular dependencie between DataManager and Octree)
#ifndef POINT_BUFFER_STATE_H
#define POINT_BUFFER_STATE_H
enum class PointBufferState
{
	EMPTY,
	LOADING,
	READY_TO_LOAD_VRAM,		// Ready in buffer but not in VRAM
	READY_IN_VRAM			// Ready in VRAM but no buffer
};
#endif
