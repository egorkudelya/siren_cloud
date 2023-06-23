defmodule Metadata.Library.RecordGenres do
  use Ecto.Schema
  import Ecto.Changeset

  schema "record_genres" do
    field :record_id, :integer
    field :genre_id, :integer

    timestamps()
  end

  def changeset(record_genres, attrs) do
    record_genres
    |> cast(attrs, [])
    |> validate_required([])
  end
end
