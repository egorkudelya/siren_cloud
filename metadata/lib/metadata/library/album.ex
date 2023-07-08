defmodule Metadata.Library.Album do
  use Ecto.Schema
  import Ecto.Changeset

  @serializable_fields [:art_url, :name, :is_single]

  @derive {Jason.Encoder, only: @serializable_fields}
  schema "albums" do
    field :art_url, :string
    field :name, :string
    field :is_single, :boolean

    belongs_to(:artist, Metadata.Library.Artist)
    many_to_many(:records, Metadata.Library.Record, join_through: Metadata.Library.AlbumsRecords)
    timestamps()
  end

  def get_album_fields() do
    @serializable_fields
  end

  def changeset(album, attrs) do
    album
    |> cast(attrs, @serializable_fields)
    |> validate_required([:name, :art_url])
    |> validate_length(:name, max: 255)
    |> assoc_constraint(:artist)
  end
end
