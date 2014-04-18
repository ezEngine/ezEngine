########################################################################
## Imports
########################################################################
from __future__ import division
import sqlite3
import os
import datetime
import math
import re
from DBAccess import DBAccess
from DBWriter import DBWriter
from DBReader import DBReader
from flask import Flask, request, session, g, redirect, url_for, abort, render_template, flash
from flask.ext.mail import Mail, Message
from jinja2 import evalcontextfilter, Markup, escape
from werkzeug.contrib.profiler import ProfilerMiddleware
import pysvn
import threading
import sys

########################################################################
## App config
######################################################################## 
app = Flask(__name__)
app.config.from_object(__name__)

# Load default config and override config from an environment variable
app.config.update(dict(
    DATABASE=os.path.join(app.root_path, 'buildSite.db'),
    SECRET_KEY='',                          # No idea what this is used for, was in the tutorial.
    USERNAME='',                            # No idea what this is used for, was in the tutorial.
    PASSWORD='',                            # No idea what this is used for, was in the tutorial.
    SVN_USER='',                            # SVN user used to query the HEAD revision.
    SVN_PASS='',                            # Password for SVN_USER
    SVN_ROOT='',                            # Root address of the SVN repository.
    SVN_USER_TO_MAIL_REQUEST_ADDRESS='',    # Server address used to resolve SVN usernames to email addresses.
    WAKE_UP_COMMAND='',                     # Command line that is executed whenever a new revision comes in that need to be built.
    MAIL_SERVER = '',                       # SMTP server for sending mails.
    MAIL_PORT = 465,                        # Port for MAIL_SERVER.
    MAIL_USE_TLS = False,                   # Use TLS for MAIL_SERVER.
    MAIL_USE_SSL = True,                    # Use SSL for MAIL_SERVER.
    MAIL_USERNAME = '',                     # Username, i.e. mail address of the build nanny used for MAIL_SERVER.
    MAIL_PASSWORD = '',                     # Password for sending over MAIL_SERVER.
    SITE_ROOT = '',                         # Used to make the img src address absolute in the html mail.
    DEBUG = False                           # Determines whether we are deployed on the site or debugging locally.
))
app.config.from_envvar('BUILDSITE_SETTINGS', silent=False)
app.config['PROFILE'] = False
#app.wsgi_app = ProfilerMiddleware(app.wsgi_app, restrictions = [2])

mail = Mail(app)

########################################################################
## Global DB helper class instances
########################################################################
DB = DBAccess(app)
DBRead = DBReader(DB)
DBWrite = DBWriter(DB, app)


########################################################################
## Jinja template filters and helper functions
########################################################################
def headrevision(ref):
    head = DBRead.HeadRevision()
    return ref == head

app.jinja_env.tests['headrevision'] = headrevision


def duration(value):
    totalSec = int(math.floor(value))
    minutes = int(math.floor(totalSec / 60))
    totalSec -= minutes * 60
    return "{0}m:{1:0>2}s".format(minutes, totalSec)

app.jinja_env.filters['duration'] = duration


def yesno(value):
    if value == 0:
        return 'no'
    else:
        return 'yes'

app.jinja_env.filters['yesno'] = yesno


def truefalse(value):
    if value == 0:
        return '<span class="failure">false</span>'
    else:
        return '<span class="success">true</span>'

app.jinja_env.filters['truefalse'] = truefalse


def nl2br(value):
    result = value.replace('\n', '<br>\n')
    return result

app.jinja_env.filters['nl2br'] = nl2br


def linuxTime(value):
    result = datetime.datetime.fromtimestamp(value).strftime('%Y-%m-%d %H:%M:%S')
    return result

app.jinja_env.filters['linuxTime'] = linuxTime


def determineOSIcons(machines):
    """Used by templates to match icons to build machines."""
    for entry in machines:
        configuration = entry['BuildMachine']['Configuration']
        if (configuration.startswith("Win")):
            entry['icon'] = 'windows.png'
        elif (configuration.startswith("Osx")):
            entry['icon'] = 'osx.png'
        elif (configuration.startswith("Linux")):
            entry['icon'] = 'linux.png'
        else:
             entry['icon'] = ''


