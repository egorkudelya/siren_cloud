defmodule FingerprintComm do

  def fingerprint_addr, do: Enum.join(["http://", System.get_env("FINGERPRINT_ADDRESS"), ":", System.get_env("GRPC_PROXY_PORT")], "")
  def expected_response, do: "{\"success\":true}"

  def post_record(id, audio_url, is_caching) do
    url = Enum.join([FingerprintComm.fingerprint_addr, "/v1/loadTrack"], "")
    body = Jason.encode!(%{
                            song_id: id,
                            url: audio_url,
                            is_caching: is_caching
                            })
    headers = [{"Content-type", "application/json"}]
    HTTPoison.post(url, body, headers, recv_timeout: :timer.seconds(180))
    |> case do
      {:ok, res} ->
        Map.get(res, :body) == FingerprintComm.expected_response
      {:error, _} ->
        false
    end
  end

  def delete_record(id) do
    url = Enum.join([FingerprintComm.fingerprint_addr, "/v1/deleteTrack"], "")
    body = Jason.encode!(%{
                            song_id: id
                            })
    headers = [{"Content-type", "application/json"}]
    HTTPoison.post(url, body, headers, recv_timeout: :timer.seconds(180))
    |> case do
      {:ok, res} ->
        Map.get(res, :body) == FingerprintComm.expected_response
      {:error, _} ->
        false
    end
  end
end
