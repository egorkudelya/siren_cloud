defmodule MetadataWeb.AlbumControllerTest do
  use MetadataWeb.ConnCase

  import Metadata.LibraryFixtures
  alias Metadata.Library.Album

  @create_attrs %{
    art_url: "some art_url",
    name: "some name"
  }
  @update_attrs %{
    art_url: "some updated art_url",
    name: "some updated name"
  }
  @invalid_attrs %{art_url: nil, name: nil}

  setup %{conn: conn} do
    {:ok, conn: put_req_header(conn, "accept", "application/json")}
  end

  describe "index" do
    test "lists all albums", %{conn: conn} do
      conn = get(conn, ~p"/api/albums")
      assert json_response(conn, 200)["data"] == []
    end
  end

  describe "create album" do
    test "renders album when data is valid", %{conn: conn} do
      artist = artist_fixture()
      conn = post(conn, ~p"/api/albums", album: Map.merge(@create_attrs, %{"artist_id" => artist.id}))
      assert %{"id" => id} = json_response(conn, 201)["data"]

      conn = get(conn, ~p"/api/albums/#{id}")

      assert %{
               "id" => ^id,
               "art_url" => "some art_url",
               "name" => "some name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn} do
      conn = post(conn, ~p"/api/albums", album: @invalid_attrs)
      assert json_response(conn, 404)["errors"] != %{}
    end
  end

  describe "update album" do
    setup [:create_album]

    test "renders album when data is valid", %{conn: conn, album: %Album{id: id} = album} do
      artist = artist_fixture()
      conn = put(conn, ~p"/api/albums/#{album}", album: Map.merge(@update_attrs, %{"artist_id" => artist.id}))
      assert %{"id" => ^id} = json_response(conn, 200)["data"]

      conn = get(conn, ~p"/api/albums/#{id}")

      assert %{
               "id" => ^id,
               "art_url" => "some updated art_url",
               "name" => "some updated name",
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn, album: album} do
      conn = put(conn, ~p"/api/albums/#{album}", album: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "delete album" do
    setup [:create_album]

    test "deletes chosen album", %{conn: conn, album: album} do
      conn = delete(conn, ~p"/api/albums/#{album}")
      assert response(conn, 204)
      conn = get(conn, ~p"/api/albums/#{album}")
      assert response(conn, 404)
    end
  end

  defp create_album(_) do
    album = album_fixture()
    %{album: album}
  end

end
