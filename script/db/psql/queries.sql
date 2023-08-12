-- Compute increment size between two backups
PREPARE backup_full_size AS
SELECT COUNT(*) AS total, COALESCE(SUM(octet_length(data)), 0) AS diff FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = $1));

PREPARE backup_incr_size AS
SELECT COUNT(*) AS total, COALESCE(SUM(octet_length(data)), 0) AS diff
FROM blocks
WHERE id IN (
	SELECT block
	FROM files2blocks nb
	WHERE file IN (
		SELECT file
		FROM backups2files
		WHERE backup = $2)
	AND NOT EXISTS (
		SELECT 1
		FROM files2blocks ob
		WHERE nb.block = ob.block AND ob.file IN (
			SELECT file
			FROM backups2files
			WHERE backup = $1)));

DO $$
DECLARE
	cur_backup CURSOR FOR SELECT id, parent_backup FROM backups WHERE size IS NULL;
	rec_backup RECORD;
	backup_size BIGINT;
BEGIN
	OPEN cur_backup;

	LOOP
		FETCH cur_backup INTO rec_backup;
		EXIT WHEN rec_backup IS NULL;

		-- update backup status

		SELECT COALESCE(SUM(octet_length(data)), 0) INTO backup_size FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = rec_backup.id));
		UPDATE backups SET size = backup_size WHERE CURRENT OF cur_backup;

		IF rec_backup.parent_backup IS NOT NULL THEN
			SELECT COALESCE(SUM(octet_length(data)), 0) INTO backup_size FROM blocks
			WHERE id IN (
				SELECT block FROM files2blocks nb
				WHERE file IN (SELECT file FROM backups2files WHERE backup = rec_backup.id)
				AND NOT EXISTS (
					SELECT 1 FROM files2blocks ob WHERE nb.block = ob.block AND ob.file IN (
						SELECT file FROM backups2files WHERE backup = rec_backup.parent_backup)));

			UPDATE backups SET increment_size = backup_size WHERE CURRENT OF cur_backup;
		END IF;
	END LOOP;

	CLOSE cur_backup;
END $$;


-- Usage per user
SELECT a.login, SUM(sub.sum) AS total
FROM accounts a
	INNER JOIN devices d ON a.id = d.account
	INNER JOIN (
		SELECT device, SUM(COALESCE(increment_size, size))
		FROM backups
		GROUP BY device) AS sub ON d.id = sub.device
GROUP BY a.login;


CREATE PROCEDURE maintain_db()
LANGUAGE plpgsql
AS $$
DECLARE
	cur_old_indexes CURSOR FOR SELECT name FROM (SELECT indexrelname AS name, (regexp_match(indexrelname, '[0-9]+'))[1]::INTEGER AS key_id FROM pg_stat_all_indexes WHERE indexrelname ~ ANY (ARRAY['blocks_hash_[0-9]+_idx', 'files_hash_[0-9]+_idx', 'metadata_hash_[0-9]+_idx'])) AS sub WHERE NOT EXISTS (SELECT 1 FROM keys WHERE id = sub.key_id);
	rec_old_indexes RECORD;
	cur_new_indexes CURSOR FOR SELECT id FROM keys;
	rec_new_indexes RECORD;
	found_index BOOLEAN;
	cur_backup CURSOR FOR SELECT * FROM backups ORDER BY id FOR UPDATE;
	rec_backup RECORD;
	backup_size BIGINT;
	cur_device CURSOR FOR SELECT key, ARRAY_AGG(id) AS devs FROM devices GROUP BY key;
	rec_device RECORD;
	found_session BOOLEAN NOT NULL := FALSE;
	cur_metadata CURSOR (dev_key INTEGER) FOR SELECT * FROM metadata m WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM backups2files WHERE metadata = m.id) FOR UPDATE;
	rec_metadata RECORD;
	cur_file CURSOR (dev_key INTEGER) FOR SELECT * FROM files f WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM backups2files WHERE file = f.id) FOR UPDATE;
	rec_file RECORD;
	cur_block CURSOR (dev_key INTEGER) FOR SELECT * FROM blocks b WHERE key = dev_key AND NOT EXISTS (SELECT 1 FROM files2blocks WHERE block = b.id) FOR UPDATE;
	rec_block RECORD;
