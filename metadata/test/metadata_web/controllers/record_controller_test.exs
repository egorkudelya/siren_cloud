defmodule MetadataWeb.RecordControllerTest do
  use MetadataWeb.ConnCase
  use ExUnit.Case
  use ExVCR.Mock, adapter: ExVCR.Adapter.Hackney

  import Metadata.LibraryFixtures
  alias Metadata.Library.Record

  @audio_url "https://cdn.pixabay.com/download/audio/2023/03/23/audio_5dae37894b.mp3?filename=Daylight.mp3%5C"
  @create_attrs %{
    "id" => 1,
    "art_url" => "some art_url",
    "audio_url" => @audio_url,
    "bit_rate" => 42,
    "date_recordered" => ~D[2023-05-11],
    "duration" => 42,
    "name" => "some name",
    "single" => nil
  }

  @update_attrs %{
   "art_url" => "some updated art_url",
    "audio_url" => @audio_url,
    "bit_rate" => 43,
    "date_recordered" => ~D[2023-05-12],
    "duration" => 43,
    "name" => "some updated name",
    "single" => nil
  }

  @invalid_attrs %{
    "id" => nil,
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

  setup %{conn: conn} do
    HTTPoison.start()
    {:ok, conn: put_req_header(conn, "accept", "application/json")}
  end

  describe "index" do
    test "lists all records", %{conn: conn} do
      conn = get(conn, ~p"/api/records")
      assert json_response(conn, 200)["data"] == []
    end
  end

  describe "create record" do
    test "renders record when data is valid", %{conn: conn} do
      artist = artist_fixture()
      genre = genre_fixture()
      album = album_fixture()

      use_cassette "post_record_success" do
        conn = post(conn, ~p"/api/records", record: Map.merge(@create_attrs,
        %{"artists" => [artist.id],
        "albums" => [album.id],
        "genres" => [genre.id]
       }))
      assert %{"id" => id} = json_response(conn, 201)["data"]
      conn = get(conn, ~p"/api/records/#{id}")

      assert %{
               "id" => ^id,
               "art_url" => "some art_url",
               "audio_url" => @audio_url,
               "bit_rate" => 42,
               "date_recordered" => "2023-05-11",
               "duration" => 42,
               "name" => "some name"
             } = json_response(conn, 200)["data"]
      end
    end

    test "renders errors when data is invalid", %{conn: conn} do
      use_cassette "post_record_failure" do
        conn = post(conn, ~p"/api/records", record: @invalid_attrs)
        assert json_response(conn, 400)["errors"] != %{}
      end
    end
  end

  describe "update record" do
    setup [:create_record]

    test "renders record when data is valid", %{conn: conn, record: %Record{id: id} = record} do
      artist = artist_fixture()
      genre = genre_fixture()
      album = album_fixture()

      conn = put(conn, ~p"/api/records/#{record}", record: Map.merge(@update_attrs,
      %{"artists" => [artist.id],
        "albums" => [album.id],
        "genres" => [genre.id]
       }))

      assert %{"id" => ^id} = json_response(conn, 200)["data"]
      conn = get(conn, ~p"/api/records/#{id}")

      assert %{
               "id" => ^id,
               "art_url" => "some updated art_url",
               "audio_url" => @audio_url,
               "bit_rate" => 43,
               "date_recordered" => "2023-05-12",
               "duration" => 43,
               "name" => "some updated name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn, record: record} do
      conn = put(conn, ~p"/api/records/#{record}", record: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "delete record" do
    setup [:create_record]

    test "deletes chosen record", %{conn: conn, record: record} do
      use_cassette "delete_record_success" do
        conn = delete(conn, ~p"/api/records/#{record}")
        assert response(conn, 204)

        conn = get(conn, ~p"/api/records/#{record}")
        assert response(conn, 404)
      end
    end
  end

  defp create_record(_) do
    record = record_fixture()
    %{record: record}
  end
end
