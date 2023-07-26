# Siren — a music recognition service written in C++ and Elixir

This is the main repository of the project. Other repositories include: 
1. Siren Core: https://github.com/egorkudelya/siren_core
2. Swift Package as a means of interfacing the Core written in C++ with Swift: https://github.com/egorkudelya/siren_package
3. IOS Client: https://github.com/egorkudelya/siren_client

**The project was written for educational purposes only.**

# Demo
There were 280 songs in the database during the test (approx. 18 hours of audio).

[![Video](https://drive.google.com/uc?id=1z6shqroRSs8Bk13mza-Qc0h56NCxDPo5)](https://drive.google.com/file/d/1WGRIsNg-bAMcJYhPKl3k77cMAG7003j-/view?usp=sharing)

# Getting started
The service expects a running Postgres instance ***on the host machine*** and Python v3.7+. To launch the containers, run *make start_siren.*

## Inside the fingerprint_serv container: ##
1. To initialize the storage, run *make prepare_fingerprint_storage*
2. To prepare and launch the reverse proxy, run *make start_proxy*
3. Build and run the server.

## Inside the metadata_serv container: ##
1. To initialize the storage, run *mix ecto.create*, then *mix ecto.migrate*
2. To run the server, execute *mix phx.server*

#### To stop the containers, run *make stop_siren* ####