BEGIN
	-- drop old indexes
	OPEN cur_old_indexes;
	LOOP
		FETCH cur_old_indexes INTO rec_old_indexes;
		EXIT WHEN rec_old_indexes IS NULL;

		EXECUTE 'DROP INDEX ' || rec_old_indexes.name;
	END LOOP;
	CLOSE cur_old_indexes;

	-- create new indexes
	OPEN cur_new_indexes;
	LOOP
		FETCH cur_new_indexes INTO rec_new_indexes;
		EXIT WHEN rec_new_indexes IS NULL;

		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'blocks_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX blocks_hash_' || rec_new_indexes.id || '_idx ON blocks(algo, hash) WHERE key = ' || rec_new_indexes.id;
		END IF;

		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'files_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX files_hash_' || rec_new_indexes.id || '_idx ON files(algo, hash, previous_version) WHERE key = ' || rec_new_indexes.id;
		END IF;

		SELECT EXISTS (SELECT 1 FROM pg_stat_all_indexes WHERE indexrelname = 'metadata_hash_' || rec_new_indexes.id || '_idx') INTO found_index;
		IF NOT found_index THEN
			EXECUTE 'CREATE UNIQUE INDEX metadata_hash_' || rec_new_indexes.id || '_idx ON metadata(algo, hash) WHERE key = ' || rec_new_indexes.id;
		END IF;
	END LOOP;
	CLOSE cur_new_indexes;

	OPEN cur_backup;
	LOOP
		FETCH cur_backup INTO rec_backup;
		EXIT WHEN rec_backup IS NULL;

		IF rec_backup.deleted THEN
			IF rec_backup.parent_backup IS NOT NULL THEN
				UPDATE backups SET increment_size = NULL, parent_backup = rec_backup.parent_backup WHERE parent_backup = rec_backup.id;
			END IF;

			DELETE FROM backups WHERE CURRENT OF cur_backup;
		ELSE
			IF rec_backup.size IS NULL THEN
				-- Compute backup size
				SELECT COALESCE(SUM(octet_length(data)), 0) INTO backup_size FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = rec_backup.id));
				UPDATE backups SET size = backup_size WHERE CURRENT OF cur_backup;
			END IF;

			IF rec_backup.increment_size IS NULL THEN
				-- Compute backup incremental size
				IF rec_backup.parent_backup IS NOT NULL THEN
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
		END IF;
	END LOOP;
	CLOSE cur_backup;

	-- Garbage collector
	OPEN cur_device;
	LOOP
		FETCH cur_device INTO rec_device;
		EXIT WHEN rec_device IS NULL;

		SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
		CONTINUE WHEN found_session;

		OPEN cur_metadata(rec_device.key);
		LOOP
			FETCH cur_metadata INTO rec_metadata;
			EXIT WHEN rec_metadata IS NULL;

			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			EXIT WHEN found_session;

			DELETE FROM metadata WHERE CURRENT OF cur_metadata;
		END LOOP;
		CLOSE cur_metadata;

		EXIT WHEN found_session;

		OPEN cur_file(rec_device.key);
		LOOP
			FETCH cur_file INTO rec_file;
			EXIT WHEN rec_file IS NULL;

			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			EXIT WHEN found_session;

			DELETE FROM files WHERE CURRENT OF cur_file;
			IF rec_file.previous_version IS NOT NULL THEN
				UPDATE files SET previous_version = rec_file.previous_version WHERE previous_version = rec_file.id;
			END IF;
		END LOOP;
		CLOSE cur_file;

		EXIT WHEN found_session;

		OPEN cur_block(rec_device.key);
		LOOP
			FETCH cur_block INTO rec_block;
			EXIT WHEN rec_block IS NULL;

			SELECT EXISTS (SELECT 1 FROM sessions WHERE device = ANY (rec_device.devs)) INTO found_session;
			EXIT WHEN found_session;

			DELETE FROM blocks WHERE CURRENT OF cur_block;
		END LOOP;
		CLOSE cur_block;

		EXIT WHEN found_session;
	END LOOP;
	CLOSE cur_device;
END $$;
