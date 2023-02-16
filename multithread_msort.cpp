# include <iostream>
# include <vector>
# include <algorithm>
# include <random>
# include <chrono>
# include <future>

int main(void)
{
	std::random_device rd;
	std::vector<std::size_t> v(1'000'000'00);
	for (auto& elem : v)
		elem = rd();

	static const std::size_t hc = std::thread::hardware_concurrency();
	std::vector<std::future<std::vector<std::size_t>>> Futures;
	std::vector<std::size_t> res = {};

	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	for (std::size_t Iter = 0; Iter < hc; Iter += 1)
	{
		std::packaged_task<std::vector<std::size_t>(std::vector<std::size_t>::iterator, std::vector<std::size_t>::iterator)> Pkg(
			[&](std::vector<std::size_t>::iterator Begin, std::vector<std::size_t>::iterator End) -> std::vector<std::size_t>
			{
				std::vector<std::size_t> Result(Begin, End);
				std::sort(Result.begin(), Result.end());
				return Result;
			}
		);
		std::future<std::vector<std::size_t>> Future = Pkg.get_future();
		Futures.push_back(std::move(Future));
		std::vector<std::size_t>::iterator Begin = v.begin() + (double)Iter / (double)hc * v.size(),
										   End = v.begin() + double(Iter + 1) / double(hc) * v.size();
		std::thread Thread(std::move(Pkg), Begin, End);
		Thread.detach();
	}

	for (auto& elem : Futures)
	{
		std::vector<std::size_t> cv = elem.get(), _n;
		std::merge(res.begin(), res.end(), cv.begin(), cv.end(), std::back_inserter(_n));
		res = std::move(_n);
	}
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << std::is_sorted(res.begin(), res.end()) << ' ' << res.size() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;
}