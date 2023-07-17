import os
import pandas as pd
import requests
import json
import os
import time


class Seeder:

    def __init__(self, dataframe):
        self.setup()
        self.df = dataframe
        self.sirenUrl = 'https://' + self.sirenHost + ':' + self.sirenPort + '/'
        self.headers = {"Content-Type": "application/json"}
        self.cache = {}

    def setup(self):
        self.admin = os.getenv('SIREN_ADMIN_NAME')
        self.password = os.getenv('SIREN_ADMIN_PASSWORD')
        self.sirenHost = os.getenv('SIREN_HOST')
        self.sirenPort = os.getenv('SIREN_PORT')

        if not self.admin:
            raise RuntimeError("SIREN_ADMIN_NAME is not set. Provide a valid admin name")
        elif not self.password:
            raise RuntimeError("SIREN_ADMIN_PASSWORD is not set. Provide a valid admin password")
        elif not self.sirenHost:
            raise RuntimeError("SIREN_HOST is not set. Provide a valid address to connect to Siren Cloud")
        elif not self.sirenPort:
            raise RuntimeError("SIREN_PORT is not set. Provide a valid port to connect to Siren Cloud")

    def initCache(self):
        def cacheIndex(indexName):
            self.cache[indexName] = {}
            index = requests.get(self.sirenUrl + indexName, auth=(self.admin, self.password), verify=False,
                                 headers=self.headers)
            if index.status_code != 200:
                return
            index = index.json()
            for body in index['data']:
                self.cache[indexName][body['name']] = body['id']

        cacheIndex('artists')
        cacheIndex('genres')
        cacheIndex('albums')
        cacheIndex('records')

    def populate(self):
        self.initCache()
        self.postArtists()
        self.postGenres()
        self.postAlbums()
        self.postTracks()

    def postArtists(self):
        url = self.sirenUrl + "artists"
        uniqueArtists = self.df['artist_name'].unique()
        for artistName in uniqueArtists:
            if artistName in self.cache['artists']:
                continue
            j = {'artist': {'name': artistName}}
            res = requests.post(url, json=j, auth=(self.admin, self.password), verify=False, headers=self.headers)
            assert res.status_code == 201
            if artistName not in self.cache['artists']:
                self.cache['artists'][artistName] = json.loads(res.text)['data']['id']

    def postGenres(self):
        url = self.sirenUrl + "genres"
        uniqueGenres = set(item for sublist in self.df['track_genres'] for item in sublist.strip('][').split(', '))
        for genreName in uniqueGenres:
            if genreName in self.cache['genres']:
                continue
            j = {'genre': {'name': genreName}}
            res = requests.post(url, json=j, auth=(self.admin, self.password), verify=False, headers=self.headers)
            assert res.status_code == 201
            if genreName not in self.cache['genres']:
                self.cache['genres'][genreName] = json.loads(res.text)['data']['id']

    def postAlbums(self):
        url = self.sirenUrl + "albums"
        albumData = self.df[['album_title', 'artist_name', 'art_url']]
        uniqueAlbums = albumData['album_title'].unique()
        for albumName in uniqueAlbums:
            if albumName in self.cache['albums']:
                continue
            albumEntry = albumData.loc[albumData['album_title'] == albumName].head(1)
            artistId = self.cache['artists'][albumEntry['artist_name'].item()]
            j = {'album': {'name': albumName, 'art_url': albumEntry['art_url'].item(), 'artist_id': artistId}}
            res = requests.post(url, json=j, auth=(self.admin, self.password), verify=False, headers=self.headers)
            assert res.status_code == 201
            if albumName not in self.cache['albums']:
                self.cache['albums'][albumName] = json.loads(res.text)['data']['id']

    def postTracks(self):
        url = self.sirenUrl + "records"
        for i, row in self.df.iterrows():
            if row['track_title'] in self.cache['records']:
                continue
            artistIds = [self.cache['artists'][row['artist_name']]]
            albumIds = [self.cache['albums'][row['album_title']]]
            genreIds = [self.cache['genres'][genre] for genre in row['track_genres'].strip('][').split(', ')]
            j = {"record": {
                "art_url": row['art_url'],
                "audio_url": row['audio_url'],
                "bit_rate": row['track_bit_rate'],
                "date_recorded": row['track_date_recorded'],
                "duration": row['track_duration'],
                "name":  row['track_title'],
                "artists": artistIds,
                "albums": albumIds,
                "genres": genreIds,
                "single": None
            }}
            res = requests.post(url, json=j, auth=(self.admin, self.password), verify=False, headers=self.headers)
            assert res.status_code == 201
            time.sleep(1)


def main():
    frame = pd.read_csv("../data/sample_dataset.csv")
    client = Seeder(frame)
    client.populate()


if __name__ == "__main__":
    main()
