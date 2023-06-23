defmodule MetadataWeb.RecordJSON do
  alias Metadata.Library.{Record, Album}

  def index(%{records: records}) do
    %{data: for(record <- records, do: data(record))}
  end

  def show(%{record: record}) do
    %{data: data(record)}
  end

  defp compose_collection(collection, is_single) do
    Enum.filter(collection, fn album ->
      if album.is_single == is_single do
        Map.take(album, Album.get_album_fields())
    end
    end)
  end

  defp data(%Record{} = record) do
    %{
      id: record.id,
      name: record.name,
      duration: record.duration,
      date_recorded: record.date_recorded,
      bit_rate: record.bit_rate,
      audio_url: record.audio_url,
      art_url: record.art_url,
      artists: record.artists,
      genres: record.genres,
      albums: compose_collection(record.albums, false),
      single: compose_collection(record.albums, true)
    }
  end
end
