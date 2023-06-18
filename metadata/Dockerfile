FROM hexpm/elixir:1.14.5-erlang-25.3.2.1-ubuntu-jammy-20230126

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      sudo \
      git \
      bash \
      gdb \
      wget \
      libcurl4-openssl-dev \
      curl \
      locales-all \
      dos2unix \
      rsync \
      tar \
      openssl \
      libpq-dev \
      libssl-dev && \
      apt-get clean

RUN mkdir /metadata
COPY . /home/user/siren
RUN rm -rf /home/user/siren/fingerprint
WORKDIR /home/user/siren/metadata

#RUN export MIX_ENV=prod
RUN mix local.hex --force
RUN mix do compile

RUN ( \
    echo 'LogLevel DEBUG2'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
    echo 'Port 53126'; \
  ) > /etc/ssh/sshd_config_siren \
  && mkdir /run/sshd

RUN useradd -m user \
  && yes password | passwd user

RUN usermod -s /bin/bash user
RUN chown -R user:user /home/user/
RUN chmod 755 /home/user/