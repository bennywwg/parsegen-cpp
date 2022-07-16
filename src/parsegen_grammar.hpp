#ifndef PARSEGEN_GRAMMAR_HPP
#define PARSEGEN_GRAMMAR_HPP

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace parsegen {

/* convention: symbols are numbered with all
   terminal symbols first, all non-terminal symbols after */

struct grammar {
  using right_hand_side = std::vector<int>;
  struct production {
    int lhs;
    right_hand_side rhs;
  };
  using production_vector = std::vector<production>;
  int nsymbols;
  int nterminals;
  production_vector productions;
  std::vector<std::string> symbol_names;
  std::map<std::string, std::string> denormalize_token_names;
  std::map<int, std::string> denormalize_production_names;

  inline std::string denormalize_token_name(std::string const& normalized) const { return denormalize_token_names.at(normalized); }
  inline std::string denormalize_production_name(int prod) const { return denormalize_production_names.at(prod); }
};

using grammar_ptr = std::shared_ptr<grammar const>;

int get_nnonterminals(grammar const& g);
bool is_terminal(grammar const& g, int symbol);
bool is_nonterminal(grammar const& g, int symbol);
int as_nonterminal(grammar const& g, int symbol);
int find_goal_symbol(grammar const& g);
void add_end_terminal(grammar& g);
int get_end_terminal(grammar const& g);
void add_accept_production(grammar& g);
int get_accept_production(grammar const& g);
int get_accept_nonterminal(grammar const& g);

std::ostream& operator<<(std::ostream& os, grammar const& g);

}  // namespace parsegen

#endif
