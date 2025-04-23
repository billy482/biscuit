-- total size of one backup
SELECT SUM(LENGTH(data)) AS size FROM blocks WHERE id IN (SELECT block FROM files2blocks WHERE file IN (SELECT file FROM backups2files WHERE backup = 24))

-- diff with previous backup
SELECT SUM(LENGTH(data)) AS diff FROM blocks WHERE id IN (SELECT block FROM files2blocks nb WHERE file IN (SELECT file FROM backups2files WHERE backup = 24) AND NOT EXISTS (SELECT 1 FROM files2blocks ob WHERE nb.block = ob.block AND ob.file IN (SELECT file FROM backups2files WHERE backup = 23)));
