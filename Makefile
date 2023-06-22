start_fingerprint:
	docker-compose -f docker-compose.yml build fingerprint_serv && \
	docker-compose -f docker-compose.yml up --no-recreate -d fingerprint_serv

prepare_fingerprint_storage:
	# run inside fingerprint_serv
	cd fingerprint/scripts && chmod +x postgres_init.sh && ./postgres_init.sh && \
	chmod +x elastic_init.sh && ./elastic_init.sh

clean_proxy:
	# run inside fingerprint_serv
	rm fingerprint/proxy/gen/proto/*.go

build_proxy:
	# run inside fingerprint_serv
	cd fingerprint/scripts && chmod +x build_proxy.sh && ./build_proxy.sh

start_proxy: build_proxy
	# run inside fingerprint_serv
	cd fingerprint/scripts && chmod +x start_proxy.sh && ./start_proxy.sh

start_metadata:
	docker-compose -f docker-compose.yml build metadata_serv && \
    docker-compose -f docker-compose.yml up --no-recreate -d metadata_serv

start_nginx:
	docker-compose -f docker-compose.yml build nginx && \
    docker-compose -f docker-compose.yml up --no-recreate -d nginx

start_siren: start_fingerprint start_metadata start_nginx

stop_siren:
	docker-compose -f docker-compose.yml stop