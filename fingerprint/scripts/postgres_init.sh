#!/bin/bash

set -e

if [ -z "$POSTGRES_USER" ] || [ -z "$POSTGRES_PASSWORD" ] || [ -z "$POSTGRES_PORT" ] || [ -z "$POSTGRES_HOST" ] || [ -z "$POSTGRES_DB_NAME" ]; then
  echo "ERROR: Required environment variables not set."
  exit 1
fi

db_name=${POSTGRES_DB_NAME}
pg_host=${POSTGRES_HOST}
table_name="fingerprint"
index_name="fp_index"

test_db_sql="SELECT 1 FROM pg_database WHERE datname = '${db_name}'"
create_db_sql="CREATE DATABASE ${db_name}"

is_success=$(PGPASSWORD="${POSTGRES_PASSWORD}" psql -d "host=$pg_host port=${POSTGRES_PORT} dbname=${db_name} user=${POSTGRES_USER}" -tc "${test_db_sql}" | grep -q "1" && echo true || echo false)

if [ "$is_success" = false ]; then
  echo "Database ${db_name} does not exist, creating a new one"
  PGPASSWORD="${POSTGRES_PASSWORD}" psql -d "host=$pg_host port=${POSTGRES_PORT} dbname=${db_name} user=${POSTGRES_USER}" -c "${create_db_sql}"
fi

CREATE_TABLE_SQL="CREATE TABLE IF NOT EXISTS ${table_name} (
                  hash               NUMERIC(20),
                  song_id            BIGINT,
                  timestamp          INT
                  );"

CREATE_INDEX_SQL="CREATE UNIQUE INDEX IF NOT EXISTS ${index_name} ON ${table_name} USING btree(hash, song_id) WITH (fillfactor=100);"

PGPASSWORD="${POSTGRES_PASSWORD}" psql -d "host=$pg_host port=${POSTGRES_PORT} dbname=${db_name} user=${POSTGRES_USER}" -c "${CREATE_TABLE_SQL}" -c "${CREATE_INDEX_SQL}"
