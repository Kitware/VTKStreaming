#!/usr/bin/env bash

bash_source="${BASH_SOURCE//\\//}" &&
cd "${bash_source%/*}/.." &&
Utilities/GitSetup/setup-user && echo &&
Utilities/GitSetup/setup-hooks && echo &&
Utilities/GitSetup/setup-lfs && echo &&
Utilities/Scripts/SetupGitAliases.sh && echo &&
(Utilities/GitSetup/setup-upstream ||
 echo 'Failed to setup origin.  Run this again to retry.') && echo &&
(Utilities/GitSetup/setup-gitlab ||
 echo 'Failed to setup GitLab.  Run this again to retry.') && echo &&
Utilities/GitSetup/tips

echo "Initializing and updating git submodules..."
git submodule update --init --recursive

# Rebase master by default
git config rebase.stat true
git config branch.master.rebase true

# Disable Gerrit hook explicitly so the commit-msg hook will
# not complain even if some gerrit remotes are still configured.
git config hooks.GerritId false

