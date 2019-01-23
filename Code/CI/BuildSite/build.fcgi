#!/usr/bin/python
from flup.server.fcgi import WSGIServer
from main import app
import os

os.environ["BUILDSITE_SETTINGS"] = "/var/www/settings.cfg"

class ScriptNameStripper(object):
   def __init__(self, app):
       self.app = app

   def __call__(self, environ, start_response):
       environ['SCRIPT_NAME'] = ''
       environ['BUILDSITE_SETTINGS'] = '/var/www/settings.cfg'
       return self.app(environ, start_response)

app = ScriptNameStripper(app)

if __name__ == '__main__':
    WSGIServer(app).run()