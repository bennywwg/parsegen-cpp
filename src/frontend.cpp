#include "frontend.hpp"

namespace parsegen {
    class Parser : public parsegen::parser {
    public:
        std::any Parse(std::string const& text) {
            return parse_string(text, "input");
            /*try {
                return std::any_cast<Program>(res);
            }
            catch (std::bad_any_cast const& ex) {
                throw std::runtime_error("parse_string completed but the return type was incorrect");
            }*/
        }

        Parser(std::shared_ptr<frontend> l) : parser(l->tables), lang(l) { }
        virtual ~Parser() = default;

    protected:
        std::shared_ptr<frontend> lang;

        virtual std::any shift(int token, std::string& text) override {
            auto it = lang->tokenCallbacks.find(token);
            if (it != lang->tokenCallbacks.end()) return it->second(text);
            throw std::runtime_error("Internal error, token that should exist wasn't found");
        }
        virtual std::any reduce(int prod, std::vector<std::any>& rhs) override {
            auto it = lang->productionCallbacks.find(prod);
            if (it != lang->productionCallbacks.end()) return it->second(rhs);
            throw std::runtime_error("Internal error, rule that should exist wasn't found");
        }
    };

    std::string frontend::GetType(std::string name) {
        if (norm.find(name) == norm.end()) {
            ++nextID;
            norm[name] = ToAlpha(nextID);
            denormalize_map[norm[name]] = name;
        }

        return norm[name];
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