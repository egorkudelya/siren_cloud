defmodule Metadata.Application do
  # See https://hexdocs.pm/elixir/Application.html
  # for more information on OTP Applications
  @moduledoc false

  use Application

  @impl true
  def start(_type, _args) do
    children = [
      # Start the Telemetry supervisor
      MetadataWeb.Telemetry,
      # Start the Ecto repository
      Metadata.Repo,
      # Start the PubSub system
      {Phoenix.PubSub, name: Metadata.PubSub},
      # Start the Endpoint (http/https)
      MetadataWeb.Endpoint
      # Start a worker by calling: Metadata.Worker.start_link(arg)
      # {Metadata.Worker, arg}
    ]

    # See https://hexdocs.pm/elixir/Supervisor.html
    # for other strategies and supported options
    opts = [strategy: :one_for_one, name: Metadata.Supervisor]
    Supervisor.start_link(children, opts)
  end

  # Tell Phoenix to update the endpoint configuration
  # whenever the application is updated.
  @impl true
  def config_change(changed, _new, removed) do
    MetadataWeb.Endpoint.config_change(changed, removed)
    :ok
  end
end
