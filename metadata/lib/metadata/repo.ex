defmodule Metadata.Repo do
  use Ecto.Repo,
    otp_app: :metadata,
    adapter: Ecto.Adapters.Postgres
end
