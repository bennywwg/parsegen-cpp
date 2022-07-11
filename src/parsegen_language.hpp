#ifndef PARSEGEN_LANGUAGE_HPP
#define PARSEGEN_LANGUAGE_HPP

#include <iosfwd>
#include <map>
#include <string>
#include <vector>

#include "parsegen_finite_automaton.hpp"
#include "parsegen_grammar.hpp"
#include "parsegen_parser_tables.hpp"

namespace parsegen {

struct language {
  struct token {
    std::string name;
    std::string regex;
  };
  std::vector<token> tokens;
  struct production {
    std::string lhs;
    std::vector<std::string> rhs;
  };
  std::vector<production> productions;
  std::map<std::string, std::string> denormalize_map;
  std::map<int, std::string> production_names;

  inline std::string denormalize_name(std::string const& normalized) const { return denormalize_map.at(normalized); }
  inline std::string production_name(int prod) const { return production_names.at(prod); }
};

using language_ptr = std::shared_ptr<language>;

grammar_ptr build_grammar(language const& language);

finite_automaton build_lexer(language const& language);

parser_tables_ptr build_parser_tables(language const& language);

std::ostream& operator<<(std::ostream& os, language const& lang);

}  // namespace parsegen

#endif
