#include <array>
#include <string>
#include <algorithm>
#include <iostream>
#include <cassert>

// Shows array filling

using namespace std::literals::string_literals;

template<unsigned SZ = 4>
struct Array
{
    template <typename... Args>
    Array(Args&&... args) noexcept {
        
        static_assert(sizeof...(Args) <= SZ, "Too many args");
        
        (assert(!args.empty()), ...);

        std::size_t idx = 0;
        // The comma operator is left-associative, ideally one would use a left unary fold.
        // Cast to void to ensures the use of built-in comma operator.
        (..., (entries[idx++] = args));
    }

    void print_on(std::ostream& os) const {
        os << '[';
        std::copy_if(entries.begin(), entries.end(), std::ostream_iterator<std::string>(os, "_"),
            [](auto const& str){ return !str.empty(); });
        os << ']';
    }

    std::array<std::string, SZ> entries;
};

template<unsigned SZ>
std::ostream& operator<<(std::ostream& os, Array<SZ> const& array) {
    array.print_on(os);
    return os;
}

int main() {
    auto const a0 = Array();

    auto const a1 = Array("Hello"s);
    std::cout << a1 << '\n';
    
    auto const a2 = Array("Hello"s, "World"s);
    std::cout << a2 << '\n';
    
    auto const a3 = Array("Hello"s, ","s, "World"s, "!"s);
    std::cout << a3 << '\n';
    
    //Array("Hello"s, "abort"s, std::string{}); // not allow, goes abort()
    
    return 0;
}
