defmodule MetadataWeb.GenreControllerTest do
  use MetadataWeb.ConnCase

  import Metadata.LibraryFixtures
  alias Metadata.Library.Genre

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
    test "lists all genres", %{conn: conn} do
      conn = get(conn, ~p"/api/genres")
      assert json_response(conn, 200)["data"] == []
    end
  end

  describe "create genre" do
    test "renders genre when data is valid", %{conn: conn} do
      conn = post(conn, ~p"/api/genres", genre: @create_attrs)
      assert %{"id" => id} = json_response(conn, 201)["data"]

      conn = get(conn, ~p"/api/genres/#{id}")

      assert %{
               "id" => ^id,
               "name" => "some name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn} do
      conn = post(conn, ~p"/api/genres", genre: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "update genre" do
    setup [:create_genre]

    test "renders genre when data is valid", %{conn: conn, genre: %Genre{id: id} = genre} do
      conn = put(conn, ~p"/api/genres/#{genre}", genre: @update_attrs)
      assert %{"id" => ^id} = json_response(conn, 200)["data"]

      conn = get(conn, ~p"/api/genres/#{id}")

      assert %{
               "id" => ^id,
               "name" => "some updated name"
             } = json_response(conn, 200)["data"]
    end

    test "renders errors when data is invalid", %{conn: conn, genre: genre} do
      conn = put(conn, ~p"/api/genres/#{genre}", genre: @invalid_attrs)
      assert json_response(conn, 422)["errors"] != %{}
    end
  end

  describe "delete genre" do
    setup [:create_genre]

    test "deletes chosen genre", %{conn: conn, genre: genre} do
      conn = delete(conn, ~p"/api/genres/#{genre}")
      assert response(conn, 204)

      conn = get(conn, ~p"/api/genres/#{genre}")
      assert response(conn, 404)
    end
  end

  defp create_genre(_) do
    genre = genre_fixture()
    %{genre: genre}
  end
end
