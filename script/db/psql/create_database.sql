CREATE EXTENSION "pgcrypto";
CREATE EXTENSION "uuid-ossp";

CREATE ROLE admin;
CREATE ROLE api;
CREATE ROLE db_maintainer;

-- Continue with db_maintainer user
-- or a member of db_maintainer
-- or a superuser
GRANT db_maintainer TO guillaume;

SET ROLE db_maintainer;


CREATE TABLE keys (
	id SERIAL PRIMARY KEY,
	fingerprint BYTEA NOT NULL UNIQUE,
	length INT NOT NULL CHECK (length > 0),
	created TIMESTAMPTZ NOT NULL,
	expire_at TIMESTAMPTZ
);

GRANT SELECT, DELETE ON TABLE keys TO admin;
GRANT SELECT, INSERT, DELETE ON TABLE keys TO api;
GRANT UPDATE (expire_at) ON TABLE keys TO admin, api;
GRANT USAGE ON SEQUENCE keys_id_seq TO api;


CREATE TABLE devices (
	id SERIAL PRIMARY KEY,
	uuid UUID NOT NULL DEFAULT uuid_generate_v4() UNIQUE,
	hostname TEXT NOT NULL UNIQUE,
	description TEXT NOT NULL,
	key INT NOT NULL REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT,
	UNIQUE (uuid, key)
);

GRANT SELECT, DELETE ON TABLE devices TO admin;
GRANT SELECT, INSERT, DELETE ON TABLE devices TO api;
GRANT UPDATE (description, key) ON TABLE devices TO admin, api;
GRANT USAGE ON SEQUENCE devices_id_seq TO api;


CREATE TYPE hash_algo AS ENUM (
	'md5',
	'sha1',
	'sha256',
	'sha512'
);


CREATE TABLE blocks (
	id BIGSERIAL PRIMARY KEY,
	algo hash_algo NOT NULL,
	hash BYTEA NOT NULL,
	data BYTEA NOT NULL,
	key INTEGER NOT NULL REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT
);
-- CREATE UNIQUE INDEX blocks_hash_?_idx ON blocks(algo, hash) WHERE key = ?

COMMENT ON COLUMN blocks.hash IS 'digest of unencrypted data';

GRANT SELECT, INSERT ON TABLE blocks TO api;
GRANT UPDATE (id) ON TABLE blocks TO api;
GRANT USAGE ON SEQUENCE blocks_id_seq TO api;


CREATE TABLE host (
	id SERIAL PRIMARY KEY,
	hostname TEXT NOT NULL
);
GRANT SELECT, INSERT ON TABLE host TO api;
GRANT USAGE ON SEQUENCE host_id_seq TO api;

CREATE TABLE files (
	id BIGSERIAL PRIMARY KEY,
	path TEXT NOT NULL,
	last_modified TIMESTAMPTZ NOT NULL,
	key INTEGER NOT NULL REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT,
	host INTEGER NULL REFERENCES host(id) ON UPDATE CASCADE ON DELETE RESTRICT
);
-- CREATE UNIQUE INDEX files_hash_?_idx ON files(algo, hash, previous_version) WHERE key = ?

GRANT SELECT, INSERT ON TABLE files TO api;
GRANT UPDATE (id) ON TABLE files TO api;
GRANT USAGE ON SEQUENCE files_id_seq TO api;


CREATE TABLE files2blocks (
	file BIGINT NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
	block BIGINT NOT NULL REFERENCES blocks(id) ON UPDATE CASCADE ON DELETE CASCADE,
	sequence INTEGER NOT NULL CHECK (sequence >= 0)
);

GRANT SELECT, INSERT ON TABLE files2blocks TO api;


CREATE TABLE backups (
	id BIGSERIAL PRIMARY KEY,
	start_time TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
	end_time TIMESTAMP WITH TIME ZONE,
	size BIGINT NULL CHECK (size >= 0),
	increment_size BIGINT NULL CHECK (increment_size >= 0),
	device INTEGER NOT NULL REFERENCES devices(id) ON UPDATE CASCADE ON DELETE CASCADE,
	parent_backup BIGINT NULL REFERENCES backups(id) ON UPDATE CASCADE ON DELETE SET NULL,
	deleted BOOLEAN NOT NULL DEFAULT FALSE
);
-- CREATE INDEX ON "Backups"(id, parent_backup) WHERE device = ?;

