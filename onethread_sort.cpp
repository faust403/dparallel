# include <algorithm>
# include <iostream>
# include <vector>
# include <random>
# include <chrono>

int main(void)
{
	std::random_device rd;
	std::vector<std::size_t> v(1'000'000'00);

	for (auto& elem : v)
		elem = rd();

	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	std::sort(v.begin(), v.end());
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << std::is_sorted(v.begin(), v.end()) << ' ' << v.size() << std::endl;
	std::cout << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;
}