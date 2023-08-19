#ifndef __BISCUIT_SOURCE_HPP__
#define __BISCUIT_SOURCE_HPP__

#include <filesystem>
#include <list>
#include <string>

namespace YAML {
	class Node;
}

namespace Biscuit {
	class Source {
		public:
			Source(const std::string& path);

			void parse(const YAML::Node& node);

		private:
			std::filesystem::path m_path;
			std::list<std::string> m_include_pattern;
			std::list<std::string> m_exclude_pattern;
			std::list<std::string> m_exclude_path;
	};
}

#endif