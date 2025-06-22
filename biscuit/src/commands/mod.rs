use clap::Command;

mod config;

pub fn parse_args(command: &Command) {
	config::parse_config_args(command);
}
