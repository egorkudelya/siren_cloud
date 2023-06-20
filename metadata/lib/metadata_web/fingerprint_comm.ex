defmodule FingerprintComm do

  @fingerprint_addr Enum.join(["http://", System.get_env("FINGERPRINT_ADDRESS"), ":", System.get_env("GRPC_PROXY_PORT")], "")
  @expected_response "{\"success\":true}"

  def fingerprint_addr, do: @fingerprint_addr
  def expected_response, do: @expected_response

  def post_record(id, audio_url, is_caching) do
    url = Enum.join([FingerprintComm.fingerprint_addr, "/v1/loadTrack"], "")
    body = Jason.encode!(%{
                            song_id: id,
                            url: audio_url,
                            is_caching: is_caching
                            })
    headers = [{"Content-type", "application/json"}]
    HTTPoison.post(url, body, headers)
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
    HTTPoison.post(url, body, headers)
    |> case do
      {:ok, res} ->
        Map.get(res, :body) == FingerprintComm.expected_response
      {:error, _} ->
        false
    end
  end
end
