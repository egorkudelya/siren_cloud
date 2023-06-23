defmodule Metadata.Library.Artist do
  use Ecto.Schema
  import Ecto.Changeset

  @derive {Jason.Encoder, only: [:name]}
  schema "artists" do
    field :name, :string

    has_many(:albums, Metadata.Library.Album, on_replace: :delete)
    many_to_many(:records, Metadata.Library.Record, join_through: Metadata.Library.RecordCredits)
    timestamps()
  end

  def changeset(artist, attrs) do
    artist
    |> cast(attrs, [:name])
    |> validate_required([:name])
    |> validate_length(:name, max: 255)
    |> unique_constraint(:name)
  end
end
