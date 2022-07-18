#include "frontend.hpp"

namespace parsegen {
    ParserImpl::ParserImpl(std::shared_ptr<frontend> l)
    : parser(build_parser_tables(*l)), lang(l) { }

    std::any ParserImpl::shift(int token, std::string& text) {
        return lang->tokenCallbacks[token](text);
    }

    std::any ParserImpl::reduce(int prod, std::vector<std::any>& rhs) {
        return lang->productionCallbacks[prod](rhs);
    }
    
    std::string demangle(const char* mangled) {
#ifdef __unix__
        int status;
        std::unique_ptr<char[], void (*)(void*)> result(
            abi::__cxa_demangle(mangled, 0, 0, &status), std::free);
        return result.get() ? std::string(result.get()) : "error occurred";
#else
        return mangled;
#endif
    }

    std::string DemangleName(std::any const& a) {
        return demangle(a.type().name());
    }

    std::string ToAlpha(int value) {
        std::string res = to_string(value);
        for (size_t i = 0; i < res.size(); ++i) {
            res[i] += ('a' - '0');
        }
        return res;
    }
}