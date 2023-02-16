# include <algorithm>
# include <iostream>
# include <vector>

# include <CL/sycl.hpp>


template<typename T>
std::pair<T, T> __sycl_minmax(sycl::queue& Queue, std::vector<T>const& v)
{
	static const std::size_t vsize = v.size();
	if (vsize == 0)
		return std::make_pair(T(0), T(0));

	std::pair<T, T> Result = std::make_pair(v[0], v[0]);

	// Создаем отрезки, в которых будем искать минимум и максимум. Кладем их в буффера, а буффера в вектор
	static const std::size_t hc = std::thread::hardware_concurrency();
	std::vector<std::vector<T>> Tasks(hc);
	for (std::size_t Iter = 0; Iter < hc; ++Iter)
	{
		typename std::vector<T>::const_iterator From = v.begin() + double(Iter) / double(hc) * v.size(),
			To = v.begin() + double(Iter + 1) / double(hc) * v.size();

		Tasks[Iter] = std::vector<T>(From, To);
	}

	std::vector<std::pair<T, T>> Results(hc);
	sycl::buffer<std::pair<T, T>, 1> ResultsBuffer(Results.data(), hc);
	sycl::buffer<std::vector<T>, 1> TasksBuffer(Tasks.data(), hc);

	Queue.submit(
		[&](sycl::handler& Handler) -> void
		{
			sycl::accessor ResultsAccessor = ResultsBuffer.template get_access<sycl::access_mode::write>(Handler);
	sycl::accessor TasksAccessor = TasksBuffer.template get_access<sycl::access_mode::read>(Handler);

	Handler.parallel_for<class ParallelFor>(sycl::range<1>{hc}, [=](sycl::id<1> const& Id) -> void
		{
			std::vector<T> const& CurrentTask = TasksAccessor[Id];
	std::pair<T, T> _LocalPair = std::make_pair(CurrentTask[0], CurrentTask[0]);

	std::for_each(CurrentTask.cbegin(), CurrentTask.cend(), [&](T const& Item) -> void
		{
			if (_LocalPair.first > Item)
			_LocalPair.first = Item;
	if (_LocalPair.second < Item)
		_LocalPair.second = Item;
		});
	ResultsAccessor[Id] = std::move(_LocalPair);
		});
		}
	).wait();

	for (auto& elem : Results)
	{
		if (Result.first > elem.first)
			Result.first = elem.first;
		if (Result.second < elem.second)
			Result.second = elem.second;
	}
	return Result;
}

template<typename T>
std::pair<T, T> __sycl_minmax(sycl::queue& Queue, std::vector<T>const& v, const std::size_t hc, const std::size_t Limit)
{
	if (v.size() / hc > Limit)
	{
		std::pair<T, T> Result = std::make_pair(v[0], v[0]);
		std::vector<std::pair<T, T>> _Local(hc);

		for (std::size_t Iter = 0; Iter < hc; ++Iter)
		{
			typename std::vector<T>::const_iterator From = v.cbegin() + double(Iter) / double(hc) * v.size(),
				To = v.cbegin() + double(Iter + 1) / double(hc) * v.size();

			_Local[Iter] = __sycl_minmax(Queue, std::vector<T>{From, To}, hc, Limit);
		}

		for (auto const& Pair : _Local)
		{
			if (Pair.first < Result.first)
				Result.first = Pair.first;
			if (Pair.second > Result.second)
				Result.second = Pair.second;
		}
		return Result;
	}
	else
		return __sycl_minmax(Queue, v);
}

template<typename T>
std::pair<T, T> sycl_minmax(std::vector<T>const& v, const std::size_t hc, const std::size_t Limit)
{
	sycl::queue Queue{ sycl::cpu_selector_v };

	return __sycl_minmax(Queue, v, hc, Limit);
}

int main(void)
{
	std::vector<unsigned char> v(10'000'000'000);
	for (std::size_t Iter = 0; Iter < v.size(); ++Iter)
		v[Iter] = (unsigned char)(Iter % 256);

	std::cout << "Benchmark start" << std::endl;
	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	std::pair<unsigned char, unsigned char> min_max = sycl_minmax(v, std::thread::hardware_concurrency(), 100'000'000);
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << int(min_max.first) << ' ' << int(min_max.second) << std::endl;
	std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;
}