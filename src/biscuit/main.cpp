#include <clipp.h>
#include <iostream>

int main(int argc, char * argv[]) {
	using namespace clipp;

	enum class modes {backup, help, restore};
	modes mode = modes::backup;

	auto backup = (
		"backup options" % in_sequence(
			command("backup").set(mode, modes::backup),
			option("-h", "--help").set(mode, modes::help) % "Show help"
		)
	);

	auto help = (
		command("help").set(mode, modes::help)
	);

	auto restore = (
		"restore options" % in_sequence(
			command("restore").set(mode, modes::restore),
			option("-h", "--help").set(mode, modes::help) % "Show help"
		)
	);

	auto cli = (
		(backup | help | restore)
	);

	if (parse(argc, argv, cli)) {
		switch (mode) {
			case modes::backup:
				break;

			case modes::help:
				std::cout << make_man_page(cli, "biscuit");
				break;

			case modes::restore:
				break;
		}
	} else
		std::cout << make_man_page(cli, "biscuit");

	return 0;
}