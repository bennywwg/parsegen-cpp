#pragma once

#include <vector>
#include <map>
#include <string>

namespace parsegen {
	struct frontend_info {
		std::map<int, std::string>			production_debug_info;
		std::map<std::string, std::string>	denormalize_token_names;
		std::vector<std::string>			denormalize_production_names;

		std::string get_debug_info(int prod) const;
		std::string denormalize_token_name(std::string const& normalized) const;
		std::string denormalize_production_name(int prod) const;
	};
}