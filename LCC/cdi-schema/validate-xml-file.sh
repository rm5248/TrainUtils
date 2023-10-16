#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CDI_XSD="$SCRIPT_DIR"/cdi-3.xsd

exec xmllint --schema "$CDI_XSD" $1 --noout
