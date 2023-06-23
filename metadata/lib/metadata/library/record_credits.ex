defmodule Metadata.Library.RecordCredits do
  use Ecto.Schema
  import Ecto.Changeset

  schema "record_credits" do
    field :record_id, :integer
    field :artist_id, :integer

    timestamps()
  end

  def changeset(record_credits, attrs) do
    record_credits
    |> cast(attrs, [])
    |> validate_required([])
  end
end
