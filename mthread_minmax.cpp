# include <iostream>
# include <thread>
# include <future>
# include <random>
# include <vector>
# include <ctime>

int main(void)
{
	std::random_device rd;
	std::vector<std::size_t> v(1'000'000'000);

	for (std::size_t Iter = 0; Iter < v.size(); ++Iter)
		v[Iter] = Iter + 1;

	const static std::size_t hc = std::thread::hardware_concurrency();
	std::vector<std::future<std::pair<std::size_t, std::size_t>>> Futures;

	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	for(std::size_t Iter = 0; Iter < hc; ++Iter)
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
		std::thread Thread (
			std::move(Pkg),
			v.begin() + double(Iter)/double(hc) * v.size(),
			v.begin() + double(Iter + 1) / double(hc) * v.size()
		);
		Thread.detach();
	}

	std::pair<std::size_t, std::size_t> Result = std::make_pair(v[0], v[0]);
	for(auto& elem :Futures)
	{
		std::pair<std::size_t, std::size_t> const& Pair = elem.get();
		if (Pair.first < Result.first)
			Result.first = Pair.first;
		if (Pair.second > Result.second)
			Result.second = Pair.second;
	}
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << Result.first << ' ' << Result.second << std::endl;
	std::cout << "Elapsed time: " << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;
}