#!/bin/sh -e

ghi --repo=derf/feh --verbose list > TODO
git commit -m 'Update TODO (via github)' TODO
