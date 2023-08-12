CREATE TABLE keys (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    fingerprint BLOB NOT NULL UNIQUE,
    length INTEGER NOT NULL CHECK (length > 0),
    created INTEGER NOT NULL,
    expire_at INTEGER
);

CREATE TABLE blocks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
    hash BLOB NOT NULL,
    data BLOB NOT NULL
);

CREATE TABLE files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
    hash BLOB NOT NULL,
    data BLOB NOT NULL
);

CREATE TABLE files2blocks (
    file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
    block INTEGER NOT NULL REFERENCES blocks(id) ON UPDATE CASCADE ON DELETE CASCADE,
    sequence INTEGER NOT NULL CHECK (sequence >= 0)
);

CREATE TABLE backups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    start_time INTEGER NOT NULL,
    end_time INTEGER,
    size INTEGER CHECK (size >= 0),
    increment_size INTEGER CHECK (increment_size >= 0),
    parent_backup INTEGER REFERENCES backups(id) ON UPDATE CASCADE ON DELETE SET NULL
);

CREATE TABLE metadata (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    hash_algo TEXT NOT NULL CHECK (hash_algo IN ('md5', 'sha1', 'sha256', 'sha512')),
    hash BLOB NOT NULL,
    data BLOB NOT NULL
);

CREATE TABLE backups2files (
    backup INTEGER NOT NULL REFERENCES backups(id) ON UPDATE CASCADE ON DELETE CASCADE,
    file INTEGER NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
    metadata INTEGER NOT NULL REFERENCES metadata(id) ON UPDATE CASCADE ON DELETE CASCADE
);