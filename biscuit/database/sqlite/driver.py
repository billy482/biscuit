# -*- coding: utf-8 -*-

import logging
import sqlite3
from typing import Optional
from ..connection import Connection
from ..driver import Driver

ConnectionOptional = Optional[Connection]

class SQLiteDriver(Driver):
	def __init__(self, config: dict):
		super().__init__()
		self._path = config['path'].get()

	def connect(self) -> ConnectionOptional:
		logger = logging.getLogger('biscuit.database')
		logger.info(f"[SQlite] Using SQLite database (version: {sqlite3.sqlite_version})")

		try:
			logger.debug(f"Opening SQLite database at {self._path}")
			connect = sqlite3.connect(self._path)

		except sqlite3.Error as e:
			logger.error(f"[SQlite] Error while opening database because {e}")
			return None

		try:
			cursor = connect.cursor()
			cursor.execute("SELECT * FROM configuration")
			cursor.fetchone()

		except sqlite3.Error as e:
			logger.info(f"[SQlite] Creating SQLite database at {self._path}")
			if not self._create_db(cursor):
				logger.error(f"[SQlite] Error while creating database")
				return None
		logger.info("[SQlite] Connected to SQLite database")

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
					host INTEGER NULL REFERENCES host(id) ON UPDATE CASCADE ON DELETE RESTRICT
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
					hash BLOB NOT NULL, data BLOB NOT NULL
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

		logger = logging.getLogger('biscuit.database')
		for query in queries:
			try:
				connection.execute(query)

			except sqlite3.Error as e:
				logger.error(f"[SQlite] Error while creating database because {e}")
				return False

		return True
