dnl Process this file with autoconf to produce a configure script.

AC_INIT([weatherstation],[m4_esyscmd([cat __VERSION__ | tr -d '\n'])],[kb@socfpga.io],[weatherstation],[http://www.socfpga.io])

AC_DEFUN([weatherstation_version_major], m4_esyscmd([cat __VERSION__ | awk -F "." '{print $1}']))
AC_DEFUN([weatherstation_version_minor], m4_esyscmd([cat __VERSION__ | awk -F "." '{print $2}']))
AC_DEFUN([weatherstation_version_micro], m4_esyscmd([cat __VERSION__ | tr -d '\n' | awk -F "." '{print $3}']))
AC_DEFUN([weatherstation_version], m4_esyscmd([cat __VERSION__ | tr -d '\n']))

WEATHERSTATION_VERSION_MAJOR=weatherstation_version_major
WEATHERSTATION_VERSION_MINOR=weatherstation_version_minor
WEATHERSTATION_VERSION_MICRO=weatherstation_version_micro
WEATHERSTATION_VERSION=weatherstation_version

AC_SUBST(WEATHERSTATION_VERSION_MAJOR)
AC_SUBST(WEATHERSTATION_VERSION_MINOR)
AC_SUBST(WEATHERSTATION_VERSION_MICRO)
AC_SUBST(WEATHERSTATION_VERSION)

AC_CONFIG_MACRO_DIR([m4])
dnl AC_CANONICAL_TARGET
AM_PROG_AR
LT_INIT()

AM_INIT_AUTOMAKE([1.10 -Wall foreign no-define no-installman subdir-objects])
AM_MAINTAINER_MODE

dnl ***************************************************************************
dnl Check for modules
dnl ***************************************************************************
dnl add package module dependencies here
dnl PKG_CHECK_MODULES([],[])

AC_CONFIG_FILES([
Makefile
source/weatherstation/Makefile
])

AC_OUTPUT

