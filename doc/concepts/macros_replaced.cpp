// https://godbolt.org/z/EqoaKr7T4

#include <string>
#include <string_view>
#include <iostream>

// concat helper macros
#define CONCAT(a, b, c) CONCAT_(a, b, c)
#define CONCAT_(a, b, c) __##a##_##b##_##c

/// uniq_<label>_<UniqueID>
#define UNIQUE_ID __LINE__
#define UNIQUE_NAME(label) CONCAT(uniq, label, UNIQUE_ID)

#define ADD_SCOPED(cat_name, event_name, ...)                            \
    static auto const UNIQUE_NAME(category_proxy) = cat_get(cat_name);   \
    CloseGuard UNIQUE_NAME(scope_guard)(                                 \
        UNIQUE_NAME(category_proxy), event_name);                        \
    if (UNIQUE_NAME(category_proxy)) {                                   \
        collect(UNIQUE_NAME(category_proxy).category_name, event_name,   \
                ##__VA_ARGS__);                                          \
    }

struct proxy {
    explicit operator bool() const noexcept { return true; }
    std::string category_name;
};

proxy cat_get(std::string name) {
    return proxy{ name };
}

struct CloseGuard {
    CloseGuard(proxy category_proxy_, std::string event_name_)
    : category_proxy{ category_proxy_ }
    , event_name{ event_name_ }
    {}
    ~CloseGuard() {
        // how to ensure noexcept ???
        if (category_proxy) { add_event(); }
    }
    void add_event() noexcept {
        std::cout << "ScopeClose: \"" 
                  << category_proxy.category_name << "\", event: \"" << event_name << "\""
                  << "\n";
    }    
    proxy category_proxy;
    std::string event_name;
};

template <typename... Args>
void collect(std::string category_name, std::string event_name, Args&&... args) {
    std::cout << "category_name: " << category_name << '\n';
    std::cout << "event_name: " << event_name  << '\n';
    std::cout << "args:";
    ((std::cout << " " << args), ...);
    std::cout << '\n';
}

int main() {

    {
        static auto const UNIQUE_NAME_category_proxy = cat_get("cat_name");
        CloseGuard UNIQUE_NAME_scope_guard(UNIQUE_NAME_category_proxy, "event_name");
        if(UNIQUE_NAME_category_proxy) {
            collect(UNIQUE_NAME_category_proxy.category_name, "event_name",
                "arg1", 42);
        }
    }

    ADD_SCOPED("other_cat", "other_event", "C++", "is", "cool");

    {
        static auto const UNIQUE_NAME_category_proxy{ cat_get("cat_name") };
        auto UNIQUE_NAME_scope_guard{ CloseGuard(UNIQUE_NAME_category_proxy, "event_name") };
        if(UNIQUE_NAME_category_proxy) {
            collect(UNIQUE_NAME_category_proxy.category_name, "event_name",
                "arg1", 42);
        }
    }

    return 0;
} 
