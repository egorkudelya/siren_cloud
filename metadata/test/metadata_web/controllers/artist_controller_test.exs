defmodule MetadataWeb.ArtistControllerTest do
  use MetadataWeb.ConnCase

  import Metadata.LibraryFixtures
  alias Metadata.Library.Artist

  @create_attrs %{
    name: "some name"
  }
  @update_attrs %{
    name: "some updated name"
  }
  @invalid_attrs %{name: nil}

  setup %{conn: conn} do
    {:ok, conn: put_req_header(conn, "accept", "application/json")}
  end

  describe "index" do
    test "lists all artists", %{conn: conn} do
      conn = get(conn, ~p"/api/artists")
      assert json_response(conn, 200)["data"] == []
    end
  end

  describe "create artist" do
    test "renders artist when data is valid", %{conn: conn} do
      conn = post(conn, ~p"/api/artists", artist: @create_attrs)
      assert %{"id" => id} = json_response(conn, 201)["data"]

      conn = get(conn, ~p"/api/artists/#{id}")

      assert %{
               "id" => ^id,
               "name" => "some name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn} do
      conn = post(conn, ~p"/api/artists", artist: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "update artist" do
    setup [:create_artist]

    test "renders artist when data is valid", %{conn: conn, artist: %Artist{id: id} = artist} do
      conn = put(conn, ~p"/api/artists/#{artist}", artist: @update_attrs)
      assert %{"id" => ^id} = json_response(conn, 200)["data"]

      conn = get(conn, ~p"/api/artists/#{id}")

      assert %{
               "id" => ^id,
               "name" => "some updated name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn, artist: artist} do
      conn = put(conn, ~p"/api/artists/#{artist}", artist: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "delete artist" do
    setup [:create_artist]

    test "deletes chosen artist", %{conn: conn, artist: artist} do
      conn = delete(conn, ~p"/api/artists/#{artist}")
      assert response(conn, 204)

      conn = get(conn, ~p"/api/artists/#{artist}")
      assert response(conn, 404)
    end
  end

  defp create_artist(_) do
    artist = artist_fixture()
    %{artist: artist}
  end
end
