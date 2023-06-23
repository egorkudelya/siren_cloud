defmodule Metadata.Repo.Migrations.CreateAlbums do
  use Ecto.Migration

  def change do
    create table(:albums) do
      add :name, :string, null: false
      add :art_url, :string, null: false
      add :is_single, :boolean, default: false
      add :artist_id, references(:artists, on_delete: :delete_all), null: false

      timestamps()
    end

    create index(:albums, [:artist_id])
  end
end
