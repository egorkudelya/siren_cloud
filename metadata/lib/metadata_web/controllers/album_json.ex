defmodule MetadataWeb.AlbumJSON do
  alias Metadata.Library.Album

  def index(%{albums: albums}) do
    %{data: for(album <- albums, do: data(album))}
  end

  def show(%{album: album}) do
    %{data: data(album)}
  end

  def data(%Album{} = album) do
    %{
      id: album.id,
      name: album.name,
      art_url: album.art_url,
      records: album.records
    }
  end
end
