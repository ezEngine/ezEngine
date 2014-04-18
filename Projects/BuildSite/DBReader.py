from __future__ import division
from DBAccess import DBAccess


class DBReader:
    ########################################################################
    ## DBReader constructor
    ########################################################################

    """This class reads build results from the database to be handed over to the html templates
    for visualization."""
    # Constructor
    def __init__(self, dbAccess):
        self.DB = dbAccess

    ########################################################################
    ## DBReader public functions
    ########################################################################

    def BuildMachines(self, rev):
        """Returns a list of all build machine records including a summary for the given revision."""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM BuildMachines ORDER BY ConfigurationName ASC')
        entries = cur.fetchall()

        machines = list()
        for entry in entries:
            machines.append({'BuildMachine':entry, 'BuildProcessResult':self.BuildProcessResultSummary(rev, entry[0])})
        
        return machines


    def RevisionInfo(self, rev):
        """Returns the author, commit message etc of the given revision."""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM Revisions WHERE id=?', (rev,))
        entries = cur.fetchall()
        if not entries:
            return None
        return entries[0]


    def HeadRevision(self):
        """Returns the highest revision that has a build result stored in the DB.
        Note this is not he HEAD revision of the server, should be changed soon."""
        db = self.DB.getDB()
        cur = db.execute('SELECT MAX(Revision) FROM BuildProcessResults')
        entries = cur.fetchall()
        return entries[0][0]


    def BuildProcessResultSummary(self, rev, BuildMachineID):
        """Returns the build result for the given revision and machine.
        For each sub-step a bool is put into the dictionary indicating whether the step succeeded or not.
        We do not want to return all result data here as that would be a massive amount of data as this is done for all build machines
        to create the build machine overview on the web-page."""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM BuildProcessResults WHERE Revision=? AND BuildMachine=?', (rev, BuildMachineID))
        BuildProcessRes = cur.fetchall()

        if (not BuildProcessRes):
            return {'Revision':rev, 'Timestamp':0, 'Success':-1, 'Duration':0.0, 'Errors':"",
                    'SVNResult':None, 'CMakeResult':None, 'BuildResult':None, 'TestResult':None}

        summary = {'Revision':BuildProcessRes[0][1], 'Timestamp':BuildProcessRes[0][2], 'Success':BuildProcessRes[0][3], 'Duration':BuildProcessRes[0][4], 'Errors':BuildProcessRes[0][5],
                   'SVNResult':None, 'CMakeResult':None, 'BuildResult':None, 'TestResult':None}
        
        cur = db.execute('SELECT Success FROM SVNResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        SVNResult = cur.fetchall()
        summary['SVNResult'] = SVNResult[0][0]

        cur = db.execute('SELECT Success FROM CMakeResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        CMakeResult = cur.fetchall()
        summary['CMakeResult'] = CMakeResult[0][0]

        cur = db.execute('SELECT Success FROM BuildResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        BuildResult = cur.fetchall()
        summary['BuildResult'] = BuildResult[0][0]

        cur = db.execute('SELECT Success FROM TestResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        TestResult = cur.fetchall()
        summary['TestResult'] = TestResult[0][0]
        return summary


    def BuildProcessResult(self, rev, BuildMachineID):
        """Basically the same as 'BuildProcessResultSummary' but the sub-steps hold their entire data.
        Can I easily merge this with the Summary version without making it ugly?"""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM BuildProcessResults WHERE Revision=? AND BuildMachine=?', (rev, BuildMachineID))
        BuildProcessRes = cur.fetchall()

        if (not BuildProcessRes):
            return {'Revision':rev, 'Timestamp':0, 'Success':-1, 'Duration':0.0, 'Errors':"",
                    'SVNResult':None, 'CMakeResult':None, 'BuildResult':None, 'TestResult':None}

        summary = {'Revision':BuildProcessRes[0][1], 'Timestamp':BuildProcessRes[0][2], 'Success':BuildProcessRes[0][3], 'Duration':BuildProcessRes[0][4], 'Errors':BuildProcessRes[0][5],
                   'SVNResult':None, 'CMakeResult':None, 'BuildResult':None, 'TestResult':None}
        
        # Fill sub-step information
        cur = db.execute('SELECT * FROM SVNResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        SVNResult = cur.fetchall()
        summary['SVNResult'] = SVNResult[0]

        cur = db.execute('SELECT * FROM CMakeResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        CMakeResult = cur.fetchall()
        summary['CMakeResult'] = CMakeResult[0]


        # BuildResults
        cur = db.execute('SELECT * FROM BuildResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        BuildResult = cur.fetchall()
        summary['BuildResult'] = BuildResult[0]

        cur = db.execute('SELECT * FROM BuildTargetResults WHERE BuildResult=?', (BuildResult[0][0],))
        BuildTargetResults = cur.fetchall()

        # is that an efficient way to fill an array?
        L = []
        for entry in BuildTargetResults:
            L.append({'BuildTargetResult':entry, 'ProcessResult':self.ProcessResult('BuildTargetResult', entry[0])})
        summary['BuildTargetResults'] = L


        # TestResults
        cur = db.execute('SELECT * FROM TestResults WHERE BuildProcessResult=?', (BuildProcessRes[0][0],))
        TestResult = cur.fetchall()
        summary['TestResult'] = TestResult[0]

        # Get TestTargetResults.
        cur = db.execute('SELECT * FROM TestTargetResults WHERE TestResult=?', (TestResult[0][0],))
        TestTargetResults = cur.fetchall()

        L = []
        for entry in TestTargetResults:
            L.append({'TestTargetResult':entry, 'ProcessResult':self.ProcessResult('TestTargetResult', entry[0])})
        summary['TestTargetResults'] = L

        return summary


    def ProcessResult(self, Parent, ID):
        """Returns the 'ProcessResult' record for the given parent sub-step 'Parent', e.g. 'TestTargetResult'.
        ID is the id of the parent sub-step record."""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM ProcessResults WHERE {}=?'.format(Parent.encode('ascii','ignore')), (ID,))
        entries = cur.fetchall()
        if not entries:
            return None
        return entries[0]


    def RevisionSummary(self, rev, machineID):
        """Returns a dictionary for a rev and machine containing the build result success for
        the machine and for all machines."""
        db = self.DB.getDB()
        cur = db.execute('SELECT * FROM BuildProcessResults WHERE Revision=?', (rev,))
        entries = cur.fetchall()

        if (not entries):
            return {'Revision':rev,'CurrentMachine':0, 'AllMachines':0 }
        
        # TODO: unknown state if not all machines have been built yet.
        # How do enums work?
        allGood = 1
        currentGood = 0
        for BuildProcessResult in entries:
            if (BuildProcessResult['Success'] == 0):
                allGood = 0
            elif (BuildProcessResult['BuildMachine'] == machineID and BuildProcessResult['Success'] == 1):
                currentGood = 1
        
        return {'Revision':rev,'CurrentMachine':currentGood, 'AllMachines':allGood }


    def RevisionNear(self, rev, machineID):
        """Builds a list of revisions that are near the given revision.
        Used for building the revision summary at the top of the web-page for navigation."""
        head = self.HeadRevision()
        revEnd = rev + 10
        if (revEnd > head):
            revEnd = head
        revStart = revEnd - 20;
        if (revStart < 0):
            revStart = 0

        L = []
        for x in range(revStart, revEnd + 1):
            L.append(self.RevisionSummary(x, machineID))
            
        return L


    def RevisionFarLeft(self, rev, machineID):
        """The fast backward button in the revision summary at the top of the web-page for navigation."""
        head = self.HeadRevision()
        revEnd = rev + 10
        if (revEnd > head):
            revEnd = head
        revStart = revEnd - 30;
        if (revStart < 0):
            revStart = 0

        return self.RevisionSummary(revStart, machineID)


    def RevisionFarRight(self, rev, machineID):
        """The fast forward button in the revision summary at the top of the web-page for navigation."""
        head = self.HeadRevision()
        revEnd = rev + 20
        if (revEnd > head):
            revEnd = head

        return self.RevisionSummary(revEnd, machineID)

    ########################################################################
    ## DBReader private fields
    ########################################################################

    DB = None