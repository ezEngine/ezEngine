import sqlite3
import os
import globals
from flask import Flask, g

class DBAccess:
    ########################################################################
    ## DBAccess constructor
    ########################################################################
    def __init__(self, app):
        self.database = app.config['DATABASE'];


    ########################################################################
    ## DBAccess public functions
    ########################################################################
    def close(self, error):
        """Closes the database again at the end of the request."""
        if hasattr(g, 'sqlite_db'):
            g.sqlite_db.close()


    def connect(self):
        """Connects to the specific database."""
        rv = sqlite3.connect(self.database)
        rv.execute('pragma foreign_keys=ON')
        rv.row_factory = sqlite3.Row
        return rv


    def init(self, app):
        """Creates a new database with the tables and indices defined in 'schema.sql'."""
        with app.app_context():
            db = self.getDB()
            with app.open_resource('schema.sql', mode='r') as f:
                db.cursor().executescript(f.read())
            db.commit()


    def getDB(self):
        """Opens a new database connection if there is none yet for the current application context."""
        if not hasattr(g, 'sqlite_db'):
            g.sqlite_db = self.connect()
        return g.sqlite_db
    

    ########################################################################
    ## DBAccess private fields
    ########################################################################
    database = None