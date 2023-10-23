#ifndef __BISCUIT_SOURCE_SOURCE_HPP__
#define __BISCUIT_SOURCE_SOURCE_HPP__

namespace YAML {
	class Node;
}

class QIODevice;

namespace Biscuit {
	namespace Source {
		class FileInfo;

		class Source {
			public:
				static Source * first_source();
				virtual QIODevice * open(const FileInfo& file) = 0;
				virtual FileInfo next() = 0;
				inline Source * next_source() {
					return this->m_next;
				}
				static bool parse(const YAML::Node& node);
				inline Source * previous_source() {
					return this->m_previous;
				}

			protected:
				Source() = default;
				virtual ~Source();

			private:
				static Source * ms_first;
				static Source * ms_last;
				Source * m_next = nullptr;
				Source * m_previous = nullptr;
		};
	}
}

#endif