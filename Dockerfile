# Copyright (c) 2018 The BitSend BSD Core Developers (dalijolijo)
FROM debian:jessie-slim

WORKDIR /usr/src/bitsend-seeder

RUN apt-get update && \
    apt-get install -y build-essential \
                       libboost-all-dev \
                       libssl-dev
                       
RUN mkdir -p /usr/src/bitsend-seeder

ADD . /usr/src/bitsend-seeder

RUN make

ENTRYPOINT ["/usr/src/bitsend-seeder/dnsseed"]
