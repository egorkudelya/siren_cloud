defmodule Metadata.Library.AlbumsRecords do
  use Ecto.Schema
  import Ecto.Changeset

  schema "aldum_records" do
    field :record_id, :integer
    field :album_id, :integer

    timestamps()
  end

  def changeset(albums_records, attrs) do
    albums_records
    |> cast(attrs, [])
    |> validate_required([])
  end
end
