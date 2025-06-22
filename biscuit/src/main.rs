use clap::{Arg, Command};

mod commands;

fn main() {
	let command = Command::new("biscuit")
		.version("1.0")
		.author("Guillaume Clercin <guillaume.clercin@billy482.net>")
		.about("A backup tool")
		.arg(
			Arg::new("config")
				.short('c')
				.long("config")
				.value_name("FILE")
				.default_value("biscuit.yaml")
				.help("Specify alternative configuration file"),
		);
	commands::parse_args(&command);

	let matches = command.get_matches();

	if let Some(config) = matches.get_one::<String>("config") {
		println!("Using config file: {}", config);
	}
}
