defmodule Metadata.Repo.Migrations.CreateAldumRecords do
  use Ecto.Migration

  def change do
    create table(:aldum_records) do
      add :record_id, references(:records, on_delete: :delete_all)
      add :album_id, references(:albums, on_delete: :delete_all)

      timestamps()
    end

    create unique_index(:aldum_records, [:album_id, :record_id])
  end
end
