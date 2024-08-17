use clap::{Parser,Subcommand};

/// Backup utility
#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Cli {
	#[command(subcommand)]
	command: Commands
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

	match args.command {
		Commands::Backup => {},
		Commands::Restore => {}
	}
}
