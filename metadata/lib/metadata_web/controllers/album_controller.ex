defmodule MetadataWeb.AlbumController do
  use MetadataWeb, :controller
  alias Metadata.{Library, Library.Album}

  action_fallback MetadataWeb.FallbackController

  def index(conn, _params) do
    albums = Library.list_albums()
    render(conn, :index, albums: albums)
  end

  def create(conn, %{"album" => %{"artist_id" => artist_id} = album_params}) when artist_id != "" do
    create_collection(conn, artist_id, album_params)
  end

  def create(_conn, _params), do: {:error, :not_found}

  def create_collection(conn, artist_id, params) do
    Library.get_artist(artist_id)
    |> case do
      nil ->
        {:error, :not_found}

      artist ->
        with {:ok, %Album{} = album} <- Library.create_album(artist, params) do
          conn
          |> put_status(:created)
          |> render(:show, album: album)
        end
    end
  end

  def show(conn, %{"id" => id}) do
    Library.get_album(id)
    |> case do
      nil ->
        {:error, :not_found}

      album ->
        render(conn, :show, album: album)
    end
  end

  def update(conn, %{"id" => id, "album" => album_params}) do
    Library.get_album(id)
    |> case do
      nil ->
        {:error, :not_found}

      album ->
        with {:ok, %Album{} = album} <- Library.update_album(album, album_params) do
          render(conn, :show, album: album)
        end
    end
  end

  def delete(conn, %{"id" => id}) do
    Library.get_album(id)
    |> case do
      nil ->
        {:error, :not_found}

      album ->
        with {:ok, %Album{}} <- Library.delete_album(album) do
          send_resp(conn, :no_content, "")
        end
    end
  end
end
