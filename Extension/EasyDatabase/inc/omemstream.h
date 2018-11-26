#pragma once
#include <ostream>
#include <streambuf>

struct membuf : public std::streambuf {
	template <size_t SIZE>
	membuf(char(&arr)[SIZE]) {
		this->setp(arr, arr + SIZE - 1);
		std::fill_n(arr, SIZE, 0);
	}
	membuf(char* arr, size_t size) {
		this->setp(arr, arr + size - 1);
		std::fill_n(arr, size, 0);
	}
};

struct omemstream : virtual membuf, std::ostream {
	template <size_t SIZE>
	omemstream(char(&arr)[SIZE]) : membuf(arr), std::ostream(this) { }
	omemstream(char* arr, size_t size) : membuf(arr, size), std::ostream(this) { }
};