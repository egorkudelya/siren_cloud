defmodule Metadata.Repo.Migrations.CreateRecordGenres do
  use Ecto.Migration

  def change do
    create table(:record_genres) do
      add :record_id, references(:records, on_delete: :delete_all)
      add :genre_id, references(:genres, on_delete: :delete_all)

      timestamps()
    end

    create unique_index(:record_genres, [:genre_id, :record_id])
  end
end
