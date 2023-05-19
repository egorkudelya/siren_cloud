defmodule MetadataWeb.GenreJSON do
  alias Metadata.Library.Genre

  def index(%{genres: genres}) do
    %{data: for(genre <- genres, do: data(genre))}
  end

  def show(%{genre: genre}) do
    %{data: data(genre)}
  end

  defp data(%Genre{} = genre) do
    %{
      id: genre.id,
      name: genre.name,
      records: genre.records
    }
  end
end
