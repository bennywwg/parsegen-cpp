#pragma once

#include <set>
#include <iostream>
#include <functional>
#include <map>
#include <variant>
#include <tuple>
#include <concepts>

#ifdef __unix__
#include <cxxabi.h>
#endif

#include <parsegen.hpp>

namespace parsegen {
    using namespace std;

    std::string demangle(const char* mangled);

    template<class T>
    static std::string DemangleName() {
        return demangle(typeid(T).name());
    }

    std::string DemangleName(std::any const& a);

    std::string ToAlpha(int value);

    class frontend : public language {
    private:
        bool HasInitializedRules = false;
    public:
        std::map<std::string, std::string>                              normalize_production_name; // std::map from typeid name to normalized name suitable for parsegen
        std::map<std::string, std::string>                              denormalize_production_name; // reverse of the above map
        std::vector<std::function<std::any(std::vector<std::any>&)>>    productionCallbacks;
        std::vector<std::function<std::any(std::string&)>>              tokenCallbacks;
        uint32_t                                                        nextID = 0;

        std::string GetType(std::string name);

        template<typename Function, typename Tuple, size_t ... I>
        auto call(Function f, Tuple t, std::index_sequence<I ...>) const {
            return f(std::get<I>(t) ...);
        }

        template<typename Function, typename Tuple>
        auto call(Function f, Tuple t) const {
            static constexpr auto size = std::tuple_size<Tuple>::value;
            return call(f, t, std::make_index_sequence<size>{});
        }

        template<int Index, typename ...Args>
        typename std::enable_if<Index == 0, void>::type
        inline PackAnys(tuple<Args...>&, std::vector<std::any>&) const { }

        template<int Index, typename ...Args>
        typename std::enable_if<Index == 1, void>::type
        inline PackAnys(tuple<Args...>& args, std::vector<std::any>& anys) const {
            std::get<0>(args) = std::any_cast<decltype(std::get<0>(args))>(anys[0]);
        }

        template<int Index, typename ...Args>
        typename std::enable_if<(Index > 1), void>::type
        inline PackAnys(tuple<Args...>& args, std::vector<std::any>& anys) const {
            PackAnys<Index - 1, Args...>(args, anys);
            std::get<Index - 1>(args) = std::any_cast<decltype(std::get<Index - 1>(args))>(anys[Index - 1]);
        }

        template<typename ...Args>
        typename std::enable_if<sizeof...(Args) == 0, void>::type
        inline RegTypes(std::vector<std::string>&) { }

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) == 0, void>::type
        inline RegTypes(std::vector<std::string>& res) {
            res.back() = GetType(DemangleName<R>());
        }

        template<typename R, typename ...Args>
        typename std::enable_if<sizeof...(Args) != 0, void>::type
        inline RegTypes(std::vector<std::string>& res) {
            RegTypes<Args...>(res);
            res[res.size() - sizeof...(Args) - 1] = GetType(DemangleName<R>());
        }

        template<typename R, typename ...Args>
        inline void AddRule_Internal(std::function<R(Args...)> const& func) {
            std::vector<std::string> rhs; rhs.resize(sizeof...(Args));
            RegTypes<Args...>(rhs);

            const std::string lhs = GetType(DemangleName<R>());

            productionCallbacks.push_back([this, func](std::vector<std::any>& anys) -> std::any {
                tuple<Args...> args;
                PackAnys<sizeof...(Args), Args...>(args, anys);
                return call(func, args);
            });

            productions.push_back({ lhs, rhs });
        }

        template<typename F>
        inline void RuleF(F lambda) { AddRule_Internal(std::function(lambda)); }

        template<typename R, typename ...Args>
        inline void AddToken_Internal(std::function<R(std::string&)> const& func, std::string const& regex) {
            const std::string lhs = GetType(DemangleName<R>());

            tokenCallbacks.push_back([this, func](std::string& val) -> std::any {
                return func(val);
            });

            tokens.push_back({ lhs, regex });
        }

        template<typename F>
        inline void Token(F lambda, std::string const& regex) { AddToken_Internal(std::function(lambda), regex); }

        template<typename T>
        typename std::enable_if<!std::is_constructible<T, std::string&>::value, void>::type
        inline Token(std::string const& regex) { AddToken_Internal(std::function([](std::string&) { return T{ }; }), regex); }

        template<typename T>
        typename std::enable_if<std::is_constructible<T, std::string&>::value, void>::type
        inline Token(std::string const& regex) { AddToken_Internal(std::function([](std::string& text) { return T{ text }; }), regex); }

        virtual void InitRules() = 0;
    };


    class ParserImpl : public parsegen::parser {
    public:
        ParserImpl(std::shared_ptr<frontend> l);
        virtual ~ParserImpl() = default;

    protected:
        std::shared_ptr<frontend> lang;

        virtual std::any shift(int token, std::string& text) override;
        virtual std::any reduce(int prod, std::vector<std::any>& rhs) override;
    };

    // T is the final type the parser will produce
    template<typename LangType>
    class Parser {
    private:
        std::shared_ptr<ParserImpl> PImpl;
    public:
        inline typename LangType::ReturnType Parse(std::string const& text) {
            std::any res = PImpl->parse_string(text, "input");
            try {
                return std::any_cast<typename LangType::ReturnType>(res);
            } catch (std::bad_any_cast const&) {
                throw std::runtime_error("parse_string completed but the return type was incorrect");
            }
        }

        Parser()
        requires std::derived_from<LangType, frontend>
        {
            std::shared_ptr<frontend> lang = std::make_shared<LangType>();

            lang->InitRules();

            PImpl = std::make_shared<ParserImpl>(lang);
        }

        template<typename ...Args>
        Parser(Args&&... args)
        requires std::derived_from<LangType, frontend> && (sizeof...(Args) > 0)
        {
            std::shared_ptr<frontend> lang = std::make_shared<LangType>(std::forward<Args>(args)...);

            lang->InitRules();

            ParserImpl = std::make_shared<ParserImpl>(lang);
        }
    };
}

#define Rule denormalize_production_names[static_cast<int>(productions.size())] = (__FILE__ + std::string(":") + std::to_string(__LINE__)), RuleF