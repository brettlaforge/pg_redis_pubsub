# Redis Publish PostgreSQL Extension

This PostgreSQL extension allows you to connect to Redis and publish messages on a Redis channel from PostgreSQL.

## Requirements

To build you must have:

- PostgreSQL 9.1+
- HIREDIS C Client

#### HIREDIS

The [HIREDIS C client library](https://github.com/redis/hiredis) is a minimalistic C client library for the Redis database.

On Ubuntu 12.04+, it can be installed using the apt-get package [libhiredis-dev](https://launchpad.net/ubuntu/+source/hiredis).

```sh
sudo apt-get install libhiredis-dev
```

## Installation

This code is a standard PostgreSQL extension so all you need to run is:

```sh
make clean install
```

And then in the database:

```sql
CREATE EXTENSION IF NOT EXISTS redis;
```

## Usage

**redis_status**()

Returns the status of the underlying Redis client.

**redis_publish**(*channel* text, *message* text)

Publishes a message on the channel provided.

If there is no underlying Redis client currently connected, it will first create a new connection before attempting to publish the message.

## Roadmap

- Add custom connection configuration support. Maybe use postgresql.conf settings to setup? Use connection string?
- Add a pool of Redis connections to use? Round-robin?
- Add subscribe and psubscribe support that executes a PostgreSQL function?

## Examples

### Basic Example

In Redis:

```
subscribe mychannel
```

In PostgreSQL:

```sql
CREATE EXTENSION IF NOT EXISTS redis;

SELECT redis_status(); -- Disconnected

SELECT redis_publish('mychannel', 'Hello World');

SELECT redis_status(); -- Connected
```

### Trigger Example

In Redis:

```
psubscribe users:*
```

In PostgreSQL:

```sql
CREATE EXTENSION IF NOT EXISTS redis;

CREATE TABLE IF NOT EXISTS users (
    id serial,
    name varchar(255)
);

CREATE OR REPLACE FUNCTION after_change()
    RETURNS TRIGGER AS
    $$
        DECLARE
            channel text;
            message json;
        BEGIN
            channel = 'users:' || NEW.id::text;
            message = to_jsonb(NEW);

            PERFORM redis_publish('users:, message::text);
            RETURN NULL;
        END;
    $$
    LANGUAGE plpgsql;

CREATE TRIGGER users_after_change
    AFTER INSERT OR UPDATE ON users
    FOR EACH ROW
    EXECUTE PROCEDURE after_change();

INSERT INTO users (name) VALUES ('Alice'), ('Bob');

UPDATE users SET name = 'Robert' WHERE name = 'Bob';
```