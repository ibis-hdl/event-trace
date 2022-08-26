// https://godbolt.org/z/8Edch148d (orig)
// https://godbolt.org/z/9ce98e6Tq
// fails with emplace_back() https://godbolt.org/z/E9b9sYKbG
// compiler doesn't know how to figure out that you mean to pass an std::array here
// -> explicit with the types https://godbolt.org/z/5aKzzdTqc
// -> use push_back, which isn't a template https://godbolt.org/z/Gvsca3z6f
// push_back would call the move ctor, and emplace_back calls the move ctors of those 
// two std::array arguments so the two things are roughly equivalent move-wise
// still API problem due to different layers of abstraction

#if 0
    items.emplace_back(
        std::array<std::string_view, MAX_ARGS>{std::get<Is*2>(std::forward<ArgsTupleT>(args_tuple)) ...},
        std::array<int, MAX_ARGS>{std::get<Is*2+1>(std::forward<ArgsTupleT>(args_tuple)) ...}
    );
		
    items.push_back(std::move(item{
        {std::get<Is*2>(std::forward<ArgsTupleT>(args_tuple)) ...},
        {std::get<Is*2+1>(std::forward<ArgsTupleT>(args_tuple)) ...}
    }));
#endif

// Idea: use class for this (doesn't compile): https://godbolt.org/z/76Ehsxo4W

#include <iostream>
#include <tuple>
#include <utility>
#include <array>

constexpr auto MAX_ARGS = 2;

template<typename T> concept ArgNameT = std::convertible_to<T, std::string_view>;
template<typename T> concept ArgValueT = std::convertible_to<T, int>;
template<typename... T> concept CountArgT = requires(T... args) { // https://godbolt.org/z/dEGsd781K
    // count must be even
    requires (sizeof...(T)) % 2 == 0;
    // the count is limited by arg count absolute size
    requires (sizeof...(T)) / (2*MAX_ARGS+1) == 0;
};

struct item
{
    item(std::array<std::string_view, MAX_ARGS>&& arg_name, std::array<int, MAX_ARGS>&& arg_value)
        : names(std::move(arg_name)), values(std::move(arg_value)) {
        std::cout << "item [name: ";
        for (std::string_view name : names)
            std::cout << name << ", ";

        std::cout << "]\nitem [value: ";
        for (int value : values)
            std::cout << value << ", ";

        std::cout << "]\n";        
    }

    std::array<std::string_view, MAX_ARGS> names;
    std::array<int, MAX_ARGS> values;
};

template <typename ArgsTupleT, std::size_t... Is>
void collect_helper(ArgsTupleT&& args_tuple, std::index_sequence<Is...>) {

    std::cout << "collect_helper:\n";
    std::cout << "[name: ";
    ((std::cout << std::get<Is*2>(std::forward<ArgsTupleT>(args_tuple)) << ", "), ...);
    std::cout << "]\n[value: ";
    ((std::cout << std::get<Is*2+1>(std::forward<ArgsTupleT>(args_tuple)) << ", "), ...);
    std::cout << "]\n";

    auto i = item(
        {std::get<Is*2>(std::forward<ArgsTupleT>(args_tuple)) ...},
        {std::get<Is*2+1>(std::forward<ArgsTupleT>(args_tuple)) ...}
        );
}

template<typename... ArgsT> requires CountArgT<ArgsT...>
void collect(ArgsT&& ... args) {
    collect_helper(std::forward_as_tuple(args...),
                   std::make_index_sequence<sizeof...(ArgsT)/2>{});
}

int main() {
    auto const hline = std::string("----------------------------\n");

    collect();
    std::cout << hline;

    collect("is bad", 666);
    std::cout << hline;

    collect("is good", 666, "answer is", 42);
    std::cout << hline;

#if 0 // constraints not satisfied
    collect("is good", 666, "answer is", 42, "invalid");
    std::cout << hline;
#endif
#if 0 // constraints not satisfied
    collect("is good", 666, "answer is", 42, "invalid", 666);
    std::cout << hline;
#endif
}