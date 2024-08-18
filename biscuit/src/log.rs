use serde::Deserialize;

#[derive(Deserialize)]
pub struct Configuration {
	backup: Backup,
	database: Database,
	key: Key,
	log: Log,
	sources: Vec<Source>
}

#[derive(Deserialize)]
struct Backup {
	block_size: u16,
	checksum: String,
	strategy: String
}

#[derive(Deserialize)]
struct Database {
	driver: String,
	path: Option<String>
}

#[derive(Deserialize)]
struct Key {
	path: String
}

#[derive(Deserialize)]
struct Log {
	levels: LogLevels,
	path: String
}

#[derive(Deserialize)]
struct LogLevels {
	core: String,
	database: String,
	ssh: String
}

#[derive(Deserialize)]
struct Source {
	exclude: Option<Vec<String>>,
	exclude_pattern: Option<Vec<String>>,
	include_pattern: Option<Vec<String>>,
	optoins: Option<SourceOption>,
	path: String
}

#[derive(Deserialize)]
struct SourceOption {
	exclude_other_filesystem: Option<bool>,
	exclude_if_present: Option<Vec<String>>
}

pub enum LoggerInitError {
	IoError(std::io::Error),
	YAML(serde_yml::Error)
}

pub fn load_logger(filename: &String) -> Result<Configuration, LoggerInitError> {
	use std::fs::File;

	let reader = File::open(filename);
	if reader.is_err() {
		return Err(LoggerInitError::IoError(reader.err().unwrap()));
	}

	let config = serde_yml::from_reader::<File,Configuration>(reader.unwrap());

	if config.is_err() {
		return Err(LoggerInitError::YAML(config.err().unwrap()))
	}

	Ok(config.ok().unwrap())
}
