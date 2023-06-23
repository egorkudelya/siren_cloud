defmodule MetadataWeb.SingleController do
  use MetadataWeb, :controller
  alias MetadataWeb.AlbumController
  alias Metadata.{Library, Library.Album}

  action_fallback MetadataWeb.FallbackController

  def index(conn, _params) do
    singles = Library.list_singles()
    render(conn, :index, albums: singles)
  end

  def show(conn, %{"id" => id}) do
    Library.get_single(id)
    |> case do
      nil ->
        {:error, :not_found}

      single ->
        render(conn, :show, album: single)
    end
  end

  def create(conn, %{"single" => %{"artist_id" => artist_id} = single_params}) when artist_id != "" do
    AlbumController.create_collection(conn, artist_id, put_in(single_params["is_single"], true))
  end

  def update(conn, %{"id" => id, "single" => single_params}) do
    Library.get_single(id)
    |> case do
      nil ->
        {:error, :not_found}

      single ->
        with {:ok, %Album{} = single} <- Library.update_album(single, put_in(single_params["is_single"], true)) do
          render(conn, :show, album: single)
        end
    end
  end

  def delete(conn, %{"id" => id}) do
    Library.get_single(id)
    |> case do
      nil ->
        {:error, :not_found}

      single ->
        with {:ok, %Album{}} <- Library.delete_album(single) do
          send_resp(conn, :no_content, "")
        end
    end
  end

end
