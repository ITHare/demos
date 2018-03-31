#define _SILENCE_PARALLEL_ALGORITHMS_EXPERIMENTAL_WARNING
#include <execution>
#include <iostream>
#include <assert.h>
#include <string>
#include <algorithm>

template<class Iterator>
size_t seq_calc_sum(Iterator begin, Iterator end) {
	size_t x = 0;
	std::for_each(begin, end, [&](int item) {
		x += item;
	});
	return x;
}

struct TestFuncForEachSeqSum {
	template<class Iterator>
	static size_t run(Iterator begin,Iterator end) {
		size_t x = 0;
		std::for_each(begin, end, [&](int item) {
			x += item;
		});
		return x;
	}
	static std::string name() { return "for_each/seq sum"; };
};

struct TestFuncForEachRefSeqSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		size_t x = 0;
		std::for_each(begin, end, [&](int& item) {
			x += item;
		});
		return x;
	}
	static std::string name() { return "for_each-ref/seq sum"; };
};

struct TestFuncForSeqSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		size_t x = 0;
		for (auto it = begin; it != end; ++it) {
			x += *it;
		}
		return x;
	}
	static std::string name() { return "for/seq sum"; };
};

struct TestFuncAccumulateSeqSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		return std::accumulate(begin, end, (size_t)0);
	}
	static std::string name() { return "accumulate/seq sum"; };
};

struct TestFuncReduceSeqSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		return std::reduce(begin, end, (size_t)0);
	}
	static std::string name() { return "reduce/seq sum"; };
};

struct TestFuncMutexParSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		size_t x = 0;
		std::mutex m;
		for (auto it = begin; it != end; ++it) {
			std::lock_guard<std::mutex> lk(m);
			x += *it;
		}
		return x;
	}
	static std::string name() { return "mutex/par sum"; };
};

struct TestFuncAtomicParSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		std::atomic<size_t> x = 0;
		for (auto it = begin; it != end; ++it) {
			x += *it;
		}
		return x;
	}
	static std::string name() { return "atomic/par sum"; };
};

struct TestFuncReduceParSum {
	template<class Iterator>
	static size_t run(Iterator begin, Iterator end) {
		return std::reduce(std::execution::par, begin, end, (size_t)0);
	}
	static std::string name() { return "reduce/par sum"; };
};

struct TestFuncManualParSum {
	template<class RandomAccessIterator>
	static size_t run(RandomAccessIterator begin, RandomAccessIterator end) {
		std::atomic<size_t> x = 0;
		constexpr int NCHUNKS = 128;
		RandomAccessIterator starts[NCHUNKS];
		size_t sz = (end - begin) / NCHUNKS;
		for (int i = 0; i < NCHUNKS; ++i) {
			starts[i] = begin + sz * i;
			assert(starts[i]<end);
		}
		std::for_each(std::execution::par_unseq, starts, starts + NCHUNKS, [&](RandomAccessIterator start) {
			size_t y = 0;
			for (auto it = start; it < start + sz; ++it) {
				y += *it;
			}
			x += y;
		});
		return x;
	}
	static std::string name() { return "manual/par sum"; };
};

struct TestFuncForEachSeqModify {
	template<class Iterator>
	static void run(Iterator begin, Iterator end) {
		std::for_each(begin, end, [](int& item) {
			item += 2;
		});
	}
	static std::string name() { return "for_each/seq modify"; };
};

struct TestFuncForSeqModify {
	template<class Iterator>
	static void run(Iterator begin, Iterator end) {
		for(auto it = begin; it != end; ++it) {
			*it += 2;
		}
	}
	static std::string name() { return "for/seq modify"; };
};

struct TestFuncForEachParModify {
	template<class Iterator>
	static void run(Iterator begin, Iterator end) {
		std::for_each(std::execution::par,begin, end, [](int& item) {
			item += 2;
		});
	}
	static std::string name() { return "for_each/par modify"; };
};

struct TestFuncTransformSeqModify {
	template<class Iterator>
	static void run(Iterator begin, Iterator end) {
		std::transform(begin, end, begin, [](int item) {
			return item + 2;
		});
	}
	static std::string name() { return "transform/seq modify"; };
};

struct TestFuncManualParModify {
	template<class RandomAccessIterator>
	static void run(RandomAccessIterator begin, RandomAccessIterator end) {
		constexpr int NCHUNKS = 128;
		RandomAccessIterator starts[NCHUNKS];
		size_t sz = (end - begin) / NCHUNKS;
		for (int i = 0; i < NCHUNKS; ++i) {
			starts[i] = begin + sz * i;
			assert(starts[i]<end);
		}
		std::for_each(std::execution::par_unseq, starts, starts + NCHUNKS, [&](RandomAccessIterator start) {
			size_t y = 0;
			for (auto it = start; it < start + sz; ++it) {
				*it += 2;
			}
		});
	}
	static std::string name() { return "manual/par modify"; };
};

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

constexpr int NN = 1000;
size_t seq0;

template<class TestFunc,class Container>
void runtest_modify(Container& a) {
	Benchmark bp;
	for (int i = 0; i < NN; ++i) {
		TestFunc::run(a.begin(), a.end());;
	}
	int64_t us = bp.us() / NN;
	std::cout << TestFunc::name() << ": took " << us << "us/run" << std::endl;
}

template<class TestFunc, class Container>
void runtest_sum(Container& a) {
	Benchmark bp;
	for (int i = 0; i < NN; ++i) {
		size_t par = TestFunc::run(a.begin(), a.end());;
		if (par != seq0) {
			std::cout << "WRONG RESULT @" << i << " iteration" << std::endl;
			abort();
		}
	}
	int64_t us = bp.us() / NN;
	std::cout << TestFunc::name() << ": took " << us << "us/run" << std::endl;
}

int main( int argc, char** argv ) {
	constexpr size_t SZ = 1048576*8;
	std::vector<int> a;
	a.reserve(SZ);

	for (size_t i = 0; i < SZ; ++i) {
		a.push_back( int(i * i*argc) );
	}
	seq0 = seq_calc_sum(a.begin(), a.end());

	runtest_sum<TestFuncForSeqSum>(a);
	runtest_sum<TestFuncForEachSeqSum>(a);
	runtest_sum<TestFuncForEachRefSeqSum>(a);
	runtest_sum<TestFuncAccumulateSeqSum>(a);
	runtest_sum<TestFuncReduceSeqSum>(a);

	runtest_sum<TestFuncMutexParSum>(a);
	runtest_sum<TestFuncAtomicParSum>(a);
	runtest_sum<TestFuncReduceParSum>(a);
	runtest_sum<TestFuncManualParSum>(a);

	std::cout << "===MODIFY===" << std::endl;

	runtest_modify<TestFuncForSeqModify>(a);
	runtest_modify<TestFuncForEachSeqModify>(a);
	runtest_modify<TestFuncForEachParModify>(a);
	runtest_modify<TestFuncTransformSeqModify>(a);
	runtest_modify<TestFuncManualParModify>(a);
	return 0;
}
