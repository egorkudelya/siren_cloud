defmodule MetadataWeb.ArtistController do
  use MetadataWeb, :controller
  alias Metadata.{Library, Library.Artist}

  action_fallback MetadataWeb.FallbackController

  def index(conn, _params) do
    artists = Library.list_artists()
    render(conn, :index, artists: artists)
  end

  def create(conn, %{"artist" => artist_params}) do
    with {:ok, %Artist{} = artist} <- Library.create_artist(artist_params) do
      conn
      |> put_status(:created)
      |> render(:show, artist: artist)
    end
  end

  def show(conn, %{"id" => id}) do
    Library.get_artist(id)
    |> case do
      nil ->
        {:error, :not_found}

      artist ->
        render(conn, :show, artist: artist)
    end
  end

  def update(conn, %{"id" => id, "artist" => artist_params}) do
    Library.get_artist(id)
    |> case do
      nil ->
        {:error, :not_found}

      artist ->
        with {:ok, %Artist{} = artist} <- Library.update_artist(artist, artist_params) do
          render(conn, :show, artist: artist)
        end
    end
  end

  def delete(conn, %{"id" => id}) do
    Library.get_artist(id)
    |> case do
      nil ->
        {:error, :not_found}

      artist ->
        with {:ok, %Artist{}} <- Library.delete_artist(artist) do
          send_resp(conn, :no_content, "")
        end
    end
  end
end
