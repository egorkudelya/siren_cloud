defmodule Metadata.LibraryFixtures do

  def artist_fixture(attrs \\ %{}) do
    rand = for _ <- 1..10, into: "", do: << Enum.random('abcdef')>>
    {:ok, artist} =
      attrs
      |> Enum.into(%{
        name: rand
      })
      |> Metadata.Library.create_artist()

    artist
  end

  def album_fixture(attrs \\ %{}) do
    artist = artist_fixture()
    album = attrs
      |> Enum.into(%{
        name: "some name",
        art_url: "some art_url",
        is_single: false,
        artist_id: artist.id
      })
      {:ok, album} = Metadata.Library.create_album(artist, album)
      album
  end

  def genre_fixture(attrs \\ %{}) do
    rand = for _ <- 1..10, into: "", do: << Enum.random('abcdef')>>
    {:ok, genre} =
      attrs
      |> Enum.into(%{
        name: rand
      })
      |> Metadata.Library.create_genre()

    genre
  end

  def record_fixture(attrs \\ %{}) do
    artist = artist_fixture()
    genre = genre_fixture()
    album = album_fixture()

    {:ok, record} =
      attrs
      |> Enum.into(%{
        "id" => 1,
        "art_url" => "some art_url",
        "audio_url" => "some audio_url",
        "bit_rate" => 42,
        "date_recordered" => ~D[2023-05-11],
        "duration" => 42,
        "name" => "some name",
        "artists" => [artist.id],
        "albums" => [album.id],
        "genres" => [genre.id],
        "single" => nil
      })
      |> Metadata.Library.create_record()
    record
  end

end
