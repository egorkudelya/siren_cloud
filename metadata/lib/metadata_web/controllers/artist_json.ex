defmodule MetadataWeb.ArtistJSON do
  alias Metadata.Library.{Artist, Album, Record}

  def index(%{artists: artists}) do
    %{data: for(artist <- artists, do: data(artist))}
  end

  def show(%{artist: artist}) do
    %{data: data(artist)}
  end

  defp compose_records(%Album{} = album) do
    album.records
    |> case do
      [] -> []

      record_list ->
        Enum.map(record_list, fn record -> Map.take(record, Record.get_record_fields()) end)
    end
  end

  defp compose_collection(artist, is_single) do
    Enum.filter(artist.albums, fn album ->
      if album.is_single == is_single do
        album_map = Map.take(album, Album.get_album_fields())
        Map.merge(album_map, %{"records" => compose_records(album)})
    end
    end)
  end

  defp data(%Artist{} = artist) do
    %{
      id: artist.id,
      name: artist.name,
      albums: compose_collection(artist, false),
      singles: compose_collection(artist, true)
    }
  end
end
