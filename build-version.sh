#!/bin/bash

COMMIT_VERSION=`git rev-list HEAD | wc -l | awk '{print $1}'`
SHA_VERSION=`git rev-parse --short HEAD`

echo $COMMIT_VERSION $SHA_VERSION
