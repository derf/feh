#!/bin/sh -e

test -z "$(git ls-files --modified --deleted --others --exclude-standard)"

ghi --repo=derf/feh --verbose list > TODO
git add TODO
git commit -m 'Update TODO (via github)'
