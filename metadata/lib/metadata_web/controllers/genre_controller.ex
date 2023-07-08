defmodule MetadataWeb.GenreController do
  use MetadataWeb, :controller
  alias Metadata.{Library, Library.Genre}

  action_fallback MetadataWeb.FallbackController

  def index(conn, _params) do
    genres = Library.list_genres()
    render(conn, :index, genres: genres)
  end

  def create(conn, %{"genre" => genre_params}) do
    with {:ok, %Genre{} = genre} <- Library.create_genre(genre_params) do
      conn
      |> put_status(:created)
      |> render(:show, genre: genre)
    end
  end

  def show(conn, %{"id" => id}) do
    Library.get_genre(id)
    |> case do
      nil ->
        {:error, :not_found}

      genre ->
        render(conn, :show, genre: genre)
    end
  end

  def update(conn, %{"id" => id, "genre" => genre_params}) do
    Library.get_genre(id)
    |> case do
      nil ->
        {:error, :not_found}

      genre ->
        with {:ok, %Genre{} = genre} <- Library.update_genre(genre, genre_params) do
          render(conn, :show, genre: genre)
        end
    end
  end

  def delete(conn, %{"id" => id}) do
    Library.get_genre(id)
    |> case do
      nil ->
        {:error, :not_found}

      genre ->
        with {:ok, %Genre{}} <- Library.delete_genre(genre) do
          send_resp(conn, :no_content, "")
        end
    end
  end
end
