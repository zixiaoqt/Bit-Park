
@echo off

git rev-list HEAD | wc -l | gawk '{print $1}'

