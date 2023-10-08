# Info-ZIP Family Tree for UnZip

This repo combines a number of publically available sources for the [Info-ZIP](https://infozip.sourceforge.net/) program `unzip` into a single git repo.
The intention is to create a *family tree* of the many version of the source code that are available.

The base and trunk of the tree contain all the Info-ZIP official and beta releases for `unzip` - this is stored in the `infozip` branch of this repo. The branches off `infozip` consist of the main Linux distributions and other forks that have used the Info-ZIP source as their starting point. Each of these is stored in a distinct git branch.


There are no guarantees that this repo contains *all* the available sources for the `unzip` code. The Linux distributions in particular (mostly) contain  the same set of changes so it is unlikely they will all ever be included here. If there is an important fork that is missing please report it.

A companion repo `pmqs/infozip-family-tree-for-zip` contains the equivalent for the Info-ZIP `unzip` program.



## Official & Beta Releases

Below are the sites where official releases and beta source releases were copied from. As alreay mentioned, these sites are merged here into a single git branch called `infozip`.

An attempt has been made to create a semi-accurate timeline by settimg the git timestamps to match the original changes when checking  into this repo. These checkin dates have been sourced from changelogs, when present, and from file timestamps.

When checking the changes into the `infozip` branch the commit messages for the changes  show the origin of each change. Where the Sourceforge site and antinode have the same  files, the Sourceforge copy was used.

* The [Info-ZIP Project](https://sourceforge.net/projects/infozip/) at SourceForge

  This is the primary official site for Info-ZIP. The source files come from https://sourceforge.net/projects/infozip/files. This site contains both official & beta releases of `unzip`.

  An older site for official Info-ZIP releases is ftp://ftp.info-zip.org/pub/infozip/src/. The SourceForge site is newer and contains more beta beta versions.


* [antinode.info](http://antinode.info/ftp/info-zip/)

  This site contains a *lot* of beta releases. Most seem to be focused on VMS changes.

**A word of caution:** All the forks off the `infozip`` branch use the zip 3.0 release as their starting point. There are a lot of infozip 3.1 beta relases in the repo that should be considered beta quality. Caveat Emptor.

## Semi-Official Beta Releases

* The GitHub repo [madler/unzip](https://github.com/madler/unzip)

  This repo is primarily concerned with fixes for
  [zipbomb](https://en.wikipedia.org/wiki/Zip_bomb) issues.
  Most downstream Linux distributions have mergered some or all of these changes. This code is based on the `unzip 6.0` release and can be found here in the `madler-unzip` branch.

## Linux and Other OS Distributions

Most Linux/OS distributions ship with a copy Info-ZIP `unzip` already installed or have the option to add it.
For the most part these distribution are based off the `6.0` unzip release (Oracle being the exception).

The changes nade in these distributions are mostly, but not exclusively, to do with fixing security and coring issues.

There are a *lot* of distributions out there. Patches from some of the main distributions are shown in the table below -- these seem to be the repos where a lot of other distributions source their changes. There is a huge amount of overlap between the distributions

Each distribution is checked into a branch that matches the distribution name. All but one are branched from the `6.0` tag in the  `infozip` branch.


| Branch Name | Code Sourced From | Branched From Infozip Tag |
|---|---|---|
| debian | https://packages.debian.org/source/sid/unzip | 6.0 |
| ubuntu | https://packages.ubuntu.com/mantic/unzip | 6.0 |
| centos | https://gitlab.com/redhat/centos-stream/rpms/zip | 6.0 |
| fedora | https://src.fedoraproject.org/rpms/unzip.git | 6.0 |
| oracle | https://github.com/oracle/solaris-userland/tree/master/components/unzip | 6.10c25 |
| opensuse | https://build.opensuse.org/package/show/Archiving/unzip | 6.0 |
| aix | https://www.ibm.com/support/pages/node/883796 | 6.0 |



## Miscellaneous Distributions

Here  are a few repos that have made custom changes to `unzip`

| Branch | Code Sourced From | Notes | Branched From Infozip Tag |
|---| --- | ---| ---|
| shigeya-unzip60 | https://github.com/shigeya/unzip60 | MacOS changes for localization & [Homebrew](https://brew.sh/)  | 6.0 |
|sskaje-lzfse | https://github.com/sskaje/unzip-lzfse  |  Adds `lzfse` support for reading [ipa](https://en.wikipedia.org/wiki/.ipa) files | 6.0
| bitwiseworks-unzip-os2 | https://github.com/bitwiseworks/unzip-os2 | OS2 Changes | 6.0
| lineageos-android_external_unzip | https://github.com/LineageOS/android_external_unzip | Android build | Debian 6.0
| packit-service-unzip | https://gitlab.com/packit-service/src/unzip | Lots of patches from other Linix distros|  6.0


## Forks not included in this repo

These sites below either contain binaries only or have made changes to the infozip sources but in a way that makes it difficult to metge into this repo. They are included here for reference.

| Code Sourced From | Notes | Branched From Infozip Tag |
| --- | ---| --- |
| https://github.com/jhamby/gnv-unzip | VMS Changes |  6.0
| http://hpux.connect.org.uk/hppd/hpux/Misc/unzip-6.0/ | HP-UX binaries | 6.0
| https://gitlab.com/FreeDOS/archiver/unzip | FreeDOS | 6.0


# AUTHOR

Paul Marquess {pmqs@cpan.org)