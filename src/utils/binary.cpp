#include <windows.h>
#include <cstring>

template <typename T>
T fromBytes(const BYTE *data, int startIdx) {
	T result;
	std::memcpy(&result, data + startIdx, sizeof(T));
	return result;
}
