#include <iostream>
#include <neolib/core/string_utils.hpp>

int main()
{
    neolib::string_search_fsa searchFsa;
    searchFsa.add_pattern("foo", [](char const* begin, char const* end) { std::cout << "Found foo [" << std::string_view{begin, end} << "]!\n"; });
    searchFsa.add_pattern("bar", [](char const* begin, char const* end) { std::cout << "Found bar [" << std::string_view{begin, end} << "]!\n"; });
    searchFsa.add_pattern("baz", [](char const* begin, char const* end) { std::cout << "Found baz [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("f*o", [](char const* begin, char const* end) { std::cout << "Found f*o [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("b*r", [](char const* begin, char const* end) { std::cout << "Found b*r [" << std::string_view{begin, end} << "]!\n"; });
    searchFsa.add_pattern("b*z", [](char const* begin, char const* end) { std::cout << "Found b*z [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("*oo", [](char const* begin, char const* end) { std::cout << "Found *oo [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("*ar", [](char const* begin, char const* end) { std::cout << "Found *ar [" << std::string_view{begin, end} << "]!\n"; });
    searchFsa.add_pattern("*az", [](char const* begin, char const* end) { std::cout << "Found *az [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("fo*", [](char const* begin, char const* end) { std::cout << "Found fo* [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("ba*", [](char const* begin, char const* end) { std::cout << "Found ba* [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("ba*", [](char const* begin, char const* end) { std::cout << "Found ba* [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("f?o", [](char const* begin, char const* end) { std::cout << "Found f?o [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("b?r", [](char const* begin, char const* end) { std::cout << "Found b?r [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("b?z", [](char const* begin, char const* end) { std::cout << "Found b?z [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("fo?", [](char const* begin, char const* end) { std::cout << "Found fo? [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("ba?", [](char const* begin, char const* end) { std::cout << "Found ba? [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("ba?", [](char const* begin, char const* end) { std::cout << "Found ba? [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("?oo", [](char const* begin, char const* end) { std::cout << "Found ?oo [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("?ar", [](char const* begin, char const* end) { std::cout << "Found ?ar [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("?az", [](char const* begin, char const* end) { std::cout << "Found ?az [" << std::string_view{ begin, end } << "]!\n"; });
    searchFsa.add_pattern("f*d", [](char const* begin, char const* end) { std::cout << "Found f*d [" << std::string_view{ begin, end } << "]!\n"; });
    
    auto search = [&](std::string const& term)
    {
        std::cout << "Search [" << term << "]:-" << std::endl;
        searchFsa.search(term);
    };

    search("fo");
    search("ba");
    search("oo");
    search("ar");
    search("az");
    search("foo");
    search("bar");
    search("baz");
    search("so, foodly doodly abazzer bar");
}