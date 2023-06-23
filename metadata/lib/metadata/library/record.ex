defmodule Metadata.Library.Record do
  use Ecto.Schema
  import Ecto.Changeset

  @serializable_fields [:name, :art_url, :audio_url, :bit_rate, :date_recorded, :duration]

  @derive {Jason.Encoder, only: @serializable_fields}
  schema "records" do
    field :art_url, :string
    field :audio_url, :string
    field :bit_rate, :integer
    field :date_recorded, :date
    field :duration, :integer
    field :name, :string
    field :is_visible, :boolean
    field :album_record_id, :integer
    field :credits_id, :integer
    field :record_genre_id, :integer

    many_to_many(:artists, Metadata.Library.Artist,
      join_through: Metadata.Library.RecordCredits,
      on_replace: :delete
    )

    many_to_many(:albums, Metadata.Library.Album,
      join_through: Metadata.Library.AlbumsRecords,
      on_replace: :delete
    )

    many_to_many(:genres, Metadata.Library.Genre,
      join_through: Metadata.Library.RecordGenres,
      on_replace: :delete
    )

    timestamps()
  end

  def get_record_fields() do
    @serializable_fields
  end

  def changeset(record, attrs, artists, albums, genres) do
    record
    |> cast(attrs, @serializable_fields ++ [:is_visible])
    |> validate_required(@serializable_fields)
    |> put_assoc(:artists, artists)
    |> put_assoc(:albums, albums)
    |> put_assoc(:genres, genres)
  end

  def changeset(record, attrs, artists: artists, genres: genres) do
    record
    |> cast(attrs, @serializable_fields ++ [:is_visible])
    |> validate_required(@serializable_fields)
    |> put_assoc(:artists, artists)
    |> put_assoc(:genres, genres)
  end

  def changeset(record, attrs, albums: albums, genres: genres) do
    record
    |> cast(attrs, @serializable_fields ++ [:is_visible])
    |> validate_required(@serializable_fields)
    |> put_assoc(:albums, albums)
    |> put_assoc(:genres, genres)
  end
end
