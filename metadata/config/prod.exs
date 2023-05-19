import Config

config :metadata, Metadata.Repo,
  username: System.get_env("POSTGRES_USER"),
  password: System.get_env("POSTGRES_PASSWORD"),
  hostname: System.get_env("METADATA_HOST"),
  database: System.get_env("POSTGRES_DB_NAME"),
  stacktrace: true,
  show_sensitive_data_on_connection_error: true,
  pool_size: System.get_env("POSTGRES_DB_POOL_SIZE")

config :metadata, MetadataWeb.Endpoint,
  http: [ip: {0, 0, 0, 0}, port: System.get_env("METADATA_PORT")],
  check_origin: false,
  code_reloader: true,
  debug_errors: true,
  secret_key_base: System.get_env("SECRET_KEY_BASE"),
  watchers: []

config :metadata, dev_routes: true

config :logger, :console, format: "[$level] $message\n"

config :phoenix, :stacktrace_depth, 10

config :phoenix, :plug_init_mode, :runtime
