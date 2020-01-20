# Synapticon build of the Etherlab EtherCAT Master

This folder contains Synapticon specific documentation, patches and scripts to
work with this fork of the IgH EtherCAT Master.

## Prerequisite

Key branches in this repository are

- `upstream/default` this branch should be up to date with the official
  repository here [IgH Etherlab](https://etherlab.org/en/ethercat/)
- `upstream/default_sncn_patches` this is the `upstream/default` branch with
  the Synapticon folder attached. The `git merge-base` between this branch and
  `upstream/default` should be the head of `upstream/default`.

These two branches are the basis for every further development. For easy
fetching and updating the `upstream/default` branch I recommend to add the
upstream repository as remote in the git repository. The `git-remote-hg` is
required to use Git as Mercurial client, see
 [Git SCM Book](https://git-scm.com/book/en/v2/Git-and-Other-Systems-Git-as-a-Client))
for more information.

Next check out the unofficial patch set from Gavin Lambert from
[here](https://sourceforge.net/u/uecasm/etherlab-patches/commit_browser). You
can decide if you use Git:

```
git clone hg::http://hg.code.sf.net/u/uecasm/etherlab-patches uecasm-etherlab-patches
```

or Mercurial:

```
hg clone http://hg.code.sf.net/u/uecasm/etherlab-patches uecasm-etherlab-patches
```

When using Git as Mercurial client please do not forget to add these
configuration to your git config:

```
remote-hg.track-branches=true
remote-hg.hg-git-compat=true
```

So the commit hashes stay the same. Git and Mercurial have different ways of
calculating the Hash value and the second line in the above configuration
influences the hash calculation on the git side.

Furthermore maybe add

```
core.notesref=refs/notes/hg
```

to the local configuration to see the Mercurial hash and git hash relationship.

## Create a new Release

[//] # (TODO: apply the patches with)

First create a new branch for the new release. The name should follow the format:

```
release/EtherCAT_Master-v1.5.2-sncn-<revision>
```

If the `git merge-base` of `upstream/default` and `upstream/default_sncn_patches` is up-to-date you can just branch off

```
git checkout -b release/EtherCAT_Master-v1.5.2-sncn-<revision> upstream/default_sncn_patches
```

Now we have to patch the EtherCAT master source first with the unofficial
patch-set and then with the Synapticon patches. To make things simpler we have some supporting files in the `synapticon/` folder:

```
synapticon/
├── 0001-Fix-wrong-user-name-field.patch
├── apply_patches.sh
├── CHANGELOG.md
├── install.sh
├── patches
│   ├── 0001-Add-individual-request-of-slave-operational-state.patch
│   ├── 0002-Add-object_code-to-sdo-information-object.patch
│   ├── 0003-Add-sdo-info-access-to-user-api-for-ethercat-library.patch
│   ├── 0004-Add-a-function-to-trigger-bus-rescanning.patch
│   └── 0005-increased-datagram-timeout-which-is-necessary-for-so.patch
├── README.md
├── synapticon_patches.txt
└── unofficial_patch_set.txt

1 directory, 12 files
```

1. The First patch necessary is in the unofficial patch set folder, in the base
directory some author name fields are not properly formed and `git-am` will
refuse to use them. If the unofficial patches are checkout with git you can just
apply the patch `git am -3 0001-Fix-wrong-user-name-field.patch` in the
directory of the unofficial patch set.

2. Now we are ready to apply the patches for the EtherCAT Master. The file
`unoffcial_patch_set.txt` contains a list of all the patches in the unofficial
patch set repository. You can edit the to modify the patch selection. To finally
apply the patches run

```
./synapticon/apply_patches.sh <path/to/uecasm/repository> ./synapticon/unofficial_patch_set.txt
```

In case of a error, the script prints the currently processed patch name. You
can try to resolve the patch merge and then remove all the lines of the already
applied patches (please don't commit this) and run the script again.

3. If all patches are applied properly we can also apply the Synapticon
patches. The list of these patches is given in the file
`synapticon_patches.txt`, with this you can just call

```
./synapticon/apply_patches.sh ./synapticon/patches ./synapticon/unofficial_patch_set.txt
```

Similar to step 2, if an error happens review the console output for the
offending patch, resolve the merge conflict and continue with the next item in
the list.

4. The last step is to update the version of our patched version of the EtherCAT Master by
editing the file `configure.ac` and add the patched version number
(`-sncn-<REVISION>`) to the autoconfig macro `AC_INIT()`, for example:

```
AC_INIT([ethercat],[1.5.2-sncn-8],[fp@igh-essen.com])
```

For the eighth revision of the Synapticon build of the Etherlab EtherCAT
Master.

## Build the Release

The last step is of course the building of the Master itself, this is covered
by the top level readme or alternatively you can use the
`synapticon/install.sh` script.
