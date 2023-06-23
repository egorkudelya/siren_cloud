defmodule Metadata.Repo.Migrations.CreateRecords do
  use Ecto.Migration

  def change do
    create table(:records) do
      add :name, :string
      add :duration, :integer
      add :date_recorded, :date
      add :bit_rate, :integer
      add :audio_url, :string
      add :art_url, :string
      add :is_visible, :boolean, default: true
      add :album_record_id, references(:albums, on_delete: :delete_all)
      add :credits_id, references(:records, on_delete: :delete_all)
      add :record_genre_id, references(:genres, on_delete: :delete_all)

      timestamps()
    end

    create index(:records, [:credits_id, :album_record_id, :record_genre_id])
  end
end
