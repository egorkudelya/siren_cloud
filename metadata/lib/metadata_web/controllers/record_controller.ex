defmodule MetadataWeb.RecordController do
  use MetadataWeb, :controller
  alias Metadata.{Library, Library.Record}
  alias FingerprintComm

  action_fallback MetadataWeb.FallbackController

  def index(conn, _params) do
    records = Library.list_records()
    render(conn, :index, records: records)
  end

  def create(conn, %{"record" => record_params}) do
    Map.get(record_params, "artists")
    |> case do
      res when res in [nil, []] ->
        {:error, :bad_request}

      _ ->
        with {:ok, %Record{} = record} <- Library.create_record(record_params) do
          FingerprintComm.post_record(Map.get(record, :id), Map.get(record, :audio_url), true)
          |> case do
            true ->
              conn
              |> put_status(:created)
              |> render(:show, record: record)

            _ ->
              Library.delete_record(record)
              {:error, :internal_server_error}
          end
        else
          _ ->
            {:error, :bad_request}
        end
    end
  end

  def show(conn, %{"id" => id}) do
    Library.get_record(id)
    |> case do
      nil ->
        {:error, :not_found}

      record ->
        render(conn, :show, record: record)
    end
  end

  def update(conn, %{"id" => id, "record" => record_params}) do
    Library.get_record(id)
    |> case do
      nil ->
        {:error, :not_found}

      record ->
        with {:ok, %Record{} = record} <- Library.update_record(record, record_params) do
          render(conn, :show, record: record)
        end
    end
  end

  def do_delete(conn, %{"id" => id}) do
    Library.get_record_including_hidden(id)
    |> case do
      nil ->
        {:error, :not_found}

    record ->
      with {:ok, %Record{}} <- Library.delete_record(record) do
        send_resp(conn, :no_content, "")
      end
    end
  end

  def delete(conn, %{"id" => id}) do
    Library.get_record(id)
    |> case do
      %Record{} = record ->
      FingerprintComm.delete_record(id)
      |> case do
        true ->
            Library.update_record(record, %{"is_visible" => false})
            send_resp(conn, :no_content, "")
        _ ->
            {:error, :internal_server_error}
        end
    nil ->
      {:error, :not_found}
    end
  end
end
