#include "frontend_info.hpp"

namespace parsegen {
	std::string frontend_info::get_debug_info(int prod) const {
		auto it = production_debug_info.find(prod);
		if (it == production_debug_info.end()) {
			return "Unknwn Rule " + std::to_string(prod);
		}
		else {
			return it->second;
		}
	}
	std::string frontend_info::denormalize_token_name(std::string const& normalized) const {
		return normalized;
		//return denormalize_token_names.at(normalized);
	}

	std::string frontend_info::denormalize_production_name(int prod) const {
		return std::to_string(prod);
		//return denormalize_production_names[prod];
	}
}