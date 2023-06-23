defmodule Metadata.Repo.Migrations.CreateRecordCredits do
  use Ecto.Migration

  def change do
    create table(:record_credits) do
      add :record_id, references(:records, on_delete: :delete_all)
      add :artist_id, references(:artists, on_delete: :delete_all)

      timestamps()
    end

    create unique_index(:record_credits, [:artist_id, :record_id])
  end
end
