DROP INDEX IF EXISTS ProcessResultsIndexSVN;
DROP INDEX IF EXISTS ProcessResultsIndexCMake;
DROP INDEX IF EXISTS ProcessResultsIndexBuild;
DROP INDEX IF EXISTS ProcessResultsIndexTest;
DROP INDEX IF EXISTS TestTargetResultsIndex;
DROP INDEX IF EXISTS TestResultsIndex;
DROP INDEX IF EXISTS BuildTargetResultsIndex;
DROP INDEX IF EXISTS BuildResultsIndex;
DROP INDEX IF EXISTS CMakeResultsIndex;
DROP INDEX IF EXISTS SVNResultsIndex;
DROP INDEX IF EXISTS BuildProcessResultsIndex;

DROP TABLE IF EXISTS ProcessResults;
DROP TABLE IF EXISTS TestTargetResults;
DROP TABLE IF EXISTS TestResults;
DROP TABLE IF EXISTS BuildTargetResults;
DROP TABLE IF EXISTS BuildResults;
DROP TABLE IF EXISTS CMakeResults;
DROP TABLE IF EXISTS SVNResults;
DROP TABLE IF EXISTS BuildProcessResults;
DROP TABLE IF EXISTS BuildMachines;
DROP TABLE IF EXISTS Revisions;

--SVN Revision
CREATE TABLE Revisions (
  id                   INTEGER PRIMARY KEY,
  Date                 INTEGER not null,
  Author               TEXT not null,
  Message              TEXT,
  ChangedPaths         TEXT
);


--BuildMachine
CREATE TABLE BuildMachines (
  id                   INTEGER PRIMARY KEY autoincrement,
  Configuration        TEXT not null,
  ConfigurationName    TEXT not null unique,
  DirectHardwareAccess INTEGER not null
);


--BuildProcessResult
CREATE TABLE BuildProcessResults (
  id            INTEGER PRIMARY KEY autoincrement,
  Revision      INTEGER not null,
  Timestamp     INTEGER not null,
  Success       INTEGER not null,
  Duration      REAL not null,
  Errors        TEXT not null,
  BuildMachine  INTEGER not null REFERENCES BuildMachines(id) ON DELETE CASCADE
);

CREATE INDEX BuildProcessResultsIndex ON BuildProcessResults (BuildMachine);


--SVNResult
CREATE TABLE SVNResults (
  id                   INTEGER PRIMARY KEY autoincrement,
  Success              INTEGER not null,
  Duration             REAL not null,
  Errors               TEXT not null,
  BuildProcessResult   INTEGER not null REFERENCES BuildProcessResults(id) ON DELETE CASCADE
);

CREATE INDEX SVNResultsIndex ON SVNResults (BuildProcessResult);


--CMakeResult
CREATE TABLE CMakeResults (
  id                   INTEGER PRIMARY KEY autoincrement,
  Success              INTEGER not null,
  Duration             REAL not null,
  Errors               TEXT not null,
  BuildProcessResult   INTEGER not null REFERENCES BuildProcessResults(id) ON DELETE CASCADE
);

CREATE INDEX CMakeResultsIndex ON CMakeResults (BuildProcessResult);


--BuildResult
CREATE TABLE BuildResults (
  id                   INTEGER PRIMARY KEY autoincrement,
  Success              INTEGER not null,
  Duration             REAL not null,
  Errors               TEXT not null,
  BuildProcessResult   INTEGER not null REFERENCES BuildProcessResults(id) ON DELETE CASCADE
);

CREATE INDEX BuildResultsIndex ON BuildResults (BuildProcessResult);

CREATE TABLE BuildTargetResults (
  id           INTEGER PRIMARY KEY autoincrement,
  Name         TEXT not null,
  Experimental INTEGER not null,
  Success      INTEGER not null,
  Duration     REAL not null,
  Errors       TEXT not null,
  BuildResult  INTEGER not null REFERENCES BuildResults(id) ON DELETE CASCADE
);

CREATE INDEX BuildTargetResultsIndex ON BuildTargetResults (BuildResult);


--TestResult
CREATE TABLE TestResults (
  id                   INTEGER PRIMARY KEY autoincrement,
  Success              INTEGER not null,
  Duration             REAL not null,
  Errors               TEXT not null,
  BuildProcessResult   INTEGER not null REFERENCES BuildProcessResults(id) ON DELETE CASCADE
);

CREATE INDEX TestResultsIndex ON TestResults (BuildProcessResult);

CREATE TABLE TestTargetResults (
  id                    INTEGER PRIMARY KEY autoincrement,
  Name                  TEXT not null,
  NeedsHardwareAccess   INTEGER not null,
  Experimental          INTEGER not null,
  Success               INTEGER not null,
  Duration              REAL not null,
  Errors                TEXT not null,
  TestResult            INTEGER not null REFERENCES TestResults(id) ON DELETE CASCADE
);

CREATE INDEX TestTargetResultsIndex ON TestTargetResults (TestResult);


--ProcessRes
CREATE TABLE ProcessResults (
  id                    INTEGER PRIMARY KEY autoincrement,
  ExitCode              INTEGER not null,
  StdOut                TEXT not null,
  ErrorOut              TEXT not null,
  Duration              REAL not null,
  SVNResult             INTEGER DEFAULT null REFERENCES SVNResults(id)          ON DELETE CASCADE,
  CMakeResult           INTEGER DEFAULT null REFERENCES CMakeResults(id)        ON DELETE CASCADE,
  BuildTargetResult     INTEGER DEFAULT null REFERENCES BuildTargetResults(id)  ON DELETE CASCADE,
  TestTargetResult      INTEGER DEFAULT null REFERENCES TestTargetResults(id)   ON DELETE CASCADE
);

CREATE INDEX ProcessResultsIndexSVN ON ProcessResults (SVNResult);
CREATE INDEX ProcessResultsIndexCMake ON ProcessResults (CMakeResult);
CREATE INDEX ProcessResultsIndexBuild ON ProcessResults (BuildTargetResult);
CREATE INDEX ProcessResultsIndexTest ON ProcessResults (TestTargetResult);
