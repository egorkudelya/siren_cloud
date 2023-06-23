defmodule Metadata.Library.Genre do
  use Ecto.Schema
  import Ecto.Changeset

  @derive {Jason.Encoder, only: [:name]}
  schema "genres" do
    field :name, :string

    many_to_many(:records, Metadata.Library.Record, join_through: Metadata.Library.RecordGenres)
    timestamps()
  end

  def changeset(genre, attrs) do
    genre
    |> cast(attrs, [:name])
    |> validate_required([:name])
    |> validate_length(:name, max: 255)
    |> unique_constraint(:name)
  end
end
