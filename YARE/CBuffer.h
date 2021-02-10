#pragma once
#ifndef _CBUFFER_H_
#define _CBUFFER_H_

#include "pch.h"

template <class T>
struct CBuffer
{
public:
	CBuffer() = default;
	CBuffer(size_t elementCount_)
	{
		elementCount = elementCount_;
		size = sizeof(T) * elementCount;
		values.resize(elementCount);
	}

	void Update() {
		if (ptr != nullptr) {
			// Copy multiple values (array)
			if (elementCount > 0) {
				memcpy(ptr, &values[0], size);
			}
			// Copy single value
			else {
				memcpy(ptr, &value, sizeof(value));
			}
		}
	}

	T value = {};
	std::vector<T> values;
	size_t elementCount = 0;
	UINT8* ptr = nullptr;
	ComPtr<ID3D12Resource> resource = NULL;

private:
	size_t size = 0;
};

#endif // !_CBUFFER_H_
