defmodule MetadataWeb.Router do
  use MetadataWeb, :router

  pipeline :api do
    plug :accepts, ["json"]
  end

  scope "/api", MetadataWeb do
    pipe_through :api

    resources "/artists",  ArtistController
    resources "/albums",   AlbumController
    resources "/genres",   GenreController
    resources "/records",  RecordController
    resources "/singles",  SingleController
    delete "/records/do_delete/:id", RecordController, :do_delete
  end
end
