# include <iostream>
# include <vector>
# include <random>
# include <chrono>
# include <algorithm>

# include <CL/sycl.hpp>

# include <thread>
# include <future>

/*
	Дан массив из 100млрд элементов, найдите минимум и максимум, оптимально используя ресурсы Вашего процессора
*/

std::pair<std::size_t, std::size_t> classic_solve(std::vector<std::size_t>& v)
{
	const static std::size_t hc = std::thread::hardware_concurrency();
	std::vector<std::future<std::pair<std::size_t, std::size_t>>> Futures;

	for (std::size_t Iter = 0; Iter < hc; ++Iter)
	{
		std::packaged_task<std::pair<std::size_t, std::size_t>(std::vector<std::size_t>::iterator, std::vector<std::size_t>::iterator)> Pkg(
			[&](std::vector<std::size_t>::iterator From, std::vector<std::size_t>::iterator To) -> std::pair<std::size_t, std::size_t> {
				std::pair<std::size_t, std::size_t> Result = std::make_pair(*From, *From);

				for (; From != To; ++From)
				{
					if (*From < Result.first)
						Result.first = *From;
					if (*From > Result.second)
						Result.second = *From;
				}
				return Result;
			}
		);
		std::future<std::pair<std::size_t, std::size_t>> Future = Pkg.get_future();
		Futures.push_back(std::move(Future));
		std::thread Thread(
			std::move(Pkg),
			v.begin() + double(Iter) / double(hc) * v.size(),
			v.begin() + double(Iter + 1) / double(hc) * v.size()
		);
		Thread.detach();
	}

	std::pair<std::size_t, std::size_t> Result = std::make_pair(v[0], v[0]);
	for (auto& elem : Futures)
	{
		std::pair<std::size_t, std::size_t> const& Pair = elem.get();
		if (Pair.first < Result.first)
			Result.first = Pair.first;
		if (Pair.second > Result.second)
			Result.second = Pair.second;
	}
	return Result;
}

[[ nodiscard ]]
std::pair<std::size_t, std::size_t> sycl_solve(sycl::queue& Queue, std::vector<std::size_t>const& v)
{
	static const std::size_t vsize = v.size();
	if (vsize == 0)
		return std::make_pair(0, 0);

	std::pair<std::size_t, std::size_t> Result = std::make_pair(v[0], v[0]);


	// Создаем отрезки, в которых будем искать минимум и максимум. Кладем их в буффера, а буффера в вектор
	static const std::size_t hc = std::thread::hardware_concurrency();
	std::vector<std::vector<std::size_t>> Tasks(hc);
	for(std::size_t Iter = 0; Iter < hc; ++Iter)
	{
		std::vector<std::size_t>::const_iterator From = v.begin() + double(Iter) / double(hc) * v.size(),
												 To   = v.begin() + double(Iter + 1) / double(hc) * v.size();

		Tasks[Iter] = std::vector<std::size_t>(From, To);
	}

	std::vector<std::pair<std::size_t, std::size_t>> Results(hc);
	sycl::buffer<std::pair<std::size_t, std::size_t>, 1> ResultsBuffer(Results.data(), hc);
	sycl::buffer<std::vector<std::size_t>, 1> TasksBuffer(Tasks.data(), hc);

	Queue.submit(
		[&](sycl::handler& Handler) -> void
		{
			sycl::accessor ResultsAccessor = ResultsBuffer.template get_access<sycl::access_mode::write>(Handler);
			sycl::accessor TasksAccessor = TasksBuffer.template get_access<sycl::access_mode::read>(Handler);

			Handler.parallel_for<class ParallelFor>(sycl::range<1>{hc}, [=](sycl::id<1> const& Id) -> void
			{
				std::vector<std::size_t> const& CurrentTask = TasksAccessor[Id];
				std::pair<std::size_t, std::size_t> _LocalPair = std::make_pair(CurrentTask[0], CurrentTask[0]);

				std::for_each(CurrentTask.cbegin(), CurrentTask.cend(), [&](std::size_t const& Item) -> void
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

int main(void)
{
	std::random_device rd;
	sycl::queue Queue{ sycl::cpu_selector_v };
	std::vector<std::size_t> v(1'000'000'000);

	for (std::size_t Iter = 0; Iter < v.size(); Iter += 1)
		v[Iter] = Iter;

	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	std::pair<std::size_t, std::size_t> SYCL_Pair = sycl_solve(Queue, v);
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << SYCL_Pair.first << ' ' << SYCL_Pair.second << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;

}