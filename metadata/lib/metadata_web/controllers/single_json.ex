defmodule MetadataWeb.SingleJSON do
  alias MetadataWeb.AlbumJSON

  def index(params) do
    AlbumJSON.index(params)
  end

  def show(params) do
    AlbumJSON.show(params)
  end

  def data(params) do
    AlbumJSON.data(params)
  end
end
