use serde::Deserialize;
use spdlog::Logger;
use std::sync::Mutex;

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
	IO(std::io::Error),
	SpdLog(spdlog::Error),
	Yaml(serde_yml::Error)
}


static CORE_LOG : Mutex<Option<Logger>> = Mutex::new(Option::None);
static DATABASE_LOG : Mutex<Option<Logger>> = Mutex::new(Option::None);
static SSH_LOG : Mutex<Option<Logger>> = Mutex::new(Option::None);

fn convert_log_level(level: &str) -> spdlog::LevelFilter {
	use spdlog::{
		Level,
		LevelFilter
	};

	match level {
		"critical" => LevelFilter::MoreSevereEqual(Level::Critical),
		"debug" => LevelFilter::MoreSevereEqual(Level::Debug),
		"error" => LevelFilter::MoreSevereEqual(Level::Error),
		"info" => LevelFilter::MoreSevereEqual(Level::Info),
		"trace" => LevelFilter::MoreSevereEqual(Level::Trace),
		"warn" => LevelFilter::MoreSevereEqual(Level::Warn),
		_ => spdlog::LevelFilter::Off
	}
}

pub fn load_logger(filename: &String) -> Result<Configuration, LoggerInitError> {
	use spdlog::{
		Logger,
		sink::FileSink
	};
	use std::{
		fs::File,
		sync::Arc
	};

	let reader = File::open(filename);
	if reader.is_err() {
		return Err(LoggerInitError::IO(reader.err().unwrap()));
	}

	let config_rst = serde_yml::from_reader::<File,Configuration>(reader.unwrap());
	if config_rst.is_err() {
		return Err(LoggerInitError::Yaml(config_rst.err().unwrap()))
	}

	let config = config_rst.ok().unwrap();

	let file_sink_rst = FileSink::builder()
		.path(&config.log.path)
		.build();
	if file_sink_rst.is_err() {
		return Err(LoggerInitError::SpdLog(file_sink_rst.err().unwrap()));
	}

	let file_sink = Arc::new(file_sink_rst.ok().unwrap());

	let core_logger_rst = Logger::builder()
		.name("core")
		.level_filter(convert_log_level(&config.log.levels.core))
		.sink(file_sink.clone())
		.build();
	if core_logger_rst.is_err() {
		return Err(LoggerInitError::SpdLog(core_logger_rst.err().unwrap()));
	}

	let mut core_log = CORE_LOG.lock().unwrap();
	*core_log = Some(core_logger_rst.ok().unwrap());

	let database_logger_rst = Logger::builder()
		.name("database")
		.level_filter(convert_log_level(&config.log.levels.database))
		.sink(file_sink.clone())
		.build();
	if database_logger_rst.is_err() {
		return Err(LoggerInitError::SpdLog(database_logger_rst.err().unwrap()));
	}

	let mut database_log = DATABASE_LOG.lock().unwrap();
	*database_log = Some(database_logger_rst.ok().unwrap());

	let ssh_logger_rst = Logger::builder()
		.name("ssh")
		.level_filter(convert_log_level(&config.log.levels.ssh))
		.sink(file_sink.clone())
		.build();
	if ssh_logger_rst.is_err() {
		return Err(LoggerInitError::SpdLog(ssh_logger_rst.err().unwrap()));
	}

	let mut ssh_log = SSH_LOG.lock().unwrap();
	*ssh_log = Some(ssh_logger_rst.ok().unwrap());

	Ok(config)
}
