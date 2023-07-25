#!/usr/bin/env bash

set -e

if [ -z "$ELASTIC_USER" ] || [ -z "$ELASTIC_PASSWORD" ] || [ -z "$SHARD_COUNT" ] || [ -z "$ES_PORT" ] || [ -z "$ES_RESULT_WINDOW" ] || [ -z "$REPLICA_COUNT" ]; then
  echo "ERROR: Required environment variables not set."
  exit 1
fi

es_user="$ELASTIC_USER"
es_password="$ELASTIC_PASSWORD"
es_host="$ELASTIC_HOST"
es_port=$ES_PORT
shard_count=$SHARD_COUNT
es_inner_window=$ES_RESULT_WINDOW
replica_count=$REPLICA_COUNT
index="fingerprint"
lucene="$index"
url="https://$es_user:$es_password@$es_host:$es_port/$lucene"

if curl -k -u "$es_user:$es_password" -X GET "https://$es_host:$es_port/_cat/indices?v" | grep -q "$index"; then
  echo "Index exists"
  exit 0
fi

index_query="{\"settings\" : {\"number_of_shards\" : $shard_count, \"number_of_replicas\" : $replica_count}}"

if curl -k -u "$es_user:$es_password" -H "Content-Type: application/json" -X PUT -d "$index_query" "$url" | grep -q "acknowledged"; then
  echo "Index created successfully"
else
  echo "Failed to create index"
  exit 1
fi

mapping_query="{\"properties\": {\"hash\":{\"type\": \"keyword\"},\"timestamp\": {\"type\": \"unsigned_long\"},\"song_id\":{\"type\": \"unsigned_long\"}}}"

if curl -k -u "$es_user:$es_password" -H "Content-Type: application/json" -X PUT -d "$mapping_query" "$url/_mapping" | grep -q "acknowledged"; then
  echo "Mapping created successfully"
fi
  echo "Failed to create mapping"
exit 1

if curl -k -u "$es_user:$es_password" -X PUT "$url/_settings" -H "Content-Type: application/json" -d "{\"index.max_inner_result_window\": $es_inner_window}" | grep -q "acknowledged"; then
  echo "max_inner_result_window updated successfully"
  exit 0
fi
  echo "Failed to set max_inner_result_window"
exit 1
