# -*- coding: utf-8 -*-

from biscuit.database import Connection, Driver
import logging
import sqlite3

class SQLiteDriver(Driver):
	def __init__(self, config: dict):
		super().__init__()
		self._path = config['path'].get()

	def connect(self) -> Connection:
		logger = logging.getLogger('biscuit.database')
		logger.info(f"[SQLite] Using SQLite database (version: {sqlite3.sqlite_version})")

		try:
			logger.debug(f"[SQLite] Opening SQLite database at {self._path}")
			connect = sqlite3.connect(self._path, isolation_level=None, check_same_thread=False)

		except sqlite3.Error as e:
			logger.error(f"[SQLite] Error while opening database because {e}")
			raise

		try:
			cursor = connect.cursor()
			cursor.execute("SELECT * FROM configuration")
			cursor.fetchone()

		except sqlite3.Error as e:
			logger.info(f"[SQLite] Creating SQLite database at {self._path}")
			if not self._create_db(cursor):
				logger.error(f"[SQLite] Error while creating database")

		logger.info("[SQLite] Connected to SQLite database")

		from .connection import SQLiteConnection
		return SQLiteConnection(connect, self)

	def _create_db(self, connection: sqlite3.Cursor) -> bool:
		"""
		Create the database.
		"""

		queries = [
			"PRAGMA journal_mode=WAL",
			"""
				CREATE TABLE keys (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					fingerprint BLOB NOT NULL UNIQUE,
					hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
					length INTEGER NOT NULL CHECK (length > 0),
					first_use INTEGER NOT NULL DEFAULT (unixepoch()),
					last_use INTEGER NOT NULL DEFAULT (unixepoch())
				)
			""",
			"""
				CREATE TABLE blocks (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
					hash BLOB NOT NULL,
					data BLOB NOT NULL,
					key INTEGER REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT
				)
			""",
			"""
				CREATE TABLE hosts (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					hostname TEXT NOT NULL
				)
			""",
			"""
				CREATE TABLE files (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					path TEXT NOT NULL,
					last_modified INTEGER NOT NULL DEFAULT (unixepoch()),
					host INTEGER NULL REFERENCES hosts(id) ON UPDATE CASCADE ON DELETE RESTRICT
				)
			""",
			"""
				CREATE TABLE files2blocks (
					file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
					block INTEGER NOT NULL REFERENCES blocks(id) ON UPDATE CASCADE ON DELETE CASCADE,
					sequence INTEGER NOT NULL CHECK (sequence >= 0)
				)
			""",
			"""
				CREATE TABLE backups (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					start_time INTEGER NOT NULL DEFAULT (unixepoch()),
					end_time INTEGER,
					size INTEGER CHECK (size >= 0),
					increment_size INTEGER CHECK (increment_size >= 0),
					parent_backup INTEGER REFERENCES backups(id) ON UPDATE CASCADE ON DELETE SET NULL
				)
			""",
			"""
				CREATE TABLE metadata (
					id INTEGER PRIMARY KEY AUTOINCREMENT,
					hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
					hash BLOB NOT NULL,
					data BLOB NOT NULL,
					key INTEGER REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT
				)
			""",
			"""
				CREATE TABLE backups2files (
					backup INTEGER NOT NULL REFERENCES backups(id) ON UPDATE CASCADE ON DELETE CASCADE,
					file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
					metadata INTEGER NOT NULL REFERENCES metadata(id) ON UPDATE CASCADE ON DELETE CASCADE
				)
			""",
			"""
				CREATE TABLE configuration (
					key TEXT PRIMARY KEY,
					value TEXT NULL
				)
			""",
			"INSERT INTO configuration VALUES ('db_version', '1')"
		]

		for query in queries:
			try:
				connection.execute(query)

			except sqlite3.Error as e:
				logger = logging.getLogger('biscuit.database')
				logger.error(f"[SQLite] Error while creating database with query: {query} because {e}")
				raise
