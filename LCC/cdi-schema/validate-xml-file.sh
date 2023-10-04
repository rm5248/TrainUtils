#!/bin/sh

# First arg: XML file to validate

exec xmllint --schema cdi-3.xsd $1 --noout
