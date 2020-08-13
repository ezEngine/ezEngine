# Generating the API docs with Doxygen

## Install Doxygen and Graphviz Dot Tool

* Install Doxygen: <https://www.doxygen.nl/download.html>

* Install the Graphviz Dot Tool: <http://www.graphviz.org/Download.php>

## Generate Docs

* Use the *Doxyfile* in this directory.
* Set `%ezEngine%/Documentation` as the working directory.

Either run it with the *Doxywizard*, or from the command line:

```cmd
%ezEngine%\Documentation>"C:\Program Files\doxygen\bin\doxygen.exe" Doxyfile
```

The output is written to: `%ezEngine%/Output/Doxygen/html/index.htm`

## Searchdata.xml

If you need to generate a searchdata.xml file, check this documentation:
<https://www.doxygen.nl/manual/extsearch.html>

You might need to run Doxygen twice, once with the settings to generate the `searchdata.xml` file, once to create the actual output with search enabled.

