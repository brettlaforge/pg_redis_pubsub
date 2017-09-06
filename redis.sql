-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION redis" to load this file. \quit

CREATE FUNCTION redis_status()
RETURNS text
AS '$libdir/redis'
LANGUAGE C;

CREATE FUNCTION redis_publish(channel text, message text)
RETURNS void
AS '$libdir/redis'
LANGUAGE C;