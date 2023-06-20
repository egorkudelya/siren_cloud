start_fingerprint:
	docker-compose -f docker-compose.yml build fingerprint_serv && \
	docker-compose -f docker-compose.yml up --no-recreate -d fingerprint_serv

prepare_fingerprint_storage:
	cd fingerprint/scripts && chmod +x postgres_init.sh && ./postgres_init.sh && \
	chmod +x elastic_init.sh && ./elastic_init.sh

clean_proxy:
	rm fingerprint/proxy/gen/proto/*.go

build_proxy:
	cd fingerprint/scripts && chmod +x build_proxy.sh && ./build_proxy.sh

start_proxy:
	cd fingerprint/scripts && chmod +x start_proxy.sh && ./start_proxy.sh

start_metadata:
	docker-compose -f docker-compose.yml build metadata_serv && \
    docker-compose -f docker-compose.yml up --no-recreate -d metadata_serv

stop_all:
	docker-compose -f docker-compose.yml stop