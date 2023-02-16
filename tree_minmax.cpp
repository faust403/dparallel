# include <algorithm>
# include <iostream>
# include <vector>
# include <random>
# include <chrono>
# include <set>

int main(void)
{
	std::random_device rd;
	std::vector<std::size_t> v(1'000'000'000);

	for (std::size_t Iter = 0; Iter < v.size(); ++Iter)
		v[Iter] = Iter;

	std::chrono::system_clock::time_point Begin = std::chrono::system_clock::now();
	std::set<std::size_t> set(v.begin(), v.end());
	std::chrono::system_clock::time_point End = std::chrono::system_clock::now();
	std::cout << *set.begin() << ' ' << *(--set.end()) << std::endl;
	std::cout << "Elaped time: " << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;

	Begin = std::chrono::system_clock::now();
	set.erase(set.begin());
	End = std::chrono::system_clock::now();
	std::cout << *set.begin() << ' ' << *(--set.end()) << std::endl;
	std::cout << "Elaped time: " << std::chrono::duration_cast<std::chrono::seconds>(End - Begin).count() << "s" << std::endl;

}