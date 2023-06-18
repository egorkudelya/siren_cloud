defmodule Metadata.LibraryTest do
  use Metadata.DataCase
  alias Metadata.Library

  describe "artists" do
    alias Metadata.Library.Artist
    import Metadata.LibraryFixtures

    @invalid_attrs %{name: nil}

    test "list_artists/0 returns all artists" do
      artist = artist_fixture()
      assert Library.list_artists() == [artist]
    end

    test "get_artist/1 returns the artist with given id" do
      artist = artist_fixture()
      assert Library.get_artist(artist.id) == artist
    end

    test "create_artist/1 with valid data creates a artist" do
      valid_attrs = %{name: "some name"}

      assert {:ok, %Artist{} = artist} = Library.create_artist(valid_attrs)
      assert artist.name == "some name"
    end

    test "create_artist/1 with invalid data returns error changeset" do
      assert {:error, %Ecto.Changeset{}} = Library.create_artist(@invalid_attrs)
    end

    test "update_artist/2 with valid data updates the artist" do
      artist = artist_fixture()
      update_attrs = %{name: "some updated name"}

      assert {:ok, %Artist{} = artist} = Library.update_artist(artist, update_attrs)
      assert artist.name == "some updated name"
    end

    test "update_artist/2 with invalid data returns error changeset" do
      artist = artist_fixture()
      assert {:error, %Ecto.Changeset{}} = Library.update_artist(artist, @invalid_attrs)
      assert artist == Library.get_artist(artist.id)
    end

    test "delete_artist/1 deletes the artist" do
      artist = artist_fixture()
      assert {:ok, %Artist{}} = Library.delete_artist(artist)
      assert is_nil(Library.get_artist(artist.id))
    end

  end

  describe "albums" do
    alias Metadata.Library.Album
    import Metadata.LibraryFixtures

    @invalid_attrs %{art_url: nil, name: nil}

    test "list_albums/0 returns all albums" do
      album = album_fixture()
      assert List.first(Library.list_albums()) == album
    end

    test "get_album/1 returns the album with given id" do
      album = album_fixture()
      assert Library.get_album(album.id) == album
    end

    test "create_album/1 with valid data creates an album" do
      valid_attrs = %{art_url: "some art_url", name: "some name"}

      artist = artist_fixture()
      assert {:ok, %Album{} = album} = Library.create_album(artist, valid_attrs)
      assert album.art_url == "some art_url"
      assert album.name == "some name"
    end

    test "create_album/1 with invalid data returns error changeset" do
      artist = artist_fixture()
      assert {:error, %Ecto.Changeset{}} = Library.create_album(artist, @invalid_attrs)
    end

    test "update_album/2 with valid data updates the album" do
      album = album_fixture()
      update_attrs = %{art_url: "some updated art_url", name: "some updated name"}

      assert {:ok, %Album{} = album} = Library.update_album(album, update_attrs)
      assert album.art_url == "some updated art_url"
      assert album.name == "some updated name"
    end

    test "update_album/2 with invalid data returns error changeset" do
      album = album_fixture()
      assert {:error, %Ecto.Changeset{}} = Library.update_album(album, @invalid_attrs)
      assert album == Library.get_album(album.id)
    end

    test "delete_album/1 deletes the album" do
      album = album_fixture()
      assert {:ok, %Album{}} = Library.delete_album(album)
      assert is_nil(Library.get_album(album.id))
    end

  end

  describe "genres" do
    alias Metadata.Library.Genre
    import Metadata.LibraryFixtures

    @invalid_attrs %{name: nil}

    test "list_genres/0 returns all genres" do
      genre = genre_fixture()
      assert Library.list_genres() == [genre]
    end

    test "get_genre/1 returns the genre with given id" do
      genre = genre_fixture()
      assert Library.get_genre(genre.id) == genre
    end

    test "create_genre/1 with valid data creates a genre" do
      valid_attrs = %{name: "some name"}

      assert {:ok, %Genre{} = genre} = Library.create_genre(valid_attrs)
      assert genre.name == "some name"
    end

    test "create_genre/1 with invalid data returns error changeset" do
      assert {:error, %Ecto.Changeset{}} = Library.create_genre(@invalid_attrs)
    end

    test "update_genre/2 with valid data updates the genre" do
      genre = genre_fixture()
      update_attrs = %{name: "some updated name"}

      assert {:ok, %Genre{} = genre} = Library.update_genre(genre, update_attrs)
      assert genre.name == "some updated name"
    end

    test "update_genre/2 with invalid data returns error changeset" do
      genre = genre_fixture()
      assert {:error, %Ecto.Changeset{}} = Library.update_genre(genre, @invalid_attrs)
      assert genre == Library.get_genre(genre.id)
    end

    test "delete_genre/1 deletes the genre" do
      genre = genre_fixture()
      assert {:ok, %Genre{}} = Library.delete_genre(genre)
      assert is_nil(Library.get_genre(genre.id))
    end

  end

  describe "records" do
    alias Metadata.Library.Record
    import Metadata.LibraryFixtures

    @invalid_attrs %{
        "art_url" => nil,
        "audio_url" => nil,
        "bit_rate" => nil,
        "date_recordered" => nil,
        "duration" => nil,
        "name" => nil,
        "artists" => [nil],
        "albums" => [nil],
        "genres" => [nil],
        "single" => nil
      }

    test "list_records/0 returns all records" do
      record_fixture()
      listed_record = List.first(Library.list_records())
      assert %Record{} = listed_record
    end

    test "get_record/1 returns the record with given id" do
      record = record_fixture()
      assert %Record{} = Library.get_record(record.id)
    end

    test "create_record/1 with valid data creates a record" do
      artist = artist_fixture()
      genre = genre_fixture()
      album = album_fixture()

      valid_attrs = %{
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
      }

      assert {:ok, %Record{} = record} = Library.create_record(valid_attrs)
      assert record.art_url == "some art_url"
      assert record.audio_url == "some audio_url"
      assert record.bit_rate == 42
      assert record.date_recordered == ~D[2023-05-11]
      assert record.duration == 42
      assert record.name == "some name"
    end

    test "create_record/1 with invalid data returns an error" do
      assert {:error, :bad_request} = Library.create_record(@invalid_attrs)
    end

    test "update_record/2 with valid data updates the record" do
      record = record_fixture()
      update_attrs = %{
        "art_url" => "some updated art_url",
        "audio_url" => "some updated audio_url",
        "bit_rate" => 43,
        "date_recordered" => ~D[2023-05-12],
        "duration" => 43,
        "name" => "some updated name",
      }
      assert {:ok, %Record{} = record} = Library.update_record(record, update_attrs)
      assert record.art_url == "some updated art_url"
      assert record.audio_url == "some updated audio_url"
      assert record.bit_rate == 43
      assert record.date_recordered == ~D[2023-05-12]
      assert record.duration == 43
      assert record.name == "some updated name"
    end

    test "update_record/2 with invalid data returns error changeset" do
      record = record_fixture()
      assert {:error, %Ecto.Changeset{}} = Library.update_record(record, @invalid_attrs)
    end

    test "delete_record/1 deletes the record" do
      record = record_fixture()
      assert {:ok, %Record{}} = Library.delete_record(record)
      assert is_nil(Library.get_record(record.id))
    end
  end
end