GRANT SELECT, INSERT ON TABLE backups TO api;
GRANT UPDATE (end_time, deleted) ON TABLE backups TO api;
GRANT USAGE ON SEQUENCE backups_id_seq TO api;


CREATE TABLE metadata (
	id BIGSERIAL PRIMARY KEY,
	algo hash_algo NOT NULL,
	hash BYTEA NOT NULL,
	data BYTEA NOT NULL,
	key INTEGER NOT NULL REFERENCES keys(id) ON UPDATE CASCADE ON DELETE RESTRICT
);
COMMENT ON COLUMN metadata.hash IS 'digest of unencrypted data';
-- CREATE UNIQUE INDEX metadata_hash_?_idx ON metadata(algo, hash) WHERE key = ?

GRANT SELECT, INSERT ON TABLE metadata TO api;
GRANT UPDATE (id) ON TABLE metadata TO api;
GRANT USAGE ON SEQUENCE metadata_id_seq TO api;


CREATE TABLE backups2files (
	backup BIGINT NOT NULL REFERENCES backups(id) ON UPDATE CASCADE ON DELETE CASCADE,
	file BIGINT NOT NULL REFERENCES files(id) ON UPDATE CASCADE ON DELETE CASCADE,
	metadata BIGINT NOT NULL REFERENCES metadata(id) ON UPDATE CASCADE ON DELETE RESTRICT
);

GRANT SELECT, INSERT ON TABLE backups2files TO api;


CREATE PROCEDURE maintain_db(max_execution_time INTERVAL DEFAULT '4min')
LANGUAGE plpgsql
AS $$
DECLARE
	start_procedure TIMESTAMPTZ NOT NULL := clock_timestamp();
	cur_old_indexes CURSOR FOR SELECT name FROM (SELECT indexrelname AS name, (regexp_match(indexrelname, '[0-9]+'))[1]::INTEGER AS key_id FROM pg_stat_all_indexes WHERE indexrelname ~ ANY (ARRAY['blocks_hash_[0-9]+_idx', 'files_hash_[0-9]+_idx', 'metadata_hash_[0-9]+_idx'])) AS sub WHERE NOT EXISTS (SELECT 1 FROM keys WHERE id = sub.key_id);
	rec_old_indexes RECORD;
	cur_new_indexes CURSOR FOR SELECT id FROM keys;
	rec_new_indexes RECORD;
	found_index BOOLEAN;
	cur_account CURSOR FOR SELECT * FROM accounts ORDER BY id;
	rec_account RECORD;
	size_used BIGINT NOT NULL := 0;
	cur_backup CURSOR FOR SELECT * FROM backups ORDER BY id;
	rec_backup RECORD;
	backup_size BIGINT NOT NULL := 0;
	cur_device CURSOR FOR SELECT k.id AS key, ARRAY_AGG(d.id) AS devs FROM keys k LEFT JOIN devices d ON k.id = d.key GROUP BY k.id ORDER BY random();
	rec_device RECORD;
	found_session BOOLEAN NOT NULL := FALSE;
	cur_metadata CURSOR (dev_key INTEGER) FOR SELECT * FROM metadata m WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM backups2files WHERE metadata = m.id) FOR UPDATE;
	rec_metadata RECORD;
	cur_file CURSOR (dev_key INTEGER) FOR SELECT * FROM files f WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM backups2files WHERE file = f.id) FOR UPDATE;
	rec_file RECORD;
	cur_block CURSOR (dev_key INTEGER) FOR SELECT * FROM blocks b WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM files2blocks WHERE block = b.id) FOR UPDATE;
	rec_block RECORD;
