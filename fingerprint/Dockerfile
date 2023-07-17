FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV GRPC_RELEASE_TAG v1.52.0
ENV JSON_RELEASE_TAG v3.11.2
ENV LIBPQXX_RELEASE_TAG 7.6.1
ENV GO_VERSION 1.19
ENV GO_DOWNLOAD_URL https://golang.org/dl/go${GO_VERSION}.src.tar.gz
ENV WASSERSTEIN_INCLUDE_DIR /usr/local/include/wasserstein

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      gcc \
      clang \
      git \
      g++ \
      bash \
      gdb \
      wget \
      cmake \
      ca-certificates \
      libcurl4-openssl-dev \
      curl \
      clang \
      make \
      ninja-build \
      autoconf \
      automake \
      locales-all \
      dos2unix \
      rsync \
      tar \
      openssl \
      libssl-dev \
      unzip \
      zlib1g-dev \
      libtool \
      autogen \
      pkg-config \
      pip \
      libfmt-dev \
      libpq-dev \
      libboost-dev \
      postgresql-server-dev-all \
      libgtest-dev \
      golang-go \
      libspdlog-dev \
      valgrind && \
      apt-get clean

WORKDIR /usr/local/include

# multi-index
RUN git clone https://github.com/boostorg/multi_index.git && cd multi_index && \
    git checkout master

# libpqxx
RUN git clone https://github.com/jtv/libpqxx.git && cd libpqxx && \
    git checkout ${LIBPQXX_RELEASE_TAG} && \
    mkdir build && cd build && \
    cmake .. -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j6 && make install

# siren_core
RUN git clone --recurse-submodules -b prod https://github.com/egorkudelya/siren_core.git && cd siren_core && \
    mkdir build && cd build &&  \
    cmake -DCMAKE_BUILD_TYPE=Release ..

# nlohmann/json
RUN git clone https://github.com/nlohmann/json.git && \
    cd json && \
    git checkout ${JSON_RELEASE_TAG} && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=OFF .. && \
    make && \
    make install

# cpr
RUN git clone https://github.com/libcpr/cpr.git && \
 cd cpr && mkdir build && cd build && \
 cmake .. -DCMAKE_BUILD_TYPE=Release -DCPR_USE_SYSTEM_CURL=ON && \
 cmake --build . && \
 cmake --install .

# wasserstein
RUN git clone https://github.com/gnardari/wasserstein.git ${WASSERSTEIN_INCLUDE_DIR}

# gRPC
RUN cd /tmp && \
  echo cloning GRPC version ${GRPC_RELEASE_TAG} && \
  cd /tmp && git clone --recurse-submodules -b ${GRPC_RELEASE_TAG} https://github.com/grpc/grpc && \
  echo GRPC and modules from grpc source && \
  cd /tmp/grpc && mkdir -p cmake/build && cd cmake/build && \
  cmake ../.. \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DgRPC_INSTALL=ON \
  -DgRPC_BUILD_TESTS=OFF \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_ZLIB_PROVIDER=package && \
  make -j6 install && \
  cd / && rm -rf /tmp/grpc

# go
ENV PATH $PATH:/usr/local/go/bin
RUN curl -fsSL "${GO_DOWNLOAD_URL}" -o golang.tar.gz \
	&& tar -C /usr/local -xzf golang.tar.gz \
	&& rm golang.tar.gz \
	&& cd /usr/local/go/src && ./make.bash 2>&1 \
	&& apt-get purge -y golang-go && apt-get autoremove -y

# perf
RUN export KERNEL_RELEASE=$(uname -r) && apt-get -y install linux-tools-${KERNEL_RELEASE};

COPY . /home/user/siren
RUN rm -rf /home/user/siren/fingerprint/cmake-build-debug && \
    rm -rf /home/user/siren/fingerprint/cmake-build-release && \
    rm -rf /home/user/siren/metadata

RUN ( \
    echo 'LogLevel DEBUG2'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
    echo 'Port 53125'; \
  ) > /etc/ssh/sshd_config_siren \
  && mkdir /run/sshd

RUN useradd -m user \
  && yes password | passwd user

RUN usermod -s /bin/bash user
RUN chown -R user:user /home/user/
RUN chmod 755 /home/user/
