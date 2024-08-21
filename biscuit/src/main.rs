use clap::{Parser,Subcommand};
use log::LoggerInitError;

mod log;

/// Backup utility
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Cli {
	#[command(subcommand)]
	command: Commands,

	/// configuration file
	#[arg(short='f')]
	config: Option<String>
}

#[derive(Debug, Subcommand)]
enum Commands {
	/// Create new backup
	Backup,
	/// Restore a backup
	Restore
}

fn main() {
	let args = Cli::parse();

	let config_file = args.config.unwrap_or("biscuit.yaml".to_string());

	let config = log::load_logger(&config_file);

	if config.is_err() {
		eprintln!("Error while loading configuration file: {}", &config_file);
		match config.err().unwrap() {
			LoggerInitError::IO(err) => eprint!("because of IO error: {}", err),
			LoggerInitError::SpdLog(err) => eprintln!("because of spdlog error: {}", err),
			LoggerInitError::Yaml(err) => eprint!("because of yaml error: {}", err)
		}
		return;
	}

	match args.command {
		Commands::Backup => {},
		Commands::Restore => {}
	}
}
