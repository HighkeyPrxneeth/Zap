#include <windows.h>
#include <iostream>
#include <cstring>

template <typename T>
T fromBytes(const BYTE *data, int startPtr) {
	T result;
	std::memcpy(&result, data + startPtr, sizeof(T));
	return result;
}
