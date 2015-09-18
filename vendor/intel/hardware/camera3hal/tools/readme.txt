# Camera Metadata XML tools
## Introduction
This scripts make use of the existing scripts provided by Android in
/system/media/camera/docs to manipulate the camera metadata in an XML form.
It generates the C code used in Camera HAL that depends on the metadata 
definitions from the framework

## Generated Files
Currently only one:
MetadataInfoAutoGen.h: it contains tables of structs to help in the parsing 
of the static metadata from camera3_profiles.xml

## Dependencies
* Python 2.7.x+
* Beautiful Soup 4+ - HTML/XML parser, used to parse `metadata_properties.xml`
* Mako 0.7+         - Template engine, needed to do file generation.
* Markdown 2.1+     - Plain text to HTML converter, for docs formatting.
* Tidy              - Cleans up the XML/HTML files.
* XML Lint          - Validates XML against XSD schema.

## Quick Setup (Ubuntu Precise):
sudo apt-get install python-mako
sudo apt-get install python-bs4
sudo apt-get install python-markdown
sudo apt-get install tidy
sudo apt-get install libxml2-utils #xmllint

## Quick Setup (MacPorts)
sudo port install py27-beautifulsoup4
sudo port install py27-mako
sudo port install py27-markdown
sudo port install tidy
sudo port install libxml2 #xmllint

# IPU4 Log Parsing tools
This tool is a python script to post-process the traces produced by the IPU4
PSL. Some of the traces (when enabled) can be visualized graphically by a log
viewer tool.
The script filters a logcat dump with those traces and performs some basic
post-processing.
The resulting trace can be currently visualized with a tool called TimeToPicy