########################################################################
## Flask app routing and web-site entry points
########################################################################
@app.route('/<int:machineID>/<int:revision>')
def show_entriesRev(machineID, revision):
    """Actual template execution for the given machine and revision."""
    #DBWrite.CheckToSendMail(481)

    head = DBRead.HeadRevision()
    # Build Machines
    machines = DBRead.BuildMachines(revision)
    determineOSIcons(machines)
    # Revision Info
    revInfo = DBRead.RevisionInfo(revision)
    buildMachinesBlock = render_template('BuildMachines.html', root='', machines=machines, rev=revision, machineID=machineID, revInfo=revInfo)


    # Navigation Bar (Near Revisions)
    nearRevs = DBRead.RevisionNear(revision, machineID)
    farLeftRev = DBRead.RevisionFarLeft(revision, machineID)
    farRightRev = DBRead.RevisionFarRight(revision, machineID)
    headRev = DBRead.RevisionSummary(head, machineID)
    navigationBarBlock = render_template('NavigationBar.html', nearRevisions=nearRevs, farLeftRev=farLeftRev, farRightRev=farRightRev, headRev=headRev,
                                         head=head, rev=revision, machineID=machineID)

    # Build Process Result
    machine = None
    for entry in machines:
        if (entry['BuildMachine']['id'] == machineID):
            machine = entry['BuildMachine']

    buildProcessResult = DBRead.BuildProcessResult(revision, machineID)
    buildProcessResultBlock = ""
    if (buildProcessResult['Success'] == -1):
        buildProcessResultBlock = render_template('BuildProcessResultMissing.html', machine=machine, buildProcessResult=buildProcessResult, rev=revision, machineID=machineID)
    else:
        buildProcessResultBlock = render_template('BuildProcessResult.html', machine=machine, buildProcessResult=buildProcessResult, rev=revision, machineID=machineID)

    # Final site composite
    return render_template('index.html', buildMachinesBlock=buildMachinesBlock, navigationBarBlock=navigationBarBlock,
                           buildProcessResultBlock=buildProcessResultBlock, rev=revision, head=head)


@app.route('/<int:machineID>')
def show_entriesHead(machineID):
    head = DBRead.HeadRevision()
    return show_entriesRev(machineID, head)


@app.route('/')
def show_entries():
    """Main page: Shows the build results of the HEAD revision."""
    head = DBRead.HeadRevision()
    return show_entriesRev(1, head)


@app.route('/post/', methods=['POST'])
def PostBuildResult():
    """Used by the CNC tool to post new build results."""
    app.logger.debug('Adding build process result via %s', request.method)

    # Add build process result
    buildProcessResultId = DBWrite.AddBuildProcessResult(request.json)
    return 'Success'


@app.teardown_appcontext
def close_db(error):
    DB.close(error)


########################################################################
## Mail Sender
######################################################################## 
def SendMail(rev, mailAddress):
    try:
        if (not app.config['MAIL_USERNAME']):
            return

        if (not mailAddress):
            return

        # Build Machines
        machines = DBRead.BuildMachines(rev)
        determineOSIcons(machines)
        revisionResult = 'Success'
        for entry in machines:
            if (entry['BuildProcessResult']['Success'] == 0):
                revisionResult = 'FAILURE'
                break

        # Revision Info
        revInfo = DBRead.RevisionInfo(rev)
        buildMachinesBlock = render_template('BuildMachines.html', root=app.config['SITE_ROOT'], machines=machines, rev=rev, machineID=-1, revInfo=revInfo)

        # Style CSS Block
        styleCSSBlock = ''
        with app.open_resource('static/style.css', mode='r') as f:
            styleCSSBlock = f.read()

        # Composite Mail
        MailPage = render_template('Mail.html', buildMachinesBlock=buildMachinesBlock, styleCSSBlock=styleCSSBlock)

        subject = '[ez - BuildResult #%d] %s' % (rev, revisionResult)
        msg = Message(subject, sender = app.config['MAIL_USERNAME'], recipients = [mailAddress])
        msg.html = MailPage
        mail.send(msg)
    except:
        app.logger.debug('*** SendMail: Unexpected error: %s', sys.exc_info()[0])


########################################################################
## SVN revision check
######################################################################## 
def ssl_server_trust_prompt(trust_dict):
    return True, 5, True

def svn_get_login(realm, username, may_save):
    return True, app.config['SVN_USER'], app.config['SVN_PASS'], False

def WakeUpBuildMachine():
    app.logger.debug('*** WakeUpBuildMachine ***')
    returnCode = os.system(app.config['WAKE_UP_COMMAND'])
    if (returnCode != 0):
        app.logger.debug('*** WakeUpBuildMachine FAILED: Return Code %d', returnCode)

def CheckSVN():
    try:
        with app.app_context():
            app.logger.debug('*** CheckSVN ***')
            # SVN setup
            svn_api = pysvn.Client()
            svn_api.callback_ssl_server_trust_prompt = ssl_server_trust_prompt
            svn_api.callback_get_login = svn_get_login
            revHead = pysvn.Revision( pysvn.opt_revision_kind.head )
            revlog = svn_api.log(app.config['SVN_ROOT'], revision_start=revHead, revision_end=revHead, discover_changed_paths=False)
            if (revlog):
                rev = revlog[0].data['revision'].number
                if (DBRead.HeadRevision() < rev):
                    WakeUpBuildMachine()
                else:
                    app.logger.debug('*** CheckSVN: at HEAD %d', rev)
            else:
                app.logger.debug('*** CheckSVN FAILED: revlog is empty')
    except:
        app.logger.debug('*** CheckSVN: Unexpected error: %s', sys.exc_info()[0])

    threading.Timer(60, CheckSVN).start()


########################################################################
## Main function
######################################################################## 
if __name__ == '__main__':
    #DB.init(app)
    DB.close("")
    
    DBWrite.callback_SendMail = SendMail
    # Start SVN check timer
    threading.Timer(5, CheckSVN).start()

    if (app.config['DEBUG'] == True):
        # Used for debugging
        app.run(use_debugger=False, debug=True, use_reloader=False)
    else:
        # Used by deployed website
        app.run(host='0.0.0.0', port=8080) 

