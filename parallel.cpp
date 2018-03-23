//should be formally compliant with the standard, but was tested only with MSVC

#define _SILENCE_PARALLEL_ALGORITHMS_EXPERIMENTAL_WARNING
#include <execution>
#include <iostream>

template<class Iterator>
size_t seq_calc_sum(Iterator begin, Iterator end) {
	size_t x = 0;
	std::for_each(begin, end, [&](int item) {
		x += item;
	});
	return x;
}

template<class Iterator>
size_t par_calc_sum_mutex(Iterator begin, Iterator end) {
	size_t x = 0;
	std::mutex m;
	std::for_each(std::execution::par,begin, end, [&](int item) {
		std::lock_guard<std::mutex> guard(m);
		x += item;
	});
	return x;
}

template<class Iterator>
size_t par_calc_sum_atomic(Iterator begin, Iterator end) {
	std::atomic<size_t> x = 0;
	std::for_each(std::execution::par, begin, end, [&](int item) {
		x += item;
	});
	return x;
}

template<class Iterator>
size_t par_calc_sum_atomic_unseq(Iterator begin, Iterator end) {
	std::atomic<size_t> x = 0;
	std::for_each(std::execution::par_unseq, begin, end, [&](int item) {
		x += item;
	});
	return x;
}

class Benchmark {
	std::chrono::high_resolution_clock::time_point start;

public:
	Benchmark() {
		start = std::chrono::high_resolution_clock::now();
	}
	int64_t us() {
		auto stop = std::chrono::high_resolution_clock::now();
		auto length = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
		return (int64_t)length.count();
	}
};

int main() {
	constexpr size_t SZ = 1048576;
	constexpr int NN = 1000;
	std::vector<int> a;
	a.reserve(SZ);

	for (size_t i = 0; i < SZ; ++i) {
		a.push_back( int(i * i) );
	}

	size_t seq0 = seq_calc_sum(a.begin(), a.end());

	{
		Benchmark bs;
		for (int i = 0; i < NN; ++i) {
			size_t seq = seq_calc_sum(a.begin(), a.end());;
			if (seq != seq0)
				abort();
		}
		int64_t mss = bs.us() / 1000;
		std::cout << "seq: took " << mss << "ms" << std::endl;
	}

	{
		Benchmark bpm;
		for (int i = 0; i < NN; ++i) {
			size_t par = par_calc_sum_mutex(a.begin(), a.end());;
			if (par != seq0)
				abort();
		}
		int64_t mspm = bpm.us() / 1000;
		std::cout << "par/mutex: took " << mspm << "ms" << std::endl;
	}
	
	{
		Benchmark bpa;
		for (int i = 0; i < NN; ++i) {
			size_t par = par_calc_sum_atomic(a.begin(), a.end());;
			if (par != seq0)
				abort();
		}
		int64_t mspa = bpa.us() / 1000;
		std::cout << "par/atomic: took " << mspa << "ms" << std::endl;
	}

	{
		Benchmark bpua;
		for (int i = 0; i < NN; ++i) {
			size_t par = par_calc_sum_atomic_unseq(a.begin(), a.end());;
			if (par != seq0)
				abort();
		}
		int64_t mspua = bpua.us() / 1000;
		std::cout << "par_unseq/atomic: took " << mspua << "ms" << std::endl;
	}
}

