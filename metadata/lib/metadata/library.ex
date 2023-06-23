defmodule Metadata.Library do
  import Ecto.Query, warn: false
  alias Metadata.{Library, Library.Artist, Repo}

  def list_artists do
    Repo.all(Artist)
    |> Repo.preload([:records, albums: [:records]])
  end

  def get_artist(id) when not is_nil(id) do
    Artist
    |> Repo.get(id)
    |> Repo.preload([:records, albums: [:records]])
  end

  def get_artist(_id), do: nil

  def create_artist(attrs \\ %{}) do
    %Artist{}
    |> Artist.changeset(attrs)
    |> Repo.insert()
    |> case do
      {:ok, %Artist{} = artist} -> {:ok, Repo.preload(artist, [:records, albums: [:records]])}
      error -> error
    end
  end

  def update_artist(%Artist{} = artist, attrs) do
    artist
    |> Artist.changeset(attrs)
    |> Repo.update()
  end

  def delete_artist(%Artist{} = artist) do
    Repo.delete(artist)
  end

  alias Metadata.Library.Album

  def list_singles do
    Album
    |> where([c], c.is_single)
    |> Repo.all()
    |> Repo.preload(:records)
  end

  def get_single(id) when not is_nil(id) do
    Album
    |> where([c], c.is_single)
    |> Repo.get(id)
    |> Repo.preload(:records)
  end

  def get_single(_id), do: nil

  def list_albums do
    Album
    |> where([c], c.is_single == false)
    |> Repo.all()
    |> Repo.preload(:records)
  end

  def get_album(id) when not is_nil(id) do
    Album
    |> where([c], c.is_single == false)
    |> Repo.get(id)
    |> Repo.preload(:records)
  end

  def get_album(_id), do: nil

  def create_album(%Artist{} = artist, attrs \\ %{}) do
    artist
    |> Ecto.build_assoc(:albums)
    |> Album.changeset(attrs)
    |> Repo.insert()
    |> case do
      {:ok, %Album{} = album} -> {:ok, Repo.preload(album, :records)}
      error -> error
    end
  end

  def update_album(%Album{} = album, attrs) do
    album
    |> Album.changeset(attrs)
    |> Repo.update()
  end

  def delete_album(%Album{} = album) do
    Repo.delete(album)
  end

  alias Metadata.Library.Genre

  def list_genres do
    Repo.all(Genre)
    |> Repo.preload(:records)
  end

  def get_genre(id) when not is_nil(id) do
    Genre
    |> Repo.get(id)
    |> Repo.preload(:records)
  end

  def get_genre(_id), do: nil

  def create_genre(attrs \\ %{}) do
    %Genre{}
    |> Genre.changeset(attrs)
    |> Repo.insert()
    |> case do
      {:ok, %Genre{} = genre} -> {:ok, Repo.preload(genre, :records)}
      error -> error
    end
  end

  def update_genre(%Genre{} = genre, attrs) do
    genre
    |> Genre.changeset(attrs)
    |> Repo.update()
  end

  def delete_genre(%Genre{} = genre) do
    Repo.delete(genre)
  end

  alias Metadata.Library.Record

  def list_records() do
    Record
    |> where([r], r.is_visible)
    |> Repo.all()
    |> Repo.preload([:artists, :albums, :genres])
  end

  def get_record(id) do
    Record
    |> where([r], r.is_visible)
    |> Repo.get(id)
    |> Repo.preload([:artists, :albums, :genres])
  end

  def get_record_including_hidden(id) when not is_nil(id) do
    Record
    |> Repo.get(id)
    |> Repo.preload([:artists, :albums, :genres])
  end

  def get_record_including_hidden(_id), do: nil

  defp bool_to_int(bool) do
    if bool, do: 1, else: 0
  end

  def create_record(attrs \\ %{})

  def create_record(%{"artists" => artist_ids, "albums" => album_ids, "single" => single_id, "genres" => genre_ids} = attrs)
    when album_ids != [] or not is_nil(single_id) do

    single = Library.get_single(single_id)
    albums = Enum.map(album_ids, fn(id) -> Library.get_album(id) end)
    artists = Enum.map(artist_ids, fn(id) -> Library.get_artist(id) end)
    genres = Enum.map(genre_ids, fn(id) -> Library.get_genre(id) end)
    collection = Enum.filter(albums ++ [single], & !is_nil(&1))

    case length(collection) == length(albums) + bool_to_int(!is_nil(single)) do
      true ->
        %Record{}
        |> Record.changeset(attrs, artists, collection, genres)
        |> Repo.insert()
        |> case do
          {:ok, %Record{} = record} -> {:ok, Repo.preload(record, [:artists, :albums, :genres])}
          error -> error
        end
      _ ->
        {:error, :bad_request}
    end
  end

  defp process_range_id([id | _] = ids) when is_integer(id) do
    ids
  end

  defp process_range_id(structs) do
    filtered = Enum.filter(structs, & !is_nil(&1))
    Enum.map(filtered, fn(struct) ->
      Map.get(struct, :id)
    end)
  end

  defp transfer_single(record) do
    single = record
    |> Map.get("albums")
    |> Enum.filter(& &1.is_single)
    |> List.first()
    Map.merge(record, %{"single" => single})
  end

  defp validate_albums(album_ids, is_legacy) do
    filtered = Enum.map(album_ids, fn(id) -> Library.get_album(id) != nil or is_legacy && Library.get_single(id) != nil end)
    length(album_ids) == Enum.count(filtered, fn(r) -> r end)
  end

  def update_record(%Record{} = record, attrs) do
    record_map = record
    |> Map.from_struct()
    |> Enum.map(fn {k, v} -> {Atom.to_string(k), v} end)
    |> Enum.into(%{})
    |> transfer_single()
    |> Map.merge(attrs)

    artists = Enum.map(process_range_id(Map.get(record_map, "artists")), fn(id) -> Library.get_artist(id) end)
    genres = Enum.map(process_range_id(Map.get(record_map, "genres")), fn(id) -> Library.get_genre(id) end)
    album_ids = process_range_id(Map.get(record_map, "albums"))
    single_field = Map.get(record_map, "single")
    single = Library.get_single(List.first(process_range_id([single_field])))

    is_single_valid = if not is_nil(single_field), do: not is_nil(single), else: true
    case validate_albums(album_ids, album_ids != Map.get(attrs, "albums")) and is_single_valid do
      true ->
        albums = Enum.filter(Enum.map(album_ids, fn(id) -> Library.get_album(id) end), & !is_nil(&1))
        collections = if not is_nil(single_field), do: albums ++ [single], else: albums
        record
        |> Record.changeset(record_map, artists, collections, genres)
        |> Repo.update()
      _ ->
        {:error, :bad_request}
    end
  end

  def delete_record(%Record{} = record) do
    Repo.delete(record)
  end

end
