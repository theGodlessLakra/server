FLUSH STATUS;

--error ER_COMPRESSED_COLUMN_USED_AS_KEY
eval CREATE TABLE t1(a $typec, KEY(a(10)));

eval CREATE TABLE t1(a $typec);
--error ER_COMPRESSED_COLUMN_USED_AS_KEY
ALTER TABLE t1 ADD KEY(a(10));
SHOW CREATE TABLE t1;

--echo # Make sure column was actually compressed
INSERT INTO t1 VALUES(REPEAT('a', 1000));
SELECT LEFT(a, 10), LENGTH(a) FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';

--echo # Make sure ALTER TABLE rebuilds table
eval ALTER TABLE t1 MODIFY COLUMN a $typeu;
SHOW CREATE TABLE t1;
SELECT LEFT(a, 10), LENGTH(a) FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';

--echo # Rebuild back
eval ALTER TABLE t1 MODIFY COLUMN a $typec;
SHOW CREATE TABLE t1;
SELECT LEFT(a, 10), LENGTH(a) FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';

--echo # Make sure CREATE TABLE ... LIKE inherits compression
CREATE TABLE t2 LIKE t1;
SHOW CREATE TABLE t2;
DROP TABLE t2;

--echo # Make sure implicit CREATE TABLE ... SELECT inherits compression
CREATE TABLE t2 SELECT * FROM t1;
SHOW CREATE TABLE t2;
SELECT LEFT(a, 10), LENGTH(a) FROM t2;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t2';
DROP TABLE t2;

--echo # Make sure explicit CREATE TABLE ... SELECT doesn't inherit compression
eval CREATE TABLE t2(a $typeu) SELECT * FROM t1;
SHOW CREATE TABLE t2;
SELECT LEFT(a, 10), LENGTH(a) FROM t2;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t2';
DROP TABLE t2;

--echo # Make sure engine change works
ALTER TABLE t1 ENGINE=InnoDB;
SHOW CREATE TABLE t1;
SELECT LEFT(a, 10), LENGTH(a) FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');

--echo # Make sure online add column works (requires InnoDB)
eval ALTER TABLE t1 ADD COLUMN b $typec DEFAULT "must be visible";
SELECT LEFT(a, 10), LENGTH(a), b FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
ALTER TABLE t1 DROP COLUMN b;

ALTER TABLE t1 ENGINE=MyISAM;
TRUNCATE TABLE t1;

--echo # Make sure column_compression_zlib_level works
SET column_compression_zlib_level= 1;
INSERT INTO t1 VALUES(REPEAT('ab', 1000));
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';
TRUNCATE TABLE t1;

SET column_compression_zlib_level= 9;
INSERT INTO t1 VALUES(REPEAT('ab', 1000));
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';

SET column_compression_zlib_level= DEFAULT;
TRUNCATE TABLE t1;

--echo # No compression, original data shorter than compressed
INSERT INTO t1 VALUES('a');
SELECT a, LENGTH(a) FROM t1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
SELECT DATA_LENGTH FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA='test' AND TABLE_NAME='t1';

--echo # Coverage for store(double) and store(longlong)
INSERT INTO t1 VALUES(3.14),(CAST(9.8 AS DOUBLE)),(1),(''),(NULL);
--echo # and for sort_string()
SELECT * FROM t1 ORDER BY a;

--echo # Coverage for val_real() and val_int()
SELECT a+1 FROM t1 ORDER BY 1;
SELECT * FROM INFORMATION_SCHEMA.SESSION_STATUS WHERE VARIABLE_NAME IN('Column_compressions', 'Column_decompressions');
#--exec hd /home/svoj/devel/maria/debug/mysql-test/var/mysqld.1/data/test/t1.MYD

DROP TABLE t1;