BEGIN
	-- delete old sessions
	DELETE FROM sessions WHERE expire_at < NOW();
	COMMIT;

	IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;

	-- drop old indexes
	FOR rec_old_indexes IN cur_old_indexes LOOP
		EXECUTE 'DROP INDEX ' || rec_old_indexes.name;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
	END LOOP;

	-- create new indexes
	FOR rec_new_indexes IN cur_new_indexes LOOP
		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'blocks_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX blocks_hash_' || rec_new_indexes.id || '_idx ON blocks(algo, hash) WHERE key = ' || rec_new_indexes.id;
		END IF;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;

		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'files_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX files_hash_' || rec_new_indexes.id || '_idx ON files(algo, hash, previous_version) WHERE key = ' || rec_new_indexes.id;
		END IF;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;

		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'metadata_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX metadata_hash_' || rec_new_indexes.id || '_idx ON metadata(algo, hash) WHERE key = ' || rec_new_indexes.id;
		END IF;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
	END LOOP;

	-- update accounts
	FOR rec_account IN cur_account LOOP
		SELECT COALESCE(SUM(octet_length(data)), 0) INTO size_used FROM blocks WHERE key IN (SELECT id FROM keys WHERE account = rec_account.id);
		UPDATE accounts SET disk_used = size_used WHERE CURRENT OF cur_account;
		COMMIT;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
	END LOOP;

	-- update backups
	FOR rec_backup IN cur_backup LOOP
		IF rec_backup.deleted THEN
			IF rec_backup.parent_backup IS NOT NULL THEN
				UPDATE backups SET increment_size = NULL, parent_backup = rec_backup.parent_backup WHERE parent_backup = rec_backup.id;
			END IF;

			DELETE FROM backups WHERE CURRENT OF cur_backup;
		ELSIF rec_backup.end_time IS NOT NULL THEN
			IF rec_backup.size IS NULL THEN
				-- Compute backup size
				SELECT COALESCE(SUM(octet_length(data)), 0) INTO backup_size FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = rec_backup.id));
				UPDATE backups SET size = backup_size WHERE CURRENT OF cur_backup;
			END IF;

			IF rec_backup.increment_size IS NULL AND rec_backup.parent_backup IS NOT NULL THEN
				-- Compute backup incremental size
				SELECT COALESCE(SUM(octet_length(data)), 0) INTO backup_size FROM blocks
				WHERE id IN (
					SELECT block FROM files2blocks nb
					WHERE file IN (SELECT file FROM backups2files WHERE backup = rec_backup.id)
					AND NOT EXISTS (
						SELECT 1 FROM files2blocks ob WHERE nb.block = ob.block AND ob.file IN (
							SELECT file FROM backups2files WHERE backup = rec_backup.parent_backup)));

				UPDATE backups SET increment_size = backup_size WHERE CURRENT OF cur_backup;
			END IF;
		END IF;

		IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
	END LOOP;
	COMMIT;

	-- Garbage collector
	<<loop_device>>
	FOR rec_device IN cur_device LOOP
		SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
		CONTINUE WHEN found_session;

		FOR rec_metadata IN cur_metadata(rec_device.key) LOOP
			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			CONTINUE loop_device WHEN found_session;

			DELETE FROM metadata WHERE CURRENT OF cur_metadata;

			IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
		END LOOP;

		FOR rec_file IN cur_file(rec_device.key) LOOP
			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			CONTINUE loop_device WHEN found_session;

			DELETE FROM files WHERE CURRENT OF cur_file;
			IF rec_file.previous_version IS NOT NULL THEN
				UPDATE files SET previous_version = rec_file.previous_version WHERE previous_version = rec_file.id;
			END IF;

			IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
		END LOOP;

		FOR rec_block IN cur_block(rec_device.key) LOOP
			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			CONTINUE loop_device WHEN found_session;

			DELETE FROM blocks WHERE CURRENT OF cur_block;

			IF clock_timestamp() - start_procedure > max_execution_time THEN RETURN; END IF;
		END LOOP;

		COMMIT;
	END LOOP loop_device;
END $$;

-- REASSIGN OWNED BY CURRENT_USER TO db_maintainer;
-- CREATE ROLE "www-data" WITH LOGIN IN ROLE api;
