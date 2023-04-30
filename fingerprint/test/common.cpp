#include "common.h"

std::string initPostgresConnStr()
{
    std::stringstream postgres;
    postgres << "postgresql://" << siren::getenv("POSTGRES_USER") << ':'
             << siren::getenv("POSTGRES_PASSWORD") << '@' << siren::getenv("POSTGRES_HOST") << ':'
             << siren::getenv("POSTGRES_PORT") << '/' << siren::getenv("POSTGRES_DB_NAME");
    return postgres.str();
}

std::string initElasticConnStr()
{
    std::stringstream elastic;
    elastic << "https://" << siren::getenv("ELASTIC_HOST") << ":" << siren::getenv("ES_PORT") << "/";
    return elastic.str();
